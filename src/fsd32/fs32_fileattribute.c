
/*
 *@@sourcefile fs32_fileattribute.c:
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
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>

#define FILE_NONFAT     0x0040  // File is non 8.3 compliant

/*
 * struct fs32_fileattribute_parms {
 *     PTR16           pAttr;
 *     unsigned short  iCurDirEnd;
 *     PTR16           pName;
 *     PTR16           pcdfsd;
 *     PTR16           pcdfsi;
 *     unsigned short  flag;
 * };
 */
int FS32ENTRY fs32_fileattribute(struct fs32_fileattribute_parms *parms)
{
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    unsigned short *pAttr;
    char *pName;
    int rc;
    struct super_block *sb;
    struct file *p_file;
    char component[CCHMAXPATH];
    UINT32 DOSmode;
    struct inode *base;
    char *tmp;

    pcdfsi = VDHQueryLin(parms->pcdfsi);
    pcdfsd = VDHQueryLin(parms->pcdfsd);
    pName = VDHQueryLin(parms->pName);
    pAttr = VDHQueryLin(parms->pAttr);

    if (trace_FS_FILEATTRIBUTE)
    {
        kernel_printf("FS_FILEATTRIBUTE pre-invocation : flag = %d, name = %s", parms->flag, pName);
    }

    if ((parms->flag == FA_RETRIEVE) ||
        (parms->flag == FA_SET))
    {

        if ((parms->flag == FA_RETRIEVE) || (Read_Write))
        {
            rc = ERROR_INVALID_PARAMETER;
            if (parms->iCurDirEnd != CURDIREND_INVALID)
            {
                tmp = pName + parms->iCurDirEnd;
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
                    tmp = skip_drive(pName);
                    base = sb->s_mounted;
                    if (base)
                    {
                        rc = NO_ERROR;
                    }
                }
            }

            if (rc == NO_ERROR)
            {

                if ((p_file = open_by_name(base, tmp, OPENMODE_READONLY)) != 0)
                {

                    if (parms->flag == FA_RETRIEVE)
                    {
                        ExtractName(pName, __StackToFlat(component));
                        *pAttr = Linux_To_DOS_Attrs(p_file->f_inode, __StackToFlat(component)) & ~FILE_NONFAT;  /* @@@ NONFAT bit cleared (cf bug reported by Martin Kneissl - martin@paule.tng.oche.de) ) */
                    }

                    if (parms->flag == FA_SET)
                    {
                        DOS_To_Linux_Attrs(p_file->f_inode, *pAttr);
                    }

                    if ((rc = vfs_close(p_file)) == NO_ERROR)
                    {
                        /*
                         * Nothing else to do
                         */
                    }
                    else
                    {
                        fs_err(FUNC_FS_CLOSE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
                    }           /* vfs_close failed */
                }
                else
                {
                    if (trace_FS_FILEATTRIBUTE)
                    {
                        fs_err(FUNC_FS_FILEATTRIBUTE, FUNC_OPEN_BY_NAME, ERROR_OPEN_FAILED, FILE_TEST_C, __LINE__);
                    }
                    rc = ERROR_FILE_NOT_FOUND;
                }               /* open_by_name failed */
            }
        }
        else
        {
            rc = ERROR_WRITE_PROTECT;
        }                       /* read only mode */
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }                           /* flag invalid */

    if (trace_FS_FILEATTRIBUTE)
    {
        kernel_printf("FS_FILEATTRIBUTE post-invocation : rc = %d, flag = %d, name = %s", rc, parms->flag, pName);
    }

    return rc;
}
