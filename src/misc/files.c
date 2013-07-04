//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/files.c,v 1.1 2001/05/09 17:49:50 umoeller Exp $
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
#include <os2.h>        // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\DevHlp32.h>
#include <os2\os2proto.h>
#include <os2\errors.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_proto.h>
#include <os2\log.h>         /* Prototypes des fonctions de log.c                      */
#include <os2\fnmatch.h>

//
// These routines will be eventually rewritten and splitted between ext2/namei.c
// (namei()), vfs/namei.c (dir_namei, open_namei, ....) and vfs/open.c (do_open ...)
//

struct file *_open_by_inode(struct super_block * p_volume, UINT32 ino_no, UINT32 openmode)
{
    struct file   *      p_file;
    int           rc;

#ifdef FS_TRACE
    kernel_printf("open_by_inode( %lu )", ino_no);
#endif
    /*******************************************************************/
    /*** Allocates a file descriptor                                 ***/
    /*******************************************************************/
    if ((p_file = get_empty_filp()) == 0) {
        fs_err(FUNC_OPEN_BY_INODE, FUNC_ALLOC_HFILE, 0, FILE_FILES_C, __LINE__);
        return 0;
    }
    /*******************************************************************/

    /*******************************************************************/
    /*** Gets the v-inode (if already exists) or allocates a new one ***/
    /*******************************************************************/
    if (ino_no == p_volume->s_mounted->i_ino) {
        p_file->f_inode = p_volume->s_mounted;
        p_volume->s_mounted->i_count ++;
    } else {
    if ((p_file->f_inode = iget(p_volume, ino_no)) == NULL) {
        fs_err(FUNC_OPEN_BY_INODE, FUNC_GET_VINODE, rc, FILE_FILES_C, __LINE__);
        put_filp(p_file);
        return 0;
    }
    }
    /*******************************************************************/


    p_file->f_pos            = 0;
    p_file->f_mode           = openmode;
    if (p_file->f_inode->i_op)
        p_file->f_op = p_file->f_inode->i_op->default_file_ops;

    return p_file;
}

int vfs_close(struct file *f)
{
    int           rc;

#ifdef FS_TRACE
    kernel_printf("close( %lu )", f->f_inode->i_ino);
#endif

    if (f->f_inode)
        iput(f->f_inode);

    if ((rc = put_filp(f)) != NO_ERROR) {
        fs_err(FUNC_CLOSE, FUNC_FREE_HFILE, rc, FILE_FILES_C, __LINE__);
        return rc;
    }

    return NO_ERROR;
}


struct file *open_by_name(struct inode *base, char *pathname, unsigned long openmode) {
    const char * basename;
    int namelen,error;
    struct inode * dir, *inode;
        struct file  * f;


        base->i_count ++;
    error = dir_namei(pathname,__StackToFlat(&namelen),__StackToFlat(&basename),base,__StackToFlat(&dir));
    if (error)
        return 0;
    dir->i_count++;     /* lookup eats the dir */
    error = lookup(dir,basename,namelen,__StackToFlat(&inode));
    if (error) {
        iput(dir);
        return 0;
    }
    error = follow_link(dir,inode,0,0,__StackToFlat(&inode));
    if (error)
        return 0;
        f = get_empty_filp();
        if (!f) {
            iput(inode);
            return 0;
        }
        f->f_inode          = inode;
        f->f_pos            = 0;
        f->f_mode           = openmode;
        if (f->f_inode->i_op)
            f->f_op = f->f_inode->i_op->default_file_ops;

        return f;
}
