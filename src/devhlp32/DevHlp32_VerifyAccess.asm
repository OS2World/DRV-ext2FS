
;
; int DH32ENTRY DevHlp32_VerifyAccess(
;                                PTR16          Seg,      /* ebp + 8  */
;                                unsigned short Size,     /* ebp + 12 */
;                                int            Flags     /* ebp + 16 */
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

        public thunk16$DevHlp32_VerifyAccess

thunk16$DevHlp32_VerifyAccess:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_VerifyAccess
;        jmp32 thunk32$DevHlp32_VerifyAccess

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_VerifyAccess
        public         DevHlp32_VerifyAccess

;
; int DH32ENTRY DevHlp32_VerifyAccess(
;                                PTR16          Seg,      /* ebp + 8  */
;                                unsigned short Size,     /* ebp + 12 */
;                                int            Flags     /* ebp + 16 */
;                               );
;
DevHlp32_VerifyAccess proc near
        push  ebp
        mov   ebp, esp
        push  ebx
    push  ecx
        push  edi

        mov   di, [ebp + 8]                 ; Off
        mov   ax, [ebp + 10]                ; Seg
        mov   cx, [ebp + 12]                ; Size
        mov   dh, [ebp + 16]                ; Flags
        mov   dl, DevHlp_VerifyAccess
        jmp far ptr thunk16$DevHlp32_VerifyAccess
thunk32$DevHlp32_VerifyAccess label far:
        jc short @@error

        mov   eax, NO_ERROR
@@error:
        and   eax, 0000ffffh
        pop   edi
        pop   ecx
        pop   ebx
        leave
        ret
DevHlp32_VerifyAccess endp

CODE32  ends

        end
