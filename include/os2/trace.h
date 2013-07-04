//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/trace.h,v 1.1 2001/05/09 17:43:43 umoeller Exp $
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

#ifndef __trace_h
#define __trace_h

#ifndef MINIFSD
extern char trace_FS_ALLOCATEPAGESPACE;
extern char trace_FS_ATTACH;
extern char trace_FS_CANCELLOCKREQUEST;
extern char trace_FS_CHDIR;
extern char trace_FS_CHGFILEPTR;
extern char trace_FS_CLOSE;
extern char trace_FS_COMMIT;
extern char trace_FS_COPY;
extern char trace_FS_DELETE;
extern char trace_FS_DOPAGEIO;
extern char trace_FS_EXIT;
extern char trace_FS_FILEATTRIBUTE;
extern char trace_FS_FILEINFO;
extern char trace_FS_FILEIO;
extern char trace_FS_FILELOCKS;
extern char trace_FS_FINDCLOSE;
extern char trace_FS_FINDFIRST;
extern char trace_FS_FINDFROMNAME;
extern char trace_FS_FINDNEXT;
extern char trace_FS_FINDNOTIFYCLOSE;
extern char trace_FS_FINDNOTIFYFIRST;
extern char trace_FS_FINDNOTIFYNEXT;
extern char trace_FS_FLUSHBUF;
// extern char trace_FS_FSCTL;
extern char trace_FS_FSINFO;
// extern char trace_FS_INIT;
extern char trace_FS_IOCTL;
extern char trace_FS_MKDIR;
extern char trace_FS_MOUNT;
extern char trace_FS_MOVE;
extern char trace_FS_NEWSIZE;
extern char trace_FS_NMPIPE;
extern char trace_FS_OPENCREATE;
extern char trace_FS_OPENPAGEFILE;
extern char trace_FS_PATHINFO;
extern char trace_FS_PROCESSNAME;
extern char trace_FS_READ;
extern char trace_FS_RMDIR;
extern char trace_FS_SETSWAP;
extern char trace_FS_SHUTDOWN;
// extern char trace_FS_VERIFYUNCNAME;
extern char trace_FS_WRITE;

extern void check_trace(const char *tmp);

#endif

#endif
