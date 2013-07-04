
;
;   fs_init.asm:
;       16-bit FS_INIT entry point with the thunks to
;       get into the 32-bit code.
;
;       FSD initialization occurs at system initialization time
;       (while CONFIG.SYS is processed). FSDs are loaded through
;       the IFS= configuration command in CONFIG.SYS. Once the
;       FSD has been loaded, FS_INIT is called to initialize it.
;
;       FSDs are structured the same as dynamic link library
;       modules. Once an FSD is loaded, the initialization routine
;       FS_INIT is called. This gives the FSD the ability to
;       process any parameters that may appear on the CONFIG.SYS
;       command line, which are passed as a parameter to the
;       FS_INIT routine. A LIBINIT routine in an FSD will be
;       ignored.
;
;       OS/2 FSDs initialize in protect mode. Because of the special
;       state of the system, an FSD may make dynamic link
;       system calls at init-time.
;
;       The list of systems calls that an FSD may make are as follows:
;
;       -- DosBeep
;       -- DosChgFilePtr
;       -- DosClose
;       -- DosDelete
;       -- DosDevConfig
;       -- DosDevIoCtl
;       -- DosFindClose
;       -- DosFindFirst
;       -- DosFindNext
;       -- DosGetEnv
;       -- DosGetInfoSeg
;       -- DosGetMessage
;       -- DosOpen
;       -- DosPutMessage
;       -- DosQCurDir
;       -- DosQCurDisk
;       -- DosQFileInfo
;       -- DosQFileMode
;       -- DosQSysInfo
;       -- DosRead
;       -- DosWrite
;
;       The FSD may not call ANY FS helper routines at initialization time.
;
;       Note that multiple code and data segments are not discarded by
;       the loader as in the case of device drivers.
;
;       The FSD may call DosGetInfoSeg to obtain access to the global
;       and process local information segments. The local segment may be
;       used in the context of all processes without further effort to
;       make it accessible and has the same selector. The local infoseg
;       is not valid in real mode or at interrupt time.

;
; Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
; Copyright (C) 2001 Ulrich M”ller.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, in version 2 as it comes in the COPYING
; file with this distribution.
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;

        .386p

        INCL_DOS        equ 1
        INCL_DOSERRORS  equ 1
        include os2.inc
        include devhlp.inc

        include segdef.inc
        include r0thunk.inc
        include fsd.inc

; *********************************************************
;
;       Declarations
;
; *********************************************************

DosTable2 struc
        d2_ErrMap24                     dd ?
        d2_MsgMap24                     dd ?
        d2_Err_Table_24                 dd ?
        d2_CDSAddr                      dd ?
        d2_GDT_RDR1                     dd ?
        d2_InterruptLevel               dd ?
        d2__cInDos                      dd ?
        d2_zero_1                       dd ?
        d2_zero_2                       dd ?
        d2_FlatCS                       dd ?
        d2_FlatDS                       dd ?
        d2__TKSSBase                    dd ?
        d2_intSwitchStack               dd ?
        d2_privateStack                 dd ?
        d2_PhysDiskTablePtr             dd ?
        d2_forceEMHandler               dd ?
        d2_ReserveVM                    dd ?
        d2_pgpPageDir                   dd ?
        d2_unknown                      dd ?
DosTable2 ends

; --------------------------------------------------
;
;   forward declarations to the marker labels
;   in src\interface\start.asm and end.asm;
;   these are used by the VMLock code below
;
; --------------------------------------------------

CODE32 segment
    extrn code32_start: byte
    extrn code32_end: byte
CODE32 ends

CODE16 segment
    extrn code16_start: byte
    extrn code16_end: byte
CODE16 ends

DATA32 segment
    extrn dgroup_start: byte
    extrn dgroup_end: byte
DATA32 ends

CONST32_RO segment
    extrn const32_ro_start: byte
    extrn const32_ro_end: byte
CONST32_RO ends

DATA16 segment
    extrn data16_start: byte
    extrn data16_end: byte
DATA16 ends

