
;
; int DH32ENTRY DevHlp32_GetDosVar(
;                                  int     index,      /* ebp + 8  */
;                                  PTR16  *value,      /* ebp + 12 */
;                                  int     member      /* ebp + 16 */
;                                 );
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

        public thunk16$DevHlp32_GetDosVar

thunk16$DevHlp32_GetDosVar:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_GetDosVar
;        jmp32 thunk32$DevHlp32_GetDosVar

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_GetDosVar
        public         DevHlp32_GetDosVar

;
; int DH32ENTRY DevHlp32_GetDosVar(
;                                  int     index,      /* ebp + 8  */
;                                  PTR16  *value,      /* ebp + 12 */
;                                  int     member      /* ebp + 16 */
;                                 );
;
DevHlp32_GetDosVar proc near
        enter 0, 0
        push ebx
        push esi
        push edi

        mov eax, [ebp + 8]          ; index
        mov ecx, [ebp + 16]         ; member
        mov dl, DevHlp_GetDOSVar
        jmp far ptr thunk16$DevHlp32_GetDosVar
thunk32$DevHlp32_GetDosVar label far:
        jc short @@error                ; if error, EAX = error code
        mov edi, [ebp + 12]
        mov [edi], bx                   ; value (ofs)
        mov [edi + 2], ax               ; value (seg)
        mov eax, NO_ERROR
@@error:
        pop edi
        pop esi
        pop ebx
        leave
        ret
DevHlp32_GetDosVar endp

CODE32  ends

        end
