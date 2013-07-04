//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfsapi/ioctl.c,v 1.1 2001/05/09 17:48:09 umoeller Exp $
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
#include <linux\errno.h>

static struct fsctl_msg ext2_os2_magic   = {sizeof(magic_msg), magic_msg};


int VFSENTRY vfs_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg, int size, int *err_no) {
    APIRET rc;
    ULONG  dataio, parmio;
    short  err;
    struct fsctl_msg  sig;

    *err_no = EPERM;

    //
    // We first check that the file descriptor refers to a file located on a file system
    // managed by ext2-os2.ifs
    //
    err    = NO_ERROR;
    dataio = 0;
    parmio = 0;
    if ((rc = DosFSCtl(
                       &sig,  sizeof(struct fsctl_msg), &dataio,
                       &err,  sizeof(short), &parmio,
                       FSCTL_ERROR_INFO,  NULL,
                       (HFILE)fd,  FSCTL_HANDLE)) != NO_ERROR) {
        return -1;
    }
    if (memcmp(&sig, &ext2_os2_magic, sizeof(struct fsctl_msg)) != 0) {
        return -1;
    }


    //
    // We issue the ioctl call
    //
    dataio = 0;
    if ((rc = DosFSCtl(
                       (void *)arg, size, &dataio,
                       0, 0, 0,
                       cmd, NULL,
                       (HFILE)fd,  FSCTL_HANDLE)) != NO_ERROR) {
        return -1;
    }

    *err_no = 0;
    return 0;
}
