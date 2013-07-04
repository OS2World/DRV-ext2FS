
/*
 *@@sourcefile fs32_delete.c:
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
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>


/*
 * struct fs32_delete_parms {
 *     unsigned short      iCurDirEnd;
 *     PTR16               pFile;
 *     PTR16               pcdfsd;
 *     PTR16               pcdfsi;
 * };
 */
int FS32ENTRY fs32_delete(struct fs32_delete_parms *parms)
{
    char *pFile;
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    struct super_block *sb;
    int rc;
    char *tmp;
    struct inode *base;


    pcdfsi = VDHQueryLin(parms->pcdfsi);
    pFile = VDHQueryLin(parms->pFile);

    if (trace_FS_DELETE)
    {
        kernel_printf("FS_DELETE pre-invocation : %s (%d) %s %s %s", pFile, parms->iCurDirEnd, (pcdfsi->cdi_flags & CDI_ISROOT ? "CDI_ISROOT" : ""), (pcdfsi->cdi_flags & CDI_ISCURRENT ? "CDI_ISCURRENT" : ""), (pcdfsi->cdi_flags & CDI_ISVALID ? "CDI_ISVALID" : ""));
    }

    if (Read_Write)
    {
        rc = ERROR_INVALID_PARAMETER;

        if (parms->iCurDirEnd != CURDIREND_INVALID)
        {
            tmp = pFile + parms->iCurDirEnd;
            pcdfsd = VDHQueryLin(parms->pcdfsd);
            if ((pcdfsd->u.p_file) && (pcdfsd->u.p_file->f_magic == FILE_MAGIC))
            {
                base = pcdfsd->u.p_file->f_inode;
                if (base)
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
                tmp = skip_drive(pFile);
                base = sb->s_mounted;
                if (base)
                {
                    rc = NO_ERROR;
                }
            }
        }
        if (rc == NO_ERROR)
        {
            rc = sys_unlink(base, tmp);
            rc = map_err(rc);   // rc is a Linux error code (from linux/errno.h)

        }

    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }

    if (trace_FS_DELETE)
    {
        kernel_printf("FS_DELETE post-invocation : %s, rc = %d", pFile, rc);
    }

    return rc;
}