; *********************************************************
;
;       DATA32
;
; *********************************************************

DATA32 segment
    ulPageCount dw 0

    ; 12 bytes lock ids for CODE32, DGROUP, Const32RO, CODE16 each
    ; these receive the lock IDs from the VMLock code below
    CODE32Lock      byte 12 dup(0)
    DGROUPLock      byte 12 dup(0)
    CONST32ROLock   byte 12 dup(0)
    CODE16Lock      byte 12 dup(0)
    DATA16Lock      byte 12 dup(0)

    ; string constants used in fs32_init.c
    public DS32_CACHE
    DS32_CACHE      byte "CACHE:", 0
    public DS32_Q
    DS32_Q          byte "Q", 0
    public DS32_NO_STRAT2
    DS32_NO_STRAT2  byte "NO_STRAT2", 0
    public DS32_CASE_RETENSIVE
    DS32_CASE_RETENSIVE byte "CASE_RETENSIVE", 0
    public DS32_RW
    DS32_RW         byte "RW", 0
    public DS32_NO_AUTO_FSCK
    DS32_NO_AUTO_FSCK byte "NO_AUTO_FSCK", 0
    public DS32_WRITE_THROUGH
    DS32_WRITE_THROUGH byte "WRITE_THROUGH", 0
    public DS32_ERRORS
    DS32_ERRORS     byte "ERRORS=", 0

DATA32 ends

; *********************************************************
;
;       DATA16
;
; *********************************************************

DATA16 segment
    public DS16_Banner
    DS16_Banner db "ext2-os2.ifs (OS/2 ext2 file-system driver)", 13, 10
                db "Copyright (C) 1995, 1996, 1997  Matthieu Willm", 13, 10
                db "Copyright (C) 2001 Ulrich M”ller", 13, 10, 13, 10
                                        ; NOTE: no terminating null byte!
    public DS16_Banner_End              ; marker byte for calculating the length
    DS16_Banner_End label byte

    public usWritten                    ; output from Dos16Write
    usWritten dw 0
DATA16 ends

; *********************************************************
;
;       CODE16
;
; *********************************************************

