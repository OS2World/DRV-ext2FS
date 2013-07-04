
/*
 *@@sourcefile fs32_allocatepagespace.c:
 *      32-bit C handler called from 16-bit entry point
 *      in src\interface\fs_thunks.asm.
 */

/*
 *      Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
 *      Copyright (C) 2001 Ulrich M”ller.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file with this distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */


#ifdef __IBMC__
#pragma strings(readonly)
#endif


#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsd32.h>
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>

#include <linux\stat.h>

#include <os2\os2proto.h>
#include <os2\ifsdbg.h>
#include <os2\filefind.h>
#include <os2\errors.h>
#include <os2\log.h>
#include <os2\volume.h>
#include <os2\os2misc.h>
#include <os2\trace.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <linux\sched.h>

extern int swap_in_progress;

int do_allocatepagespace(struct sffsi32 *psffsi, union sffsd32 *psffsd, unsigned long ulSize)
{
    int rc = NO_ERROR;
    struct file *f;
    struct buffer_head *bh;
    unsigned long tmp;
    blk_t start, end, i;
    int err;


    if (trace_FS_ALLOCATEPAGESPACE)
    {
        kernel_printf("FS_ALLOCATEPAGESPACE pre-invocation - size=%lu", ulSize);
    }

    /*
     * Increments the swap flag
     */
    swap_in_progress++;

    /*
     * Gets the file structure from psffsd
     */
    if ((f = psffsd->f) != 0)
    {

        /*
         * Updates the I-node
         */
        tmp = f->f_inode->i_size;
        f->f_inode->i_size = ulSize;
        if (ulSize < tmp)
        {
            f->f_inode->i_op->truncate(f->f_inode);
        }
        else
        {
            start = tmp / f->f_inode->i_sb->s_blocksize;
            end = (ulSize + f->f_inode->i_sb->s_blocksize - 1) / f->f_inode->i_sb->s_blocksize;
            for (i = start; i < end; i++)
            {
                bh = ext2_getblk(f->f_inode, i, 1, __StackToFlat(&err));
                if (bh)
                {
                    bforget(bh);
                }
                else
                {
                    ulSize = i * f->f_inode->i_sb->s_blocksize;
                    f->f_inode->i_size = ulSize;
                    rc = map_err(err);
                    i = end + 1;
                }
            }
        }


        f->f_inode->i_mtime = CURRENT_TIME;
        f->f_inode->i_dirt = 1;

        /*
         * Updates the SFT
         */
        psffsi->sfi_size = ulSize;
        psffsi->sfi_tstamp |= ST_PWRITE;

        /*
         * Commits changes to disk
         */
        VFS_fsync(f);
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }

    /*
     * Decrements the swap flag
     */
    swap_in_progress--;

    if (trace_FS_ALLOCATEPAGESPACE)
    {
        kernel_printf("FS_ALLOCATEPAGESPACE post-invocation - rc = %d", rc);
    }
    return rc;
}

/*
 * struct fs32_allocatepagespace_parms {
 *     unsigned long ulWantContig;
 *     unsigned long ulSize;
 *     PTR16 psffsd;
 *     PTR16 psffsi;
 * };
 */
int FS32ENTRY fs32_allocatepagespace(struct fs32_allocatepagespace_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;

    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

    rc = do_allocatepagespace(psffsi, psffsd, parms->ulSize);

    return rc;
}
