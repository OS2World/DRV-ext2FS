
/*
 *@@sourcefile fs32_newsize.c:
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
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <os2\fsd32.h>
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>

/*
 * struct fs32_newsize_parms {
 *     unsigned short IOflag;
 *     unsigned long len;
 *     PTR16          psffsd;
 *     PTR16          psffsi;
 * };
 */
int FS32ENTRY fs32_newsize(struct fs32_newsize_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;
    unsigned long tmp;


    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

    if ((psffsd->f) && (psffsd->f->f_magic == FILE_MAGIC))
    {
        if (trace_FS_NEWSIZE)
        {
            kernel_printf("FS_NEWSIZE(ino = %lu, sz = %lu)", psffsd->f->f_inode->i_ino, parms->len);
        }

        if (Read_Write)
        {
            if (psffsd->f->f_inode->i_ino != INODE_DASD)
            {

                /*
                 * Updates the I-node
                 */
                tmp = psffsd->f->f_inode->i_size;
                psffsd->f->f_inode->i_size = parms->len;
                if (parms->len < tmp)
                    psffsd->f->f_inode->i_op->truncate(psffsd->f->f_inode);
                psffsd->f->f_inode->i_mtime = CURRENT_TIME;
                psffsd->f->f_inode->i_dirt = 1;

                /*
                 * Updates the SFT
                 */
                psffsi->sfi_size = parms->len;
                psffsi->sfi_tstamp |= ST_PWRITE;

                rc = NO_ERROR;
            }
            else
            {
                kernel_printf("FS_NEWSIZE() called on a direct access device handle !");
                rc = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            kernel_printf("ERROR ! FS_NEWSIZE called and write access not enabled");
            rc = ERROR_WRITE_PROTECT;
        }
    }
    else
    {
        kernel_printf("FS_NEWSIZE() - p_file = NULL");
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}
