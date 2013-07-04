//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/read_write.c,v 1.1 2001/05/09 17:48:08 umoeller Exp $
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

#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <os2\types.h>
#ifndef MINIFSD
#include <os2\devhlp32.h>
#endif
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\stat.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>


int VFS_readdir(struct file *file, struct dirent *dirent) {

    /**********************************************************************/
    /*** Tests if it's really a directory                               ***/
    /**********************************************************************/
    if (!S_ISDIR(file->f_inode->i_mode)) {
        return ERROR_ACCESS_DENIED;
    }
    /**********************************************************************/

    return (file->f_op->readdir(file->f_inode, file,
                         dirent, 1) > 0 ? NO_ERROR : ERROR_HANDLE_EOF);
}



int VFS_read(struct file *file, char *buf, loff_t len, unsigned long *pLen) {
    int  err;
    long read;

    read = file->f_op->read(file->f_inode, file, buf, len);
    if (read >= 0) {
        *pLen = (UINT32)read;
        err   = NO_ERROR;
    } else {
#ifdef FS_TRACE
        kernel_printf("ext2_file_read returned %d", err);
#endif
        *pLen = 0;
        err   = map_err((int)read);
    }
    return err;
}

/*
 * The following routines are meaningless for a mini FSD
 */
#ifndef MINIFSD
int VFS_write(struct file *file, char *buf, loff_t len, unsigned long *pLen) {
    int  err;
    long written;


    written = file->f_op->write (file->f_inode, file, buf, (long)len);
    if (written < 0) {
#ifdef FS_TRACE
        kernel_printf("ext2_file_write returned %lu", written);
#endif
        err   = map_err((int)written);
        *pLen = 0;
    } else {
        err   = NO_ERROR;
        *pLen = written;
    } /* endif */
    return err;
}
#endif /* #ifndef MINIFSD */
