;
; $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/minifsd.asm,v 1.1 2001/05/09 17:48:07 umoeller Exp $
;

; 32 bits Linux ext2 file system driver for OS/2 WARP - Allows OS/2 to
; access your Linux ext2fs partitions as normal drive letters.
; Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

        .386p

        INCL_DOSERRORS equ 1
        include bseerr.inc

        include segdef.inc
        include r0thunk.inc

CODE16  segment
ASSUME CS:CODE16
        public thunk16$remove_from_list_16

thunk16$remove_from_list_16:
        call dword ptr [bp + 8]
        jmp far ptr FLAT:thunk16$remove_from_list_16
;        jmp32 thunk32$remove_from_list_16

CODE16 ends


CODE32  segment
ASSUME CS:CODE32, DS:FLAT

        ;public thunk32$remove_from_list_16
        public         remove_from_list_16

; extern void _System remove_from_list_16(
;                                         PTR16 remove_from_list,    /* ebp + 8  */
;                                         PTR16 f,                   /* ebp + 12 */
;                                         PTR16 list_head,           /* ebp + 16 */
;                                         PTR16 counter              /* ebp + 20 */
;                                        );
remove_from_list_16 proc near
    enter 0, 0
        push ds
        push es
        push ebx
        push esi
        push edi

        mov eax, [ebp + 20]
        push eax
        mov eax, [ebp + 16]
        push eax
        mov eax, [ebp + 12]
        push eax
;        mov eax, [ebp + 8]
        jmp far ptr thunk16$remove_from_list_16
thunk32$remove_from_list_16 label far:
        add esp, 12

        pop edi
        pop esi
        pop ebx
        pop es
        pop ds
        leave
        ret
remove_from_list_16 endp

CODE32  ends

        end
