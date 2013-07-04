;
; $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/strategy2.asm,v 1.1 2001/05/09 17:48:08 umoeller Exp $
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
        public thunk16$send_RLH_to_driver

thunk16$send_RLH_to_driver proc far
        call dword ptr [ebp + 12]
        jmp far ptr FLAT:thunk16$send_RLH_to_driver
;        jmp32 thunk32$send_RLH_to_driver
thunk16$send_RLH_to_driver endp

CODE16 ends

DATA32 segment
    public stub_request_completed
    public stub_request_list_completed
    public stub_pager_RLE_completed
    public stub_pager_RLH_completed

stub_request_completed      dw offset CODE16:__stub_request_completed
                dw seg    CODE16
stub_request_list_completed dw offset CODE16:__stub_request_list_completed
                dw seg    CODE16
stub_pager_RLE_completed    dw offset CODE16:__stub_pager_RLE_completed
                dw seg    CODE16
stub_pager_RLH_completed    dw offset CODE16:__stub_pager_RLH_completed
                dw seg    CODE16

DATA32 ends
;FIXED_CODE16  segment
;ASSUME CS:FIXED_CODE16
CODE16  segment
ASSUME CS:CODE16
        public __stub_request_completed
        public __stub_request_list_completed

__stub_request_completed proc far
        mov ecx, es:[bx + 4]        ; RH_Head_Offset
        sub bx, cx          ; es:bx now points to the RLH
        mov eax, es:[bx - 4]        ; lin addr of request list
        add eax, ecx            ; lin addr of request
        push eax
        mov ax, Dos32FlatDS
        mov ds, ax
        mov es, ax
;        call32 REQUEST_COMPLETED
        call far ptr FLAT:REQUEST_COMPLETED
        neg ax
        retf
__stub_request_completed endp

__stub_pager_RLE_completed proc far
        mov ecx, es:[bx + 4]        ; RH_Head_Offset
        sub bx, cx          ; es:bx now points to the RLH
        mov eax, es:[bx - 4]        ; lin addr of request list
        add eax, ecx            ; lin addr of request
        push eax
        mov ax, Dos32FlatDS
        mov ds, ax
        mov es, ax
;        call32 PAGER_REQUEST_COMPLETED
        call far ptr FLAT:PAGER_REQUEST_COMPLETED
        neg ax
        retf
__stub_pager_RLE_completed endp

__stub_request_list_completed proc far
        mov eax, es:[bx - 4]        ; lin addr of request list
        push eax
        mov ax, Dos32FlatDS
        mov ds, ax
        mov es, ax
        ;call32 REQUEST_LIST_COMPLETED
        call far ptr FLAT:REQUEST_LIST_COMPLETED
        neg ax
        retf
__stub_request_list_completed endp

__stub_pager_RLH_completed proc far
        mov eax, es:[bx - 4]        ; lin addr of request list
        push eax
        mov ax, Dos32FlatDS
        mov ds, ax
        mov es, ax
        ;call32 PAGER_REQUEST_LIST_COMPLETED
        call far ptr FLAT:PAGER_REQUEST_LIST_COMPLETED
        neg ax
        retf
__stub_pager_RLH_completed endp

;FIXED_CODE16  ends
CODE16  ends

CODE32  segment
ASSUME CS:CODE32
        extrn PAGER_REQUEST_COMPLETED  : far
        extrn PAGER_REQUEST_LIST_COMPLETED  : far
        extrn REQUEST_COMPLETED       : far
        extrn REQUEST_LIST_COMPLETED  : far

        ; public thunk32$send_RLH_to_driver
        public         send_RLH_to_driver

; extern void _System  send_RLH_to_driver(
;                                         PTR16 pRLH,   /* ebp + 8  */
;                                         PTR16 strat2  /* ebp + 12 */
;                                        );

send_RLH_to_driver proc near
    enter 0, 0
        push ds
        push es
        push ebx
        push esi
        push edi

        les bx, dword ptr [ebp + 8]     ; pRLH
        jmp far ptr thunk16$send_RLH_to_driver
thunk32$send_RLH_to_driver label far:

        pop edi
        pop esi
        pop ebx
        pop es
        pop ds
        leave
        ret
send_RLH_to_driver endp

CODE32  ends

        end
