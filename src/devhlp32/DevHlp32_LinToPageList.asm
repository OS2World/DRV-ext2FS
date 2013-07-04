
;
; int DH32ENTRY DevHlp32_LinToPageList(
;                                void            *lin,         /* ebp + 8  */
;                                unsigned long    size,        /* ebp + 12 */
;                                struct PageList *pages        /* ebp + 16 */
;                                unsigned long   *nr_pages,    /* ebp + 20 */
;                               );
;

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
        include devhlp.inc
        include segdef.inc
        include r0thunk.inc

CODE16 segment
        ASSUME CS:CODE16, DS:FLAT

        public thunk16$DevHlp32_LinToPageList

thunk16$DevHlp32_LinToPageList:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_LinToPageList
;        jmp32 thunk32$DevHlp32_LinToPageList

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_LinToPageList
        public         DevHlp32_LinToPageList

;
; int DH32ENTRY DevHlp32_LinToPageList(
;                                void            *lin,         /* ebp + 8  */
;                                unsigned long    size,        /* ebp + 12 */
;                                struct PageList *pages        /* ebp + 16 */
;                                unsigned long   *nr_pages,    /* ebp + 20 */
;                               );
;
DevHlp32_LinToPageList proc near
        push ebp
        mov  ebp, esp
        push ebx
        push edi

        mov   eax, [ebp + 8]                 ; lin
        mov   ecx, [ebp + 12]                ; size
        mov   edi, [ebp + 16]                ; Flags
        mov   edx, DevHlp_LinToPageList
        jmp far ptr thunk16$DevHlp32_LinToPageList
thunk32$DevHlp32_LinToPageList label far:
        jc short @@error

        mov   ebx, [ebp + 20]                ; LinAddr
    cmp ebx, 0
    jz short @@out
        mov   [ebx], eax
@@out:
        mov   eax, NO_ERROR
@@error:
        pop edi
        pop ebx
        leave
        ret
DevHlp32_LinToPageList endp

CODE32  ends

        end
