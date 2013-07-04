
;
; int DH32ENTRY DevHlp32_ResetEventSem(
;                        unsigned long handle        /* ebp + 8  */
;            int *nposts             /* ebp +12 */
;                       );
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

        public thunk16$DevHlp32_ResetEventSem

thunk16$DevHlp32_ResetEventSem:
        call [DevHelp2]
;        jmp32 thunk32$DevHlp32_ResetEventSem
        jmp far ptr FLAT:thunk32$DevHlp32_ResetEventSem

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_ResetEventSem
        public         DevHlp32_ResetEventSem

;
; int DH32ENTRY DevHlp32_ResetEventSem(
;                        unsigned long handle        /* ebp + 8  */
;            int *nposts             /* ebp +12 */
;                       );
;
DevHlp32_ResetEventSem proc near
        push  ebx
        push  edi
        mov   eax, [esp + 8]                 ; handle
        mov   edi, [esp + 12]            ; address of var for posts
        mov   dl, DevHlp_ResetEventSem
        jmp   far ptr thunk16$DevHlp32_ResetEventSem
thunk32$DevHlp32_ResetEventSem label far:
        jc short @@error
        mov eax, NO_ERROR
@@error:
        and   eax,0000ffffh
        pop   edi
        pop   ebx
        ret
DevHlp32_ResetEventSem endp

CODE32  ends

        end