CODE16 segment
assume cs:CODE16, ds:DATA16

    ; --------------------------------------------------
    ;
    ;   FS_ATTRIBUTE:
    ;
    ;
    ; --------------------------------------------------

    public FS_ATTRIBUTE
    FS_ATTRIBUTE dd 00000000000000000000000000000000b
    ;               ||||                        ||||
    ;               |+++--- FSD version         ||||
    ;               +-- additional attributes   ||||
    ;                                           ||||
    ;                     level 7 requests -----+|||
    ;                     (case-preserving?)     |||
    ;                 lock notifications   ------+||
    ;                     UNC support      -------+|
    ;                     remote FSD       --------+

    ; --------------------------------------------------
    ;
    ;   FS_NAME:
    ;       required IFS export showing the kernel the
    ;       file system name (must be null-terminated).
    ;
    ;       All comparisons with user-specified strings
    ;       are done similar to file names; case is ignored
    ;       and embedded blanks are significant. FS_NAMEs,
    ;       however, may be input to applications by users.
    ;       Embedded blanks should be avoided. The name
    ;       exported as FS_NAME need NOT be the same as the
    ;       1-8 FSD name in the boot sector of formatted
    ;       media, although it may be. The ONLY name the
    ;       kernel pays any attention to, and the only name
    ;       accessible to user programs through the API, is
    ;       the name exported as FS_NAME.
    ;
    ; --------------------------------------------------

    public FS_NAME
    FS_NAME db "ext2", 0

    ; --------------------------------------------------
    ;
    ;   FS_INIT:
    ;       initialization routine, called during CONFIG.SYS
    ;       parsing.
    ;
    ;       int far pascal FS_INIT(char far * cmdline,           ; bp + 14
    ;                              unsigned long DevHelp,        ; bp + 10
    ;                              unsigned long far *pMiniFSD); ; bp + 6
    ;
    ;       THIS IS RUNNING ON RING 3.
    ;
    ;       Old ext2fs code opened MWDD32.SYS here and threw
    ;       out an ioctl, giving MWDD32 the address of the
    ;       32-bit fs32_init() C routine, which was then
    ;       called from ring 0.
    ;
    ;       Essentially, not much is left of this... I have
    ;       made the following changes:
    ;
    ;       1)  Much 16-bit assembler code is here in FS_INIT
    ;           to prepare calling the 32-bit fs32_init()
    ;           even at ring 3.
    ;
    ;       2)  Because many of the calls that the old fs32_init
    ;           did required ring 0,
    ;
    ;           a)  some of this has been rewritten in asm here;
    ;
    ;           b)  more has been moved to fs32_mount, which now
    ;               checks if it's being called for the first
    ;               time and then calls the various init routines
    ;               in the Linux sources on ring 0. Seems to work.
    ;
    ; --------------------------------------------------

    public FS_INIT
    FS_INIT proc far

            ; EnterProc       ,,alignesp
            push    bp
            mov     bp, sp
            and     sp, not 3
            movzx   ebp, bp

            ; SaveReg <ebx,esi,edi,ds,es>
            push    ebx
            push    esi
            push    edi
            push    ds
            push    es

            RPL_RING3   EQU 003H

        ;
        ; initialize FLAT DS segments so we can read/write into DATA32
        ;
            ; sandervl: the flat descriptors have DPL 3 until
            ; slightly before PM comes up
            mov     ax, Dos32FlatDS                 ; flat kernel data selector
            or      ax, RPL_RING3
            mov     ds, ax
            mov     es, ax

            assume ds:FLAT, es:FLAT

        ; save the address of DevHelp from arguments given to us
            mov     eax, [bp + 10]
            mov     DevHelp2, eax

            ; int 3  ; <------------------------------------

        ;
        ; get TKSSBase so we can thunk stack to flat
        ; (this has been added here because this was
        ; done by mwdd32.sys originally)
        ;

            ; TKSSBase is used by __StackToFlat() to convert a stack based address
            ; to a FLAT address without the overhead of DevHlp_VirtToLin.
            ;
            ; TKSSBase is in the DosTable, which is obtained through GetDOSVar
            ; with the undocumented index 9.
            ;
            ; The layout is :
            ;     byte      count of following dword (n)
            ;     dword 1   -+
            ;       .        |
            ;       .        | this is DosTable1
            ;       .        |
            ;     dword n   -+
            ;     byte      count of following dword (p)
            ;     dword 1   -+
            ;       .        |
            ;       .        | this is DosTable2
            ;       .        |
            ;     dword p   -+
            ;
            ; Flat CS  is dword number 10 in DosTable2
            ; Flat DS  is dword number 11 in DosTable2
            ; TKSSBase is dword number 12 in DosTable2
            ;
            mov eax, 9                          ; undocumented DosVar : DosTable pointer
            xor ecx, ecx                        ; unused with this index
            mov edx, DevHlp_GetDOSVar
            call dword ptr [DevHelp2]
            jc CannotGetDosTable                   ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                   ;                               ³
            ; ax:bx points to DosTable now                                         ³
                                                   ;                               ³
            mov es, ax                             ;                               ³
            movzx ebx, bx                          ;                               ³
                    ; es:ebx points to DosTable now                                ³
            ; now skip DosTable1                                                   ³
            movzx ecx, byte ptr es:[ebx]     ; count of dword in DosTable1         ³
            mov eax, es:[ebx + 4 * ecx + 2].d2__TKSSBase      ;                    ³
            mov TKSSBase, eax                                 ;                    ³
                                                              ;                    ³
        ;                                                     ;                    ³
        ; get info segs                                       ;                    ³
        ;                                                     ;                    ³
            ;                                                                      ³
            ; this code had to be rewritten in asm because the                     ³
            ; original fs32_init() called DevHelp32_GetInfoSegs,                   ³
            ; which did too much thunking for ring 3 (among others,                ³
            ; it changed the segment registers). So we do                          ³
            ; DevHlp_GetDOSVar here and convert this to linear                     ³
            ; for the C code to use later (stored in DATA32).                      ³
            ;                                                                      ³
                                                              ;                    ³
            ; 1) global info seg                                                   ³
            mov al, DHGETDOSV_SYSINFOSEG        ; 1, global info seg               ³
            xor ecx, ecx                        ; unused with this index           ³
            mov dl, DevHlp_GetDOSVar                          ;                    ³
            call dword ptr [DevHelp2]                         ;                    ³
            jc  CannotGetDosTable                  ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                    ; ax:bx points to index now                                    ³
            ; convert 16:16 global info seg pointer to linear                      ³
            ; VirtToLin expects AX:ESI ptr                                         ³
            movzx esi, bx                                     ;                    ³
            mov dl, DevHlp_VirtToLin                          ;                    ³
            call dword ptr [DevHelp2]                         ;                    ³
            jc  CannotGetDosTable                             ;                    ³
                    ; now we have linear address in EAX                            ³
            ; store in DATA32 (variable is in fs32_init.c)                         ³
            mov G_pSysInfoSeg, eax                            ;                    ³
                                                              ;                    ³
            ; 2) local info seg                                                    ³
            mov al, DHGETDOSV_LOCINFOSEG        ; 2, global info seg               ³
            xor ecx, ecx                        ; unused with this index           ³
            mov dl, DevHlp_GetDOSVar                          ;                    ³
            call dword ptr [DevHelp2]                         ;                    ³
            jc  CannotGetDosTable                  ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                    ; ax:bx points to index now                                    ³
            ; convert 16:16 global info seg pointer to linear                      ³
            ; VirtToLin expects AX:ESI ptr                                         ³
            movzx esi, bx                                     ;                    ³
            mov dl, DevHlp_VirtToLin                          ;                    ³
            call dword ptr [DevHelp2]                         ;                    ³
            jc  CannotGetDosTable                  ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                    ; now we have linear address in EAX                            ³
            ; store in DATA32 (variable is in fs32_init.c)                         ³
            mov G_pLocInfoSeg, eax                            ;                    ³
                                                              ;                    ³
        ; lock CODE32 into physical memory (added)                                 ³
            ; ebx = startaddr                                                      ³
            mov ebx, offset FLAT:code32_start                 ;                    ³
            ; ecx = length                                                         ³
            mov ecx, offset FLAT:code32_end                   ;                    ³
            sub ecx, offset FLAT:code32_start                 ;                    ³
            ; edi = &ulPageCount (DATA32)                                          ³
            mov edi, offset FLAT:ulPageCount                  ;                    ³
            ; esi = &Code32Lock (12 bytes, DATA32)                                 ³
            mov esi, offset FLAT:CODE32Lock                   ;                    ³
            ; eax = lock flags                                                     ³
            mov eax, 010h                ; VMDHL_LONG    0x0010                    ³
                                                              ;                    ³
            mov dl, DevHlp_VMLock                             ;                    ³
            call dword ptr [DevHelp2]                         ;                    ³
                       ; if carry set, EAX has error code                          ³
            jc FS_INIT_farjmp1_back         ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                              ;                    ³  ³
        ; lock DGROUP into physical memory (added)                                 ³  ³
            ; ebx = startaddr                                                      ³  ³
            mov ebx, offset FLAT:dgroup_start                 ;                    ³  ³
            ; ecx = length                                                         ³  ³
            mov ecx, offset FLAT:dgroup_end                   ;                    ³  ³
            sub ecx, offset FLAT:dgroup_start                 ;                    ³  ³
            ; edi = &ulPageCount (DATA32)                                          ³  ³
            mov edi, offset FLAT:ulPageCount                  ;                    ³  ³
            ; esi = &Code32Lock (12 bytes, DATA32)            ;                    ³  ³
            mov esi, offset FLAT:DGROUPLock                   ;                    ³  ³
            ; eax = lock flags                                ;                    ³  ³
            mov eax, 010h                ; VMDHL_LONG    0x0010                    ³  ³
                                                              ;                    ³  ³
            mov dl, DevHlp_VMLock                             ;                    ³  ³
            call dword ptr [DevHelp2]                         ;                    ³  ³
                       ; if carry set, EAX has error code                          ³  ³
            jc FS_INIT_farjmp1_back         ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                              ;                    ³  ³
        ; lock CONST32_RO into physical memory (added)        ;                    ³  ³
                                                              ;                    ³  ³
            ; ebx = startaddr                                 ;                    ³  ³
            mov ebx, offset FLAT:const32_ro_start             ;                    ³  ³
            ; ecx = length                                    ;                    ³  ³
            mov ecx, offset FLAT:const32_ro_end               ;                    ³  ³
            sub ecx, offset FLAT:const32_ro_start             ;                    ³  ³
            ; edi = &ulPageCount (DATA32)                     ;                    ³  ³
            mov edi, offset FLAT:ulPageCount                  ;                    ³  ³
            ; esi = &Code32Lock (12 bytes, DATA32)            ;                    ³  ³
            mov esi, offset FLAT:CONST32ROLock                ;                    ³  ³
            ; eax = lock flags                                ;                    ³  ³
            mov eax, 010h                ; VMDHL_LONG    0x0010                    ³  ³
                                                              ;                    ³  ³
            mov dl, DevHlp_VMLock                             ;                    ³  ³
            call dword ptr [DevHelp2]                         ;                    ³  ³
                       ; if carry set, EAX has error code                          ³  ³
            jc FS_INIT_farjmp1_back         ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                              ;                    ³  ³
        ; lock CODE16 into physical memory                    ;                    ³  ³
            ; ebx = startaddr                                 ;                    ³  ³
            mov ebx, offset FLAT:code16_start                 ;                    ³  ³
            ; ecx = length                                    ;                    ³  ³
            mov ecx, offset FLAT:code16_end                   ;                    ³  ³
            sub ecx, offset FLAT:code16_start                 ;                    ³  ³
            ; edi = &ulPageCount (DATA32)                     ;                    ³  ³
            mov edi, offset FLAT:ulPageCount                  ;                    ³  ³
            ; esi = &Code32Lock (12 bytes, DATA32)            ;                    ³  ³
            mov esi, offset FLAT:CODE16Lock                   ;                    ³  ³
            ; eax = lock flags                                                     ³  ³
            mov eax, 010h                ; VMDHL_LONG 0x0010                       ³  ³
                                                              ;                    ³  ³
            mov dl, DevHlp_VMLock                             ;                    ³  ³
            call dword ptr [DevHelp2]                         ;                    ³  ³
                       ; if carry set, EAX has error code                          ³  ³
            jc FS_INIT_farjmp1_back         ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                              ;                    ³  ³
        ; lock DATA16 into physical memory                    ;                    ³  ³
            ; ebx = startaddr                                 ;                    ³  ³
            mov ebx, offset FLAT:data16_start                 ;                    ³  ³
            ; ecx = length                                    ;                    ³  ³
            mov ecx, offset FLAT:data16_end                   ;                    ³  ³
            sub ecx, offset FLAT:data16_start                 ;                    ³  ³
            ; edi = &ulPageCount (DATA32)                     ;                    ³  ³
            mov edi, offset FLAT:ulPageCount                  ;                    ³  ³
            ; esi = &Code32Lock (12 bytes, DATA32)            ;                    ³  ³
            mov esi, offset FLAT:DATA16Lock                   ;                    ³  ³
            ; eax = lock flags                                                     ³  ³
            mov eax, 010h                ; VMDHL_LONG 0x0010                       ³  ³
                                                              ;                    ³  ³
            mov dl, DevHlp_VMLock                             ;                    ³  ³
            call dword ptr [DevHelp2]                         ;                    ³  ³
                       ; if carry set, EAX has error code                          ³  ³
            jc FS_INIT_farjmp1_back         ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                                                              ;                    ³  ³
        ; get command line                                    ;                    ³  ³
                                                              ;                    ³  ³
            ; int 3  ; <------------------------------------

            ; get 16:16 parameters command line               ;                    ³  ³
            ; (ptr is NULL if there are none)                 ;                    ³  ³
            mov     eax, [bp + 14]                            ;                    ³  ³
            test    eax, eax                                  ;                    ³  ³
            jz      NoParams                       ; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿       ³  ³
                                                              ;            ³       ³  ³
            ; we do have parameters:                          ;            ³       ³  ³
            ; convert 16:16 cmdLine to linear for C code      ;            ³       ³  ³
            ; VirtToLin expects AX:ESI ptr                    ;            ³       ³  ³
            movzx   esi, ax                                   ;            ³       ³  ³
            shr     eax, 16                                   ;            ³       ³  ³
            mov     dl, DevHlp_VirtToLin                      ;            ³       ³  ³
            call    dword ptr [DevHelp2]                      ;            ³       ³  ³
            jnc     CallFarFS32Init                           ;            ³       ³  ³
                                                              ;            ³       ³  ³
