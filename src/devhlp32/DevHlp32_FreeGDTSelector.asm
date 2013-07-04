
;
; int DH32ENTRY2 DevHlp32_FreeGDTSelector(
;                    unsigned short sel                  /* ax */
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

        public thunk16$DevHlp32_FreeGDTSelector

thunk16$DevHlp32_FreeGDTSelector:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_FreeGDTSelector
;        jmp32 thunk32$DevHlp32_FreeGDTSelector

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_FreeGDTSelector
        public         DevHlp32_FreeGDTSelector

;
; int DH32ENTRY2 DevHlp32_FreeGDTSelector(
;                    unsigned short sel                  /* ax */
;                   );
;
DevHlp32_FreeGDTSelector proc near
        mov dl, DevHlp_FreeGDTSelector
        jmp far ptr thunk16$DevHlp32_FreeGDTSelector
thunk32$DevHlp32_FreeGDTSelector label far:
    jc short @@error
    mov eax, NO_ERROR
@@error:
        ret
DevHlp32_FreeGDTSelector endp

CODE32  ends

        end
