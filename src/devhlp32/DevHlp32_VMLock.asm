
;
; int DH32ENTRY DevHlp32_VMLock(
;                               unsigned long flags,                 [ebp + 8]
;                               unsigned long lin,                   [ebp + 12]
;                               unsigned long length,                [ebp + 16]
;                               void          *pPageList,            [ebp + 20]
;                               void          *pLockHandle,          [ebp + 24]
;                               unsigned long *pPageListCount        [ebp + 28]
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
        ASSUME CS:CODE16, DS:FLAT

        public thunk16$DevHlp32_VMLock

thunk16$DevHlp32_VMLock:
        call [DevHelp2]
        jmp far ptr FLAT:thunk32$DevHlp32_VMLock
;        jmp32 thunk32$DevHlp32_VMLock

CODE16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        ; public thunk32$DevHlp32_VMLock
        public         DevHlp32_VMLock

;
; int DH32ENTRY DevHlp32_VMLock(
;                               unsigned long flags,                 [ebp + 8]
;                               unsigned long lin,                   [ebp + 12]
;                               unsigned long length,                [ebp + 16]
;                               void          *pPageList,            [ebp + 20]
;                               void          *pLockHandle,          [ebp + 24]
;                               unsigned long *pPageListCount        [ebp + 28]
;                              );
;
DevHlp32_VMLock proc near
        push ebp
        mov  ebp, esp
        push ebx
        push esi
        push edi

        mov ebx, [ebp + 12]        ; lin
        mov ecx, [ebp + 16]        ; length
        mov edi, [ebp + 20]        ; pPageList    (FLAT)
        mov esi, [ebp + 24]        ; pLockHandle  (FLAT)
        mov eax, [ebp + 8]         ; flags
        mov dl, DevHlp_VMLock
        jmp far ptr thunk16$DevHlp32_VMLock
thunk32$DevHlp32_VMLock label far:
        jc short @@error           ; if error, EAX = error code
        mov edi, [ebp + 28]        ; pPageListCount
        mov [edi], eax             ;*pPageListCount
        mov eax, NO_ERROR
@@error:
        pop edi
        pop esi
        pop ebx
        leave
        ret
DevHlp32_VMLock endp

CODE32  ends

        end
