;
; $Header: e:\\netlabs.cvs\\ext2fs/src/interface/start.asm,v 1.1 2001/05/09 17:49:49 umoeller Exp $
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

    include segdef.inc

; *********************************************************
;
;       CODE16
;
; *********************************************************

CODE16 segment
    public code16_start
    code16_start label byte
CODE16 ends

; *********************************************************
;
;       CODE32
;
; *********************************************************

CODE32 segment
    public code32_start
    code32_start label byte
CODE32 ends

; *********************************************************
;
;       DATA16
;
; *********************************************************

DATA16 segment
    public data16_start
    data16_start label byte
DATA16 ends

; *********************************************************
;
;       CONST32_RO
;
; *********************************************************

CONST32_RO segment
    public const32_ro_start
    const32_ro_start label byte
CONST32_RO ends

; *********************************************************
;
;       DGROUP
;
; *********************************************************

BSS32 segment
    public bss32_start
    bss32_start label byte
BSS32 ends

DATA32 segment
    public data32_start
    data32_start label byte

    public dgroup_start
    dgroup_start label byte

    public TKSSBase
    TKSSBase dword 0

    public DevHelp2
    DevHelp2 dword 0

DATA32 ends

    end

