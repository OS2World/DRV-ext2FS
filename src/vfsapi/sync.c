//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfsapi/sync.c,v 1.1 2001/05/09 17:48:09 umoeller Exp $
//

// Linux ext2 file system driver for OS/2 2.x and WARP - Allows OS/2 to
// access your Linux ext2fs partitions as normal drive letters.
// Copyright (C) 1995, 1996 Matthieu WILLM
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

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <os2\vfsapi.h>

/*
 *@@ vfs_sync:
 *      OS/2 implementation of the Unix sync() call.
 *      Commits all data to disk. Will ONLY work on
 *      ext2fs partitions.
 */

int VFSENTRY vfs_sync(void) {
    APIRET rc;
    ULONG  dataio = 0;
    ULONG  parmio = 0;

    return DosFSCtl(
                    NULL, 0, &dataio,
                    NULL, 0, &parmio,
                    EXT2_OS2_SYNC,  "ext2",
                    (HFILE)-1,  FSCTL_FSDNAME
                   );
}
