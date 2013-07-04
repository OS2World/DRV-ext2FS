//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/trace.c,v 1.1 2001/05/09 17:49:51 umoeller Exp $
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

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#include <string.h>

#include <os2\trace.h>

char trace_FS_ALLOCATEPAGESPACE    = 0;
char trace_FS_ATTACH               = 0;
char trace_FS_CANCELLOCKREQUEST    = 0;
char trace_FS_CHDIR                = 0;
char trace_FS_CHGFILEPTR           = 0;
char trace_FS_CLOSE                = 0;
char trace_FS_COMMIT               = 0;
char trace_FS_COPY                 = 0;
char trace_FS_DELETE               = 0;
char trace_FS_DOPAGEIO             = 0;
char trace_FS_EXIT                 = 0;
char trace_FS_FILEATTRIBUTE        = 0;
char trace_FS_FILEINFO             = 0;
char trace_FS_FILEIO               = 0;
char trace_FS_FILELOCKS            = 0;
char trace_FS_FINDCLOSE            = 0;
char trace_FS_FINDFIRST            = 0;
char trace_FS_FINDFROMNAME         = 0;
char trace_FS_FINDNEXT             = 0;
char trace_FS_FINDNOTIFYCLOSE      = 0;
char trace_FS_FINDNOTIFYFIRST      = 0;
char trace_FS_FINDNOTIFYNEXT       = 0;
char trace_FS_FLUSHBUF             = 0;
// char trace_FS_FSCTL                = 0;
char trace_FS_FSINFO               = 0;
// char trace_FS_INIT                 = 0;
char trace_FS_IOCTL                = 0;
char trace_FS_MKDIR                = 0;
char trace_FS_MOUNT                = 0;
char trace_FS_MOVE                 = 0;
char trace_FS_NEWSIZE              = 0;
char trace_FS_NMPIPE               = 0;
char trace_FS_OPENCREATE           = 0;
char trace_FS_OPENPAGEFILE         = 0;
char trace_FS_PATHINFO             = 0;
char trace_FS_PROCESSNAME          = 0;
char trace_FS_READ                 = 0;
char trace_FS_RMDIR                = 0;
char trace_FS_SETSWAP              = 0;
char trace_FS_SHUTDOWN             = 0;
// char trace_FS_VERIFYUNCNAME        = 0;
char trace_FS_WRITE                = 0;


#define CHECK_TRACE(buf, func)                                                      \
                  if (strncmp(buf, #func, sizeof(#func) - 1) == 0) {                \
                      trace_##func = 1;                                             \
                      continue;                                                     \
                  }

/*
 *@@ check_trace:
 *      checks on the IFS command line for entry points to trace.
 */

void check_trace(const char *tmp)
{
    do {
        //
        // IFS entry points to trace.
        //
        CHECK_TRACE(tmp, FS_ALLOCATEPAGESPACE);
        CHECK_TRACE(tmp, FS_ATTACH);
        CHECK_TRACE(tmp, FS_CANCELLOCKREQUEST);
        CHECK_TRACE(tmp, FS_CHDIR);
        CHECK_TRACE(tmp, FS_CHGFILEPTR);
        CHECK_TRACE(tmp, FS_CLOSE);
        CHECK_TRACE(tmp, FS_COMMIT);
        CHECK_TRACE(tmp, FS_COPY);
        CHECK_TRACE(tmp, FS_DELETE);
        CHECK_TRACE(tmp, FS_DOPAGEIO);
        CHECK_TRACE(tmp, FS_EXIT);
        CHECK_TRACE(tmp, FS_FILEATTRIBUTE);
        CHECK_TRACE(tmp, FS_FILEINFO);
        CHECK_TRACE(tmp, FS_FILEIO);
        CHECK_TRACE(tmp, FS_FILELOCKS);
        CHECK_TRACE(tmp, FS_FINDCLOSE);
        CHECK_TRACE(tmp, FS_FINDFIRST);
        CHECK_TRACE(tmp, FS_FINDFROMNAME);
        CHECK_TRACE(tmp, FS_FINDNEXT);
        CHECK_TRACE(tmp, FS_FINDNOTIFYCLOSE);
        CHECK_TRACE(tmp, FS_FINDNOTIFYFIRST);
        CHECK_TRACE(tmp, FS_FINDNOTIFYNEXT);
        CHECK_TRACE(tmp, FS_FLUSHBUF);
//         CHECK_TRACE(tmp, FS_FSCTL);
        CHECK_TRACE(tmp, FS_FSINFO);
//         CHECK_TRACE(tmp, FS_INIT);
        CHECK_TRACE(tmp, FS_IOCTL);
        CHECK_TRACE(tmp, FS_MKDIR);
        CHECK_TRACE(tmp, FS_MOUNT);
        CHECK_TRACE(tmp, FS_MOVE);
        CHECK_TRACE(tmp, FS_NEWSIZE);
        CHECK_TRACE(tmp, FS_NMPIPE);
        CHECK_TRACE(tmp, FS_OPENCREATE);
        CHECK_TRACE(tmp, FS_OPENPAGEFILE);
        CHECK_TRACE(tmp, FS_PATHINFO);
        CHECK_TRACE(tmp, FS_PROCESSNAME);
        CHECK_TRACE(tmp, FS_READ);
        CHECK_TRACE(tmp, FS_RMDIR);
        CHECK_TRACE(tmp, FS_SETSWAP);
        CHECK_TRACE(tmp, FS_SHUTDOWN);
//         CHECK_TRACE(tmp, FS_VERIFYUNCNAME);
        CHECK_TRACE(tmp, FS_WRITE);
    } while (0);
}
