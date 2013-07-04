
;
; int DH32ENTRY DevHlp32_ProcRun(
;                        unsigned long eventid        /* ebp + 8  */
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

        public thunk16$DevHlp32_ProcRun

thunk16$DevHlp32_ProcRun:
        call [DevHelp2]
;        jmp32 thunk32$DevHlp32_ProcRun
        jmp far ptr FLAT:thunk32$DevHlp32_ProcRun

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_ProcRun
        public         DevHlp32_ProcRun

;
; int DH32ENTRY DevHlp32_ProcRun(
;                        unsigned long eventid        /* ebp + 8  */
;                       );
;
DevHlp32_ProcRun proc near
        push ebx
        mov bx, [esp + 8]                 ; eventid_low
        mov ax, [esp + 10]                ; eventid_high
        mov dl, DevHlp_ProcRun
        jmp far ptr thunk16$DevHlp32_ProcRun
thunk32$DevHlp32_ProcRun label far:
        pop ebx
        ret
DevHlp32_ProcRun endp

CODE32  ends

        end
