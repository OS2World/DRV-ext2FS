
/*
 *@@sourcefile fs32_findfirst.c:
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
#include <linux\errno.h>
#include <os2\filefind.h>


/*
 * struct fs32_findfirst_parms {
 *     unsigned short  flags;
 *     unsigned short  level;
 *     PTR16           pcMatch;
 *     unsigned short  cbData;
 *     PTR16           pData;
 *     PTR16           pfsfsd;
 *     PTR16           pfsfsi;
 *     unsigned short  attr;
 *     unsigned short  iCurDirEnd;
 *     PTR16           pName;
 *     PTR16           pcdfsd;
 *     PTR16           pcdfsi;
 * };
 */
int FS32ENTRY fs32_findfirst(struct fs32_findfirst_parms *parms)
{
    struct fsfsi32 *pfsfsi;
    struct fsfsd32 *pfsfsd;
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    char *pName;
    char *pData;
    unsigned short *pcMatch;
    int rc;
    int rc2;
    struct file *f;
    struct super_block *sb;
    char lock[12];
    int caseRetensive;
    ULONG PgCount;

    char *tmp, *tmp2;
    char c;

    struct inode *base;
    const char *basename;
    int namelen, error;
    struct inode *dir;

    pfsfsi = VDHQueryLin(parms->pfsfsi);
    pfsfsd = VDHQueryLin(parms->pfsfsd);
    pcdfsi = VDHQueryLin(parms->pcdfsi);
    pcdfsd = VDHQueryLin(parms->pcdfsd);
    pName = VDHQueryLin(parms->pName);
    pcMatch = VDHQueryLin(parms->pcMatch);

    if (trace_FS_FINDFIRST)
    {
        kernel_printf("FS_FINDFIRST pre-invocation : %s , match = %u , level = %u, flag = %u)", pName, *pcMatch, parms->level, parms->flags);
    }

    rc = ERROR_INVALID_PARAMETER;

    /*
     * Checks that flags is valid.
     */
    if ((parms->flags == FF_NOPOS) ||
        (parms->flags == FF_GETPOS))
    {

        /*
         * Checks that level is valid.
         */
        if ((parms->level == FIL_STANDARD) ||
            (parms->level == FIL_QUERYEASIZE) ||
            (parms->level == FIL_QUERYEASFROMLIST))
        {

            /*
             * Checks that *pcMatch is not 0
             */
            if (*pcMatch)
            {
                /*
                 * Thunks user data buffer
                 */
                if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                {
                    /*
                     * Saves the file name to search into pfsfsd
                     */
                    if ((rc = DevHlp32_VMAlloc(CCHMAXPATH, VMDHA_NOPHYSADDR, VMDHA_SWAP, (void **)(&(((p_hfind) pfsfsd)->pName)))) == NO_ERROR)
                    {

                        /*
                         * Adds MAY_HAVE_READONLY to the attributes to search for (cf doc : this bit is never passed to the IFS)
                         */
                        ((p_hfind) pfsfsd)->attr = parms->attr | FILE_READONLY;

                        /*
                         * Extracts the name of search.
                         */
                        ExtractName(pName, ((p_hfind) pfsfsd)->pName);

                        /*
                         * Opens the directory of search.
                         */
                        rc = ERROR_INVALID_PARAMETER;
                        if (parms->iCurDirEnd != CURDIREND_INVALID)
                        {
                            /*
                             * The current directory is part of pName, so use it as the base of search.
                             */
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
                            /*
                             * The current directory is not relevant, so use the root I-node as the base of search.
                             */
                            sb = getvolume(pfsfsi->fsi_hVPB);
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
                        /*
                         * Now if no error, base contains the I-node to start from, and tmp the
                         * name of search, starting from that directory.
                         */
                        if (rc == NO_ERROR)
                        {
                            for (tmp2 = tmp; (c = *(tmp2++)) && (c != '/') && (c != '\\'););
                            if (!c)
                            {
                                /*
                                 * tmp  is the name of search
                                 * base is the directory I-node of search
                                 */
                                dir = base;
                                dir->i_count++;
                                error = 0;
                            }
                            else
                            {
                                /*
                                 * there are still path components left
                                 */
                                base->i_count++;
                                error = dir_namei(tmp, __StackToFlat(&namelen), __StackToFlat(&basename), base, __StackToFlat(&dir));
                            }
                            rc = ERROR_PATH_NOT_FOUND;
                            if (!error)
                            {
                                if (S_ISDIR(dir->i_mode))
                                {
                                    /*
                                     * Assigns a file structure
                                     */
                                    f = get_empty_filp();
                                    if (f)
                                    {
                                        f->f_inode = dir;
                                        f->f_pos = 0;
                                        f->f_mode = OPENMODE_READONLY;
                                        if (f->f_inode->i_op)
                                            f->f_op = f->f_inode->i_op->default_file_ops;

                                        ((p_hfind) pfsfsd)->p_file = f;

                                        /*
                                         * Locks the user buffer so that another thread can't free it from under us.
                                         */
                                        rc = DevHlp32_VMLock(
                                                                VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE,
                                                                pData,
                                                                parms->cbData,
                                                                (void *)-1,
                                                          __StackToFlat(lock),
                                                       __StackToFlat(&PgCount)
                                            );
                                        if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                        {
                                            /*
                                             * Do the actual search
                                             */
                                            rc = myfindnext(
                                                               dir->i_sb,
                                                               f,
                                                  parms->attr | FILE_READONLY,
                                                               pfsfsi,
                                                               pfsfsd,
                                                               pData,
                                                               parms->cbData,
                                                               pcMatch,
                                                               parms->level,
                                                               parms->flags,
                                                               0,
                                                               1,
                                                           is_case_retensive()
                                                );
                                            /*
                                             * Unlocks the user buffer.
                                             */
                                            if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                            {
                                                /*
                                                 * Nothing else to do
                                                 */
                                            }
                                            else
                                            {
                                                rc = rc2;
                                            }   /* VMUnlock failed */
                                        }   /* VMLock failed */
                                    }
                                    else
                                    {
                                        iput(dir);
                                    }   /* f = 0 */
                                }   /* !S_ISDIR */
                            }   /* error != 0 */
                        }       /* error retrieving base I-node */
                    }           /* Devhlp32_VMAlloc failed */
                }               /* pData cannot be thunked */
            }                   /* pcMatch = 0 */
        }                       /* Invalid level */
    }                           /* Invalid flag */

    if (trace_FS_FINDFIRST)
    {
        kernel_printf("FS_FINDFIRST post-invocation - rc = %d - pcMatch = %d", rc, *pcMatch);
    }

    return rc;
}
