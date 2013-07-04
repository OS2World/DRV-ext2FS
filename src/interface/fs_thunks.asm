
;
;   fs_thunks.asm:
;       this file contains ALL 16-bit entry points EXCEPT
;       FS_INIT (which is in fs_init.asm).
;
;       Each FS_xxx entry point gets called directly from
;       the kernel. Here we thunk the parameters and then
;       call the corresponding 32-bit C function (fs32_xxx()),
;       or return an error directly if the call is not
;       supported.
;
;       Calling conventions between FS router, FSD, and FS
;       helpers are:
;
;       --  Arguments will be pushed in left-to-right order
;           onto the stack.
;
;       --  The callee is responsible for cleaning up the stack.
;
;       --  Registers DS, SI, DI, BP, SS, SP are preserved.
;
;       --  Return conditions appear in AX with the convention
;           that AX == 0 indicates successful completion.
;           AX != 0 indicates an error with the value of AX
;           being the error code.
;
;       Looks like the _Far _Pascal16 calling convention to me.
;
;       Interrupts must ALWAYS be enabled and the direction
;       flag should be presumed to be undefined. Calls to the
;       FS helpers will change the direction flag at will.
;
;       In OS/2, file system drivers are always called in kernel
;       protect mode. This has the advantage of allowing the FSD
;       to execute code without having to account for preemption;
;       no preemption occurs when in kernel mode. While this
;       greatly simplifies FSD structure, it forces the FSD to
;       yield the CPU when executing long segments of code. In
;       particular, an FSD must not hold the CPU for more than
;       2 milliseconds at a time. The FSD helper FSH_YIELD is
;       provided so that FSDs may relinquish the CPU.
;
;       File system drivers cannot have any interrupt-time
;       activations. Because they occupy high, movable, and
;       swappable memory, there is no guarantee of addressability
;       of the memory at interrupt time.
;
;       Each FS service routine may block.

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

        INCL_DOSERRORS equ 1
        include bseerr.inc

        include segdef.inc
        include r0thunk.inc
        include fsd.inc

; --------------------------------------------------
;
;   thunk_fs:
;       macro for the thunks defined below. [umoeller]
;
;       This macro implements the direct FS_* entry
;       points which are exported from the IFS and
;       therefore called by the kernel.
;
;       Essentially, using
;           thunk_fs CHDIR 16
;
;       will
;       a)  define the FS_CHDIR export,
;       b)  call far FS32_CHDIR,
;       c)  remove 16 bytes (4 DWORDs) from the stack.
;
; --------------------------------------------------

thunk_fs macro name, size
    CODE16 SEGMENT
    assume CS:CODE16

    public FS_&name
    FS_&name proc far
            enter 0, 0
            push ds
            push es
            mov ax, Dos32FlatDS
            mov ds, ax
            mov es, ax

            assume DS:FLAT, ES:FLAT

            lea eax, [bp + 6]       ; adress of FS_xxx parameter
            add eax, TKSSBase      ; ... converted to 32 bits FLAT
            push eax                ; ... and passed to 32 bits routine
            ; now call 32-bit function;
            ; all the fs32_xxx implementations have
            ; been defined as _Far32 _Pascal, so
            ; callee cleans up the stack
            call far ptr FLAT:FS32_&name

            assume DS:NOTHING, ES:NOTHING

            pop es
            pop ds

            leave
            retf size
    FS_&name endp
    CODE16 ends
endm

;
;   IFS ENTRY POINTS COME HERE
;       All these are called directly by the OS/2 kernel.
;
;       NOTE: No segment definition here. The macro
;       itself writes out a CODE16 block.
;

thunk_fs ALLOCATEPAGESPACE, 16
thunk_fs CHDIR            , 16
thunk_fs CHGFILEPTR       , 16
thunk_fs CLOSE            , 12
thunk_fs COMMIT           , 12
thunk_fs DELETE           , 14
thunk_fs DOPAGEIO         , 12
thunk_fs EXIT             , 6
thunk_fs FILEATTRIBUTE    , 14
thunk_fs FILEINFO         , 20
thunk_fs FINDCLOSE        , 8
thunk_fs FINDFIRST        , 38
thunk_fs FINDFROMNAME     , 30
thunk_fs FINDNEXT         , 22
thunk_fs FLUSHBUF         , 4
thunk_fs FSCTL            , 28
thunk_fs FSINFO           , 12
thunk_fs IOCTL            , 32
thunk_fs MKDIR            , 20
thunk_fs MOUNT            , 16
thunk_fs MOVE             , 22
thunk_fs NEWSIZE          , 14
thunk_fs OPENCREATE       , 42
thunk_fs OPENPAGEFILE     , 30
thunk_fs PATHINFO         , 24
thunk_fs READ             , 18
thunk_fs RMDIR            , 14
thunk_fs SHUTDOWN         , 6
thunk_fs WRITE            , 18

; --------------------------------------------------
;
;   nothunk_fs:
;       second macro for entry points which have
;       no 32-bit C code and should simply return
;       an error.
;
; --------------------------------------------------

nothunk_fs macro name, rc, size
    public FS_&name

FS_&name proc far
    mov ax, rc
    retf size
FS_&name endp
endm

CODE16 segment
    assume CS:CODE16

    nothunk_fs ATTACH           , ERROR_NOT_SUPPORTED, 22
                ; for remove file systems only

    nothunk_fs CANCELLOCKREQUEST, ERROR_NOT_SUPPORTED, 12
    nothunk_fs COPY             , ERROR_CANNOT_COPY  , 14
    nothunk_fs FILEIO           , ERROR_NOT_SUPPORTED, 20
    nothunk_fs FILELOCKS        , ERROR_NOT_SUPPORTED, 24
    nothunk_fs FINDNOTIFYCLOSE  , ERROR_NOT_SUPPORTED, 2
    nothunk_fs FINDNOTIFYFIRST  , ERROR_NOT_SUPPORTED, 36
    nothunk_fs FINDNOTIFYNEXT   , ERROR_NOT_SUPPORTED, 18
    nothunk_fs NMPIPE           , ERROR_NOT_SUPPORTED, 22
    nothunk_fs PROCESSNAME      , NO_ERROR           , 4
    nothunk_fs SETSWAP          , ERROR_NOT_SUPPORTED, 8

    ;    FS_INIT                         is in fs_init.asm
    ;    FS_VERIFYUNCNAME                N/A : local FSD

CODE16 ends

CODE32 segment
assume CS:CODE32, DS:FLAT

    ;
    ; _fs32_allocatepagespace:
    ;

    public _fs32_allocatepagespace
    _fs32_allocatepagespace proc near
        push eax
        db 09ah
        dd offset FLAT:FS32_ALLOCATEPAGESPACE
        dw Dos32FlatCS
        ret
    _fs32_allocatepagespace endp

    ;
    ; _fs32_opencreate:
    ;

    public _fs32_opencreate
    _fs32_opencreate proc near
        push eax
        db 09ah
        dd offset FLAT:FS32_OPENCREATE
        dw Dos32FlatCS
        ret
    _fs32_opencreate endp

CODE32 ends

        end
