//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/log.h,v 1.1 2001/05/09 17:43:41 umoeller Exp $
//

// 32 bits Linux ext2 file system driver for OS/2 WARP - Allows OS/2 to
// access your Linux ext2fs partitions as normal drive letters.
// Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef __log_h
#define __log_h

#include <os2\types.h>

/*
 * circular log variables
 */
#define BUFMSGSIZ 4096
extern char    BufMsg[BUFMSGSIZ];
extern UINT16  BufPtr;
extern UINT16  BufOpen;
extern UINT32  BufSem;

int fs_err(UINT32 infunction, UINT32 errfunction, int retcode, UINT32 sourcefile, UINT32 sourceline);
int fs_log(char *text);

#define OUTPUT_COM1 0x3F8
#define OUTPUT_COM2 0x2F8

extern char debug_com;             // output debug info to COM port
extern int  debug_port;            // base I/O address of COM port

#ifdef MINIFSD
extern int vsprintf(const char *fmt, ...);
#endif

#endif /* __log_h */
