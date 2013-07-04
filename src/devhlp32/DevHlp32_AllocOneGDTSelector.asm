
;
; int DH32ENTRY DevHlp32_AllocOneGDTSelector(
;                    unsigned short *psel       /* ebp + 8 !!! STACK BASED !!! */
;                   );
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

        public thunk16$DevHlp32_AllocOneGDTSelector
    thunk16$DevHlp32_AllocOneGDTSelector:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_AllocOneGDTSelector

CODE16 ends

CODE32 segment
        ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING
        public DevHlp32_AllocOneGDTSelector

;
; int DH32ENTRY DevHlp32_AllocOneGDTSelector(
;                    unsigned short *psel       /* ebp + 8 !!! STACK BASED !!! */
;                   );

DevHlp32_AllocOneGDTSelector proc near
        enter 2, 0
        push es
        push ebx
        push esi
        push edi

        lea edi, [ebp - 2]         ; tmpsel
        mov eax, ss
        mov es, eax
        mov ecx, 1         ; one selector
        mov dl, DevHlp_AllocGDTSelector
        jmp far ptr thunk16$DevHlp32_AllocOneGDTSelector
        public thunk32$DevHlp32_AllocOneGDTSelector
thunk32$DevHlp32_AllocOneGDTSelector:
        jc short @@error           ; if error, EAX = error code
        mov ax, [ebp - 2]          ; tmpsel
        mov ebx, [ebp + 8]         ; psel
        mov ss:[ebx], ax           ; *psel
        mov eax, NO_ERROR
@@error:
        pop edi
        pop esi
        pop ebx
        pop es
        leave
        ret
DevHlp32_AllocOneGDTSelector endp

CODE32  ends

end


