
;
; int DH32ENTRY DevHlp32_UnSetIRQ(
;                                 unsigned short interrupt_level,   /* ebp + 8  */
;                                 unsigned short data16_segment     /* ebp + 12 */
;                                );
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
        ASSUME CS:CODE16, ES:FLAT

        public thunk16$DevHlp32_UnSetIRQ

thunk16$DevHlp32_UnSetIRQ:
        call es:[DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_UnSetIRQ
;        jmp32 thunk32$DevHlp32_UnSetIRQ

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_UnSetIRQ
        public         DevHlp32_UnSetIRQ

;
; int DH32ENTRY DevHlp32_UnSetIRQ(
;                                 unsigned short interrupt_level,   /* ebp + 8  */
;                                 unsigned short data16_segment     /* ebp + 12 */
;                                );
;
DevHlp32_UnSetIRQ proc near
    enter 0, 0
    push ds
    push ebx
    mov bx, [ebp + 8]
    mov cx, [ebp + 12]
    mov ds, cx
        mov   dl, DevHlp_UnSetIRQ
        jmp far ptr thunk16$DevHlp32_UnSetIRQ
thunk32$DevHlp32_UnSetIRQ label far:
    xor eax, eax
    pop ebx
    pop ds
    leave
        ret
DevHlp32_UnSetIRQ endp

CODE32  ends

        end
