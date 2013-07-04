//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfsapi/stat.c,v 1.1 2001/05/09 17:48:09 umoeller Exp $
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

#include <string.h>

#include <os2\vfsapi.h>

static struct fsctl_msg ext2_os2_magic   = {sizeof(magic_msg), magic_msg};


/*
 *@@ vfs_fstat:
 *      This is the ext2-os2 implementation of the Unix fstat()
 *      system call. It returns a "true" stat structure containing
 *      the "true" Unix file mode, the "true" file ownership... This
 *      stat structure is the same as the Linux stat structure
 *      defined in /usr/include/sys/stat.h. All the macros and
 *      definitions for file modes and file ownership are also the
 *      same as in Linux.
 */

int VFSENTRY vfs_fstat(int fd,              // in: valid OS/2 standard file handle (DosOpen)
                       struct vfs_stat *s)  // in/out: contains i-node information.
{
    APIRET rc;
    ULONG  dataio, parmio;
    short  err;
    struct fsctl_msg  sig;

    //
    // We first check that the file descriptor refers to a file located on a file system
    // managed by ext2-os2.ifs
    //
    err    = NO_ERROR;
    dataio = 0;
    parmio = sizeof(short);
    if ((rc = DosFSCtl(
                       &sig,  sizeof(struct fsctl_msg), &dataio,
                       &err,  sizeof(short), &parmio,
                       FSCTL_ERROR_INFO,  NULL,
                       (HFILE)fd,  FSCTL_HANDLE)) != NO_ERROR) {
        return rc;
    }
    if (memcmp(&sig, &ext2_os2_magic, sizeof(struct fsctl_msg)) != 0) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // We then ask ext2-os2.ifs to do the fstat() call
    //
    dataio = 0;
    parmio = 0;
    if ((rc = DosFSCtl(
                       s,  sizeof(struct vfs_stat), &dataio,
                       NULL,  0, &parmio,
                       EXT2_OS2_FSTAT,  NULL,
                       (HFILE)fd,  FSCTL_HANDLE)) != NO_ERROR) {
        return rc;
    }

    return 0;
}

/*
 *@@ vfs_stat:
 *      This is the ext2-os2 implementation of the Unix stat()
 *      system call. It returns a "true" stat structure containing
 *      the "true" Unix file mode, the "true" file ownership... This
 *      stat structure is the same as the Linux stat structure
 *      defined in /usr/include/sys/stat.h. All the macros and
 *      definitions for file modes and file ownership are also the
 *      same as in Linux.
 */

int VFSENTRY vfs_stat(const char *pathname,     // in: filename (as with DosOpen)
                      struct vfs_stat *s)       // out: i-node information
{
    APIRET rc;
    ULONG  dataio, parmio;
    short  err;
    struct fsctl_msg  sig;

    //
    // We first check that the file descriptor refers to a file located on a file system
    // managed by ext2-os2.ifs
    //
    err    = NO_ERROR;
    dataio = 0;
    parmio = sizeof(short);
    if ((rc = DosFSCtl(
                       &sig,  sizeof(struct fsctl_msg), &dataio,
                       &err,  sizeof(short), &parmio,
                       FSCTL_ERROR_INFO,  (PSZ)pathname,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }
    if (memcmp(&sig, &ext2_os2_magic, sizeof(struct fsctl_msg)) != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We then ask ext2-os2.ifs to do the stat() call
    //
    dataio = sizeof(struct vfs_stat);
    parmio = 0;
    if ((rc = DosFSCtl(
                       s,  sizeof(struct vfs_stat), &dataio,
                       NULL,  0, &parmio,
                       EXT2_OS2_STAT,  (PSZ)pathname,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }

    return 0;
}
