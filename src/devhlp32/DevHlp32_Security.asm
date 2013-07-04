
;
; int DH32ENTRY DevHlp32_Security(
;                 unsigned long   func,     /* ebp + 8  */
;                                 void           *ptr       /* ebp + 12 */
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

        DevHlp_Security equ 044h

    DHSEC_GETEXPORT equ 048a78df8h
    DHSEC_SETIMPORT equ 073ae3627h
    DHSEC_GETINFO   equ 033528882h

CODE16 segment
        ASSUME CS:CODE16, DS:FLAT

        public thunk16$DevHlp32_Security

thunk16$DevHlp32_Security:
        call [DevHelp2]
;        jmp32 thunk32$DevHlp32_Security
        jmp far ptr FLAT:thunk32$DevHlp32_Security

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_Security
        public         DevHlp32_Security

;
; int DH32ENTRY DevHlp32_Security(
;                 unsigned long   func,     /* ebp + 8  */
;                                 void           *ptr       /* ebp + 12 */
;                                );
;
DevHlp32_Security proc near
        push ebp
        mov  ebp, esp
    push ebx

        mov ecx, [ebp + 12]   ; ptr
        mov eax, [ebp + 8]    ; func
        mov edx, DevHlp_Security
        jmp far ptr thunk16$DevHlp32_Security
thunk32$DevHlp32_Security label far:
        mov eax, NO_ERROR
    pop ebx
        leave
        ret
DevHlp32_Security endp

CODE32  ends

        end