NoParams:                                                     ; <ÄÄÄÄÄÄÄÄÄÄÙ       ³  ³
            ; error: load NULL pointer                                             ³  ³
            xor eax, eax                                               ;           ³  ³
                                                                       ;           ³  ³
CallFarFS32Init:                                                       ;           ³  ³
            jmp far ptr FLAT:FS_INIT_farjmp1            ; ÄÄÄ¿                     ³  ³
                                                        ;    ³                     ³  ³
            ; +++++++ 32-bit thunk                      ;    ³                     ³  ³
            CODE32    segment                           ;    ³                     ³  ³
                assume cs:CODE32                        ;    ³                     ³  ³
FS_INIT_farjmp1 label near                              ; <ÄÄÙ                     ³  ³
                ; eax = FS32_INIT(pszParams)                           ;           ³  ³
                push  eax            ; flat pszParams ptr                          ³  ³
                call far ptr FLAT:FS32_INIT                            ;           ³  ³
                                                                       ;           ³  ³
                jmp far ptr CODE16:FS_INIT_farjmp1_back ; ÄÄÄ¿                     ³  ³
            CODE32    ends                              ;    ³                     ³  ³
            ; ------- 32-bit thunk                      ;    ³                     ³  ³
                                                        ;    ³                     ³  ³
