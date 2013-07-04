
/*
 *@@sourcefile fs32_commit.c:
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

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <os2\fsd32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <linux\fs_proto.h>

/*
 * struct fs32_commit_parms {
 *     PTR16 psffsd;
 *     PTR16 psffsi;
 *     unsigned short IOflag;
 *     unsigned short type;
 * };
 */
int FS32ENTRY fs32_commit(struct fs32_commit_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;


    if (trace_FS_COMMIT)
    {
        kernel_printf("FS_COMMIT pre-invocation : type = %d", parms->type);
    }

    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

    if (Read_Write)
    {
        if (psffsd->f)
        {
#if 0
            //
            // Do the time stamping stuff - We commit the time stamp to disk if both
            // ST_Sxxx and ST_Pxxx flags are set
            //
            if (psffsi->sfi_tstamp & ST_PCREAT)
            {
                psffsd->f->f_inode->i_ctime = date_dos2unix(psffsi->sfi_ctime, psffsi->sfi_cdate);
                psffsd->f->f_inode->i_dirt = 1;
            }
            if (psffsi->sfi_tstamp & ST_PREAD)
            {
                psffsd->f->f_inode->i_atime = date_dos2unix(psffsi->sfi_atime, psffsi->sfi_adate);
                psffsd->f->f_inode->i_dirt = 1;
            }
            if (psffsi->sfi_tstamp & ST_PWRITE)
            {
                psffsd->f->f_inode->i_mtime = date_dos2unix(psffsi->sfi_mtime, psffsi->sfi_mdate);
                psffsd->f->f_inode->i_dirt = 1;
            }
#endif
            if ((rc = VFS_fsync(psffsd->f)) == NO_ERROR)
            {
                psffsi->sfi_tstamp = 0;
            }
            else
            {
                kernel_printf("FS_COMMIT() - VFS_fsync returned %d", rc);
            }
        }
        else
        {
            kernel_printf("FS_COMMIT() - psffsd->f is NULL");
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }

    if (trace_FS_COMMIT)
    {
        kernel_printf("FS_COMMIT post-invocation : rc = %d", rc);
    }
    return rc;
}
