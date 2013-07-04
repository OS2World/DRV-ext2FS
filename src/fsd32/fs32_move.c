
/*
 *@@sourcefile fs32_move.c:
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
 * struct fs32_move_parms {
 *     unsigned short flag;
 *     unsigned short iDstCurDirEnd;
 *     PTR16          pDst;
 *     unsigned short iSrcCurDirEnd;
 *     PTR16          pSrc;
 *     PTR16          pcdfsd;
 *     PTR16          pcdfsi;
 * };
 */
int FS32ENTRY fs32_move(struct fs32_move_parms *parms)
{
    char *pSrc, *srcname;
    char *pDst, *dstname;
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    int rc;
    struct inode *srcbase, *dstbase;
    struct super_block *sb = 0;

    pcdfsi = VDHQueryLin(parms->pcdfsi);
    pcdfsd = VDHQueryLin(parms->pcdfsd);
    pSrc = VDHQueryLin(parms->pSrc);
    pDst = VDHQueryLin(parms->pDst);

    if (trace_FS_MOVE)
    {
        kernel_printf("FS_MOVE pre-invocation : %s -> %s", pSrc, pDst);
    }

    if (Read_Write)
    {
        rc = ERROR_INVALID_PARAMETER;
        if (parms->iSrcCurDirEnd != CURDIREND_INVALID)
        {
            srcname = pSrc + parms->iSrcCurDirEnd;
            if ((pcdfsd->u.p_file) && (pcdfsd->u.p_file->f_magic == FILE_MAGIC))
            {
                srcbase = pcdfsd->u.p_file->f_inode;
                if (srcbase)
                {
                    rc = NO_ERROR;
                }
            }
        }
        else
        {
            sb = getvolume(pcdfsi->cdi_hVPB);
            if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
            {
                srcname = skip_drive(pSrc);
                srcbase = sb->s_mounted;
                if (srcbase)
                {
                    rc = NO_ERROR;
                }
            }
        }

        if (rc == NO_ERROR)
        {
            rc = ERROR_INVALID_PARAMETER;
            if (parms->iDstCurDirEnd != CURDIREND_INVALID)
            {
                dstname = pDst + parms->iDstCurDirEnd;
                if ((pcdfsd->u.p_file) && (pcdfsd->u.p_file->f_magic == FILE_MAGIC))
                {
                    dstbase = pcdfsd->u.p_file->f_inode;
                    if (dstbase)
                    {
                        rc = NO_ERROR;
                    }
                }
            }
            else
            {
                if (!sb)
                    sb = getvolume(pcdfsi->cdi_hVPB);
                if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
                {
                    dstname = skip_drive(pDst);
                    dstbase = sb->s_mounted;
                    if (dstbase)
                    {
                        rc = NO_ERROR;
                    }
                }
            }

            if (rc == NO_ERROR)
            {
                srcbase->i_count++;
                dstbase->i_count++;
                rc = sys_rename(srcbase, dstbase, srcname, dstname);
                rc = map_err(rc);
            }

        }
    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }

    if (trace_FS_MOVE)
    {
        kernel_printf("FS_MOVE post-invocation (%s -> %s) rc = %d", pSrc, pDst, rc);
    }

    return rc;
}
