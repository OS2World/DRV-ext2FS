
;
; int DH32ENTRY DevHlp32_setIRQ(
;                               unsigned short offset_irq,  /* ebp + 8  */
;                               unsigned short interrupt_level, /* ebp + 12 */
;                               unsigned short sharing_flag,    /* ebp + 16 */
;                               unsigned short default_dataseg  /* ebp + 20 */
;                              );
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
        ASSUME CS:CODE16, DS:NOTHING, ES:FLAT

        public thunk16$DevHlp32_setIRQ

thunk16$DevHlp32_setIRQ:
        call es:[DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_setIRQ
;        jmp32 thunk32$DevHlp32_setIRQ

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_setIRQ
        public         DevHlp32_setIRQ

;
; int DH32ENTRY DevHlp32_setIRQ(
;                               unsigned short offset_irq,  /* ebp + 8  */
;                               unsigned short interrupt_level, /* ebp + 12 */
;                               unsigned short sharing_flag,    /* ebp + 16 */
;                               unsigned short default_dataseg  /* ebp + 20 */
;                              );
;
DevHlp32_setIRQ proc near
        enter 0, 0
        push ds
        push ebx
        mov ax, [ebp + 8]
        mov bx, [ebp + 12]
        mov dh, [ebp + 16]
        mov cx, [ebp + 20]
        mov ds, cx
        mov   dl, DevHlp_SetIRQ
        jmp far ptr thunk16$DevHlp32_setIRQ
thunk32$DevHlp32_setIRQ label far:
        jc short @@error
        mov ax, NO_ERROR
@@error:
        movzx eax, ax
        pop ebx
        pop ds
        leave
        ret
DevHlp32_setIRQ endp

CODE32  ends

        end
