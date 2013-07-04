
/*
 *@@sourcefile fs32_chdir.c:
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
#include <os2\fsd32.h>
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>

#include <linux\stat.h>

#include <os2\os2proto.h>
#include <os2\ifsdbg.h>
#include <os2\filefind.h>
#include <os2\errors.h>
#include <os2\log.h>            /* Prototypes des fonctions de log.c                      */
#include <os2\volume.h>         /* Prototypes des fonctions de volume.c                   */
#include <os2\os2misc.h>
#include <os2\trace.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <linux\sched.h>

#define THISFILE FILE_FS_DIR_C

/*
 * struct fs32_chdir_parms {
 *     unsigned short      iCurDirEnd;
 *     PTR16               pDir;
 *     PTR16               pcdfsd;
 *     PTR16               pcdfsi;
 *     unsigned short      flag;
 * };
 */
int FS32ENTRY fs32_chdir(struct fs32_chdir_parms *parms)
{
    char *pDir;
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    int rc;
    struct file *p_file;
    struct super_block *sb;
    int valid;
    char *tmp;
    struct inode *base;

    switch (parms->flag)
    {
        case CD_EXPLICIT:
            pcdfsi = VDHQueryLin(parms->pcdfsi);
            pcdfsd = VDHQueryLin(parms->pcdfsd);
            pDir = VDHQueryLin(parms->pDir);

            if (trace_FS_CHDIR)
            {
                kernel_printf("FS_CHDIR( CD_EXPLICIT, %s )", pDir);
            }

            rc = ERROR_INVALID_PARAMETER;
            if (parms->iCurDirEnd != CURDIREND_INVALID)
            {
                tmp = pDir + parms->iCurDirEnd;
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
                    tmp = skip_drive(pDir);
                    base = sb->s_mounted;
                    if (base)
                    {
                        rc = NO_ERROR;
                    }
                }
            }

            if (rc == NO_ERROR)
            {
                if ((p_file = open_by_name(base, tmp, OPENMODE_READONLY)) == 0)
                {
                    rc = ERROR_PATH_NOT_FOUND;
                }
                else
                {
                    if (!S_ISDIR(p_file->f_inode->i_mode))
                    {
                        kernel_printf("FS_CHDIR( %s ) Not a directory", pDir);
                        vfs_close(p_file);
                        rc = ERROR_ACCESS_DENIED;
                    }
                    else
                    {
                        pcdfsd->u.is_valid = 1;
                        pcdfsd->u.p_file = p_file;
                        rc = NO_ERROR;
                    }
                }
            }
            break;

        case CD_VERIFY:
            pcdfsi = VDHQueryLin(parms->pcdfsi);
            pcdfsd = VDHQueryLin(parms->pcdfsd);

            if (trace_FS_CHDIR)
            {
                kernel_printf("FS_CHDIR : flag = CD_VERIFY hVPB=0x%04X", pcdfsi->cdi_hVPB);
            }

            //
            // Gets the superblock from pcdfsi
            //
            sb = getvolume(pcdfsi->cdi_hVPB);
            valid = 1;
            if ((p_file = open_by_name(sb->s_mounted, skip_drive(pcdfsi->cdi_curdir), OPENMODE_READONLY)) == 0)
            {
                valid = 0;
            }
            else
            {
                if (!S_ISDIR(p_file->f_inode->i_mode))
                {
                    vfs_close(p_file);
                    valid = 0;
                }

                if (pcdfsi->cdi_flags & CDI_ISVALID)
                {
                    vfs_close(pcdfsd->u.p_file);
                }
                if (valid)
                {
                    pcdfsd->u.is_valid = 1;
                    pcdfsd->u.p_file = p_file;
                    return NO_ERROR;
                }
                else
                {
                    vfs_close(p_file);
                    return ERROR_PATH_NOT_FOUND;
                }
            }
            break;

        case CD_FREE:
            pcdfsd = VDHQueryLin(parms->pcdfsd);

            if (trace_FS_CHDIR)
            {
                kernel_printf("FS_CHDIR( CD_FREE )");
            }


            if (pcdfsd->u.is_valid == 1)
            {
                pcdfsd->u.is_valid = 0;
                if ((rc = vfs_close(pcdfsd->u.p_file)) != NO_ERROR)
                {
                    fs_err(FUNC_FS_CHDIR, FUNC_CLOSE, rc, THISFILE, __LINE__);
                }
//                return rc;
            }
            else
            {
                rc = NO_ERROR;
            }
            break;

        default:
            fs_log("FS_CHDIR : invalid flag");
            rc = ERROR_INVALID_PARAMETER;

    }

    return rc;
}