CannotGetDosTable:                                      ;    ³           <ÄÄÄÄÄÄÄÄÄÙ  ³
            mov eax, ERROR_INVALID_PARAMETER            ;    ³                        ³
                                                        ;    ³                        ³
FS_INIT_farjmp1_back label far                          ; <ÄÄÙ <ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

            ; eax has return value of either FS32_INIT()
            ; or one of the DevHlps above that might have failed

            test eax, eax
            jnz FS_INIT_exit

        ; no error so far: say hello world

            ;
            ; moved "hello world" from the C code to here
            ; as well. The original fs32_init() called
            ; DevHlp32_SaveMessage, which allocated GDT
            ; selectors and lots of other scary stuff...
            ; too much for ring 3.
            ;
            ; I tested, and we can simply do a
            ; DosWrite(STDOUT), and it will display as
            ; well... no need for SaveMessage.
            ;

            extrn DOS16WRITE: far

            ; DS is still FLAT kernel data segment

            ; arc = Dos16Write(HFILE hf,
            ;                  VOID FAR* pvData,
            ;                  USHORT usLength,
            ;                  USHORT FAR * pusWritten)
            push 1                  ; HFILE stdout
            push seg DATA16  ; seg DS16_Banner
            mov ax, word ptr offset DATA16:DS16_Banner
            push ax          ; ofs DS16_Banner
            mov bx, word ptr offset DATA16:DS16_Banner_End
            sub bx, ax
            push bx          ; usLength
            push seg DATA16  ; seg DS16_Banner
            push word ptr offset DATA16:usWritten
            call DOS16WRITE

FS_INIT_exit:
            ; clean up
            pop es
            pop ds
            pop edi
            pop esi
            pop ebx
            leave
            ret 12

    FS_INIT endp

CODE16 ends

end

