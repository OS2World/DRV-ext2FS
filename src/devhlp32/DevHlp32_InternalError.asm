
;
; void DH32ENTRY DevHlp32_InternalError(
;                           char *msg,  /* ebp + 8  */
;                           int   msglen    /* ebp + 12 */
;                          );
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
        ASSUME CS:CODE16, DS:DATA16, ES:FLAT

        public thunk16$DevHlp32_InternalError

thunk16$DevHlp32_InternalError:
        call es:[DevHelp2]
    ;
    ; We should never reach this point
    ;
    int 3
;        jmp far ptr FLAT:thunk32$DevHlp32_InternalError

CODE16 ends

DATA16 segment
    public panic_buffer
    panic_buffer db 512 dup (?)
DATA16 ends

CODE32 segment
ASSUME CS:CODE32, DS:FLAT, ES:FLAT, SS:NOTHING

        public         DevHlp32_InternalError

;
; void DH32ENTRY DevHlp32_InternalError(
;                           char *msg,  /* ebp + 8  */
;                           int   msglen    /* ebp + 12 */
;                          );
;
DevHlp32_InternalError proc near
    enter 0, 0
    mov ax, seg DATA16
    mov es, ax
    mov ax, offset DATA16:panic_buffer
    movzx edi, ax
    mov esi, [ebp + 8]
    mov ecx, [ebp + 12]
    and ecx, 511
    rep movsb

    mov edi, [ebp + 12]
    and edi, 511
    push ds
    pop es
    mov ax, seg DATA16
    mov ds, ax
    mov si, offset DATA16:panic_buffer
    mov dl, DevHlp_InternalError
    jmp far ptr thunk16$DevHlp32_InternalError
DevHlp32_InternalError endp

CODE32  ends

        end
