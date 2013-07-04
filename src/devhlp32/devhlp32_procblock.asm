
;
; int DH32ENTRY DevHlp32_ProcBlock(
;                          unsigned long  eventid,       /* bp + 8  */
;                          long           timeout,       /* bp + 12 */
;                          short          interruptible  /* bp + 16 */
;                         );
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

        public thunk16$DevHlp32_ProcBlock

thunk16$DevHlp32_ProcBlock:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_ProcBlock
;        jmp32 thunk32$DevHlp32_ProcBlock

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_ProcBlock
        public         DevHlp32_ProcBlock

;
; int DH32ENTRY DevHlp32_ProcBlock(
;                          unsigned long  eventid,       /* bp + 8  */
;                          long           timeout,       /* bp + 12 */
;                          short          interruptible  /* bp + 16 */
;                         );
;
DevHlp32_ProcBlock proc near
        push ebp
        mov  ebp, esp
        push ebx
        push edi
        mov bx, [ebp + 8]                 ; eventid_low
        mov ax, [ebp + 10]                ; eventid_high
        mov di, [ebp + 14]                ; timeout_high
        mov cx, [ebp + 12]                ; timeout_low
        mov dh, [ebp + 16]                ; interruptible_flag
        mov dl, DevHlp_ProcBlock
        jmp far ptr thunk16$DevHlp32_ProcBlock
thunk32$DevHlp32_ProcBlock label far:
;        mov ah, 0
    and eax, 0FFh
        pop edi
        pop ebx
        leave
        ret
DevHlp32_ProcBlock endp

CODE32  ends

        end
