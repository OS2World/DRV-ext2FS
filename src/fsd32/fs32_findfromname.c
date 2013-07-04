
/*
 *@@sourcefile fs32_findfromname.c:
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
#include <linux\stat.h>
#include <os2\filefind.h>

/*
 * struct fs32_findfromname_parms {
 *     unsigned short  flags;
 *     PTR16           pName;
 *     unsigned long   position;
 *     unsigned short  level;
 *     PTR16           pcMatch;
 *     unsigned short  cbData;
 *     PTR16           pData;
 *     PTR16           pfsfsd;
 *     PTR16           pfsfsi;
 * };
 */
int FS32ENTRY fs32_findfromname(struct fs32_findfromname_parms *parms)
{
    struct fsfsi32 *pfsfsi;
    struct fsfsd32 *pfsfsd;

//    char           *pName;
    char *pData;
    unsigned short *pcMatch;
    int rc, rc2;
    char lock[12];
    ULONG PgCount;


    pfsfsi = VDHQueryLin(parms->pfsfsi);
    pfsfsd = VDHQueryLin(parms->pfsfsd);
    pcMatch = VDHQueryLin(parms->pcMatch);

    if (trace_FS_FINDFROMNAME)
    {
        kernel_printf("FS_FINDFROMNAME pre-invocation %s , match = %u , level = %u, flag = %u", ((p_hfind) pfsfsd)->pName, *pcMatch, parms->level, parms->flags);
    }

    rc = ERROR_INVALID_PARAMETER;

    if ((parms->flags == FF_NOPOS) ||
        (parms->flags == FF_GETPOS))
    {

        if ((parms->level == FIL_STANDARD) ||
            (parms->level == FIL_QUERYEASIZE) ||
            (parms->level == FIL_QUERYEASFROMLIST))
        {

            if (*pcMatch != 0)
            {

                if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                {

                    rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount));
                    if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                    {

                        /*
                         * Do the actual search
                         */
                        rc = myfindnext(
                                    ((p_hfind) pfsfsd)->p_file->f_inode->i_sb,
                                           ((p_hfind) pfsfsd)->p_file,
                                           ((p_hfind) pfsfsd)->attr,
                                           pfsfsi,
                                           pfsfsd,
                                           pData,
                                           parms->cbData,
                                           pcMatch,
                                           parms->level,
                                           parms->flags,
                                           ((p_hfind) pfsfsd)->last,
                                           0,
                                           is_case_retensive()
                            );
                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                        {
                            /*
                             * Nothing else ...
                             */
                        }
                        else
                        {
                            rc = rc2;
                        }       /* VMUnlock failed */
                    }           /* VMLock failed */
                }               /* VirtToLin failed */
            }                   /* *pcMatch = 0 */
        }                       /* level invalid */
    }                           /* flags invalid */

    if (trace_FS_FINDFROMNAME)
    {
        kernel_printf("FS_FINDFROMNAME post-invocation %s ,rc = %d", ((p_hfind) pfsfsd)->pName, rc);
    }

    return rc;
}
