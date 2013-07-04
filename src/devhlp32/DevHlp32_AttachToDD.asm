
;
; int _Optlink DevHlp32_AttachToDD(
;                    char *ddname,               /* eax !!! STACK BASED !!! */
;                    void *ddtable               /* edx !!! STACK BASED !!! */
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

        public thunk16$DevHlp32_AttachToDD

thunk16$DevHlp32_AttachToDD:
        call es:[DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_AttachToDD
;        jmp32 thunk32$DevHlp32_AttachToDD

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_AttachToDD
        public         DevHlp32_AttachToDD

;
; int _Optlink DevHlp32_AttachToDD(
;                    char *ddname,               /* eax !!! STACK BASED !!! */
;                    void *ddtable               /* edx !!! STACK BASED !!! */
;                   );
;
DevHlp32_AttachToDD proc near
        push ds
        push ebx
        push edi

        mov ebx, eax                    ; offset ds:ddname
        mov edi, edx                    ; offset ds:ddtable
        mov eax, ss
        mov ds, eax
        mov dl, DevHlp_AttachDD
        jmp far ptr thunk16$DevHlp32_AttachToDD
thunk32$DevHlp32_AttachToDD label far:
        jc short @@error                         ; if error, EAX = error code
        mov eax, NO_ERROR
@@error:
        pop edi
        pop ebx
        pop ds
        ret
DevHlp32_AttachToDD endp

CODE32  ends

        end
