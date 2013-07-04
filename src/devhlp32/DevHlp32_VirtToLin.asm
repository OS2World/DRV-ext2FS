
;
; int DH32ENTRY DevHlp32_VirtToLin(
;                                  PTR16  virt  [ebp + 8]
;                                  void **plin  [ebp + 12]
;     );
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

        public thunk16$DevHlp32_VirtToLin

thunk16$DevHlp32_VirtToLin:
        call [DevHelp2]
;        jmp32 thunk32$DevHlp32_VirtToLin
        jmp far ptr FLAT:thunk32$DevHlp32_VirtToLin

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_VirtToLin
        public         DevHlp32_VirtToLin

;
; int DH32ENTRY DevHlp32_VirtToLin(
;                                  PTR16  virt  [ebp + 8]
;                                  void **plin  [ebp + 12]
;     );
;
DevHlp32_VirtToLin proc near
        push ebp
        mov  ebp, esp
        push esi
        push edi

        movzx esi, word ptr[ebp + 8]        ; ofs
        mov ax, word ptr[ebp + 10]          ; seg
        mov dl, DevHlp_VirtToLin
        jmp far ptr thunk16$DevHlp32_VirtToLin
thunk32$DevHlp32_VirtToLin label far:
        jc short @@error           ; if error, EAX = error code
        mov edi, [ebp + 12]        ; plin
        mov [edi], eax             ;*plin
        mov eax, NO_ERROR
@@error:
        pop edi
        pop esi
        leave
        ret
DevHlp32_VirtToLin endp

CODE32  ends

        end
