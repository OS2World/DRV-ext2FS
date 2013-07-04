//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfsapi/link.c,v 1.1 2001/05/09 17:48:09 umoeller Exp $
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
 *@@ vfs_link:
 *      Creates a hard link to an existing file.
 */

int VFSENTRY vfs_link(const char *src,      // in: source file name to be linked
                      const char *dst)      // in: file name for the new link
{
    APIRET rc;
    ULONG  dataio, parmio;
    short  err;
    struct fsctl_msg  sig;
    char dst_norm[CCHMAXPATH];

    //
    // We check that the source path refers to a file located on a file system
    // managed by ext2-os2.ifs
    //
    err    = NO_ERROR;
    dataio = 0;
    parmio = sizeof(short);
    if ((rc = DosFSCtl(
                       &sig,  sizeof(struct fsctl_msg), &dataio,
                       &err,  sizeof(short), &parmio,
                       FSCTL_ERROR_INFO,  (PSZ)src,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }
    if (memcmp(&sig, &ext2_os2_magic, sizeof(struct fsctl_msg)) != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We check that the destination path refers to a file located on a file system
    // managed by ext2-os2.ifs
    //
    err    = NO_ERROR;
    dataio = 0;
    parmio = sizeof(short);
    if ((rc = DosFSCtl(
                       &sig,  sizeof(struct fsctl_msg), &dataio,
                       &err,  sizeof(short), &parmio,
                       FSCTL_ERROR_INFO,  (PSZ)dst,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }
    if (memcmp(&sig, &ext2_os2_magic, sizeof(struct fsctl_msg)) != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We get the normalized destination path name
    //
    dataio = CCHMAXPATH;
    if ((rc = DosFSCtl(
                       dst_norm,  CCHMAXPATH, &dataio,
                       0, 0, 0,
                       EXT2_OS2_NORMALIZE_PATH,  (PSZ)dst,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }

    //
    // We then ask ext2-os2.ifs to do the link() call
    //
    dataio = strlen(dst_norm) + 1;
    if ((rc = DosFSCtl(
                       dst_norm,  dataio, &dataio,
                       0, 0, 0,
                       EXT2_OS2_LINK,  (PSZ)src,
                       (HFILE)(-1),  FSCTL_PATHNAME)) != NO_ERROR) {
        return rc;
    }

    return 0;
}
