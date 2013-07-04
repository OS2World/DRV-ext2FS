
;
; int DH32ENTRY DevHlp32_PageListToLin(
;                                      unsigned long     size,        /* ebp + 8 */
;                                      struct PageList  *pPageList,   /* ebp + 12 */
;                                      void            **pLin         /* ebp + 16 */
;                                     );
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

        public thunk16$DevHlp32_PageListToLin

thunk16$DevHlp32_PageListToLin:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_PageListToLin
;        jmp32 thunk32$DevHlp32_PageListToLin

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_PageListToLin
        public         DevHlp32_PageListToLin

;
; int DH32ENTRY DevHlp32_PageListToLin(
;                                      unsigned long     size,        /* ebp + 8 */
;                                      struct PageList  *pPageList,   /* ebp + 12 */
;                                      void            **pLin         /* ebp + 16 */
;                                     );
;
DevHlp32_PageListToLin proc near
    enter 0, 0
        push edi
    push ebx

        mov   ecx, [ebp + 8]                 ; size
        mov   edi, [ebp + 12]                ; pPageList
        mov   dl, DevHlp_PageListToLin
        jmp far ptr thunk16$DevHlp32_PageListToLin
thunk32$DevHlp32_PageListToLin label far:
        jc short @@error

        mov   ebx, [ebp + 16]                ; pLin
    cmp ebx, 0
    jz short @@out
        mov   [ebx], eax
        mov   eax, NO_ERROR
@@error:
        pop ebx
        pop edi
        leave
        ret

@@out:
    mov eax, ERROR_INVALID_PARAMETER
    jmp short @@error
DevHlp32_PageListToLin endp

CODE32  ends

        end
