
/*
 *@@sourcefile fs32_fileinfo.c:
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
 * struct fs32_fileinfo_parms {
 *     unsigned short  IOflag;
 *     unsigned short  cbData;
 *     PTR16           pData;
 *     unsigned short  level;
 *     PTR16           psffsd;
 *     PTR16           psffsi;
 *     unsigned short  flag;
 * };
 */
int FS32ENTRY fs32_fileinfo(struct fs32_fileinfo_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    char *pData;
    int rc;
    int rc2;
    UINT32 len;
    PEAOP16 peaop;
    PFEALIST fpFEAList;
    ULONG PgCount;
    char lock[12];

    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
    {
        if ((parms->level != FIL_STANDARD) &&
            (parms->level != FIL_QUERYEASIZE) &&
            (parms->level != FIL_QUERYEASFROMLIST) &&
            (parms->level != FIL_QUERYALLEAS) &&    // Undocumented level - similar to level 3 but for full EA set
             (parms->level != FIL_LEVEL7))
        {
            if ((rc = kernel_printf("FS_FILEINFO() - invalid level %u", parms->level)) != NO_ERROR)
            {
                return rc;
            }                   /* end if */
            return ERROR_INVALID_PARAMETER;
        }


        //
        // Gets the file structure from psffsd
        //
        if (psffsd->f == 0)
            return ERROR_INVALID_PARAMETER;

        switch (parms->flag)
        {
            case FI_SET:
                if (trace_FS_FILEINFO)
                {
                    kernel_printf("FS_FILEINFO - FI_SET");
                }

                rc = NO_ERROR;
                if (Read_Write)
                {
//                if ((rc = LockUserBuffer(pData, parms->cbData, __StackToFlat(lock), LOCK_READ, &lock_lin)) == NO_ERROR) {
                    if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                    {
                        if ((rc = fileinfo_to_ino(pData, psffsd->f->f_inode, parms->level, parms->cbData, psffsi)) == NO_ERROR)
                        {
                            /*
                             * Nothing else to do
                             */
                        }
                        else
                        {
                            kernel_printf("fileinfo_to_ino returned %d", rc);
                        }
                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                        {
                            /* Nothing else to do */
                        }
                        else
                        {
                            kernel_printf("FS_FILEINFO : VMUnlock() returned %d", rc2);
                            rc = rc2;
                        }       /* VMUnlock failed */
                    }
                    else
                    {
                        kernel_printf("FS_FILEINFO : VMLock() returned %d", rc);
                    }
                }
                else
                {
                    rc = ERROR_WRITE_PROTECT;
                }
                break;

            case FI_RETRIEVE:
                if (trace_FS_FILEINFO)
                {
                    kernel_printf("FS_FILEINFO - FI_RETRIEVE - lvl = %u - inode %lu )", parms->level, psffsd->f->f_inode->i_ino);
                }
                if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                {

                    if ((parms->level == FIL_QUERYEASFROMLIST) ||
                        (parms->level == FIL_QUERYALLEAS))
                    {
                        if (parms->cbData >= 4)
                        {
                            peaop = (PEAOP16) pData;
//                    if ((rc = FSH_PROBEBUF(PB_OPWRITE, (pchar)peaop, 4)) != NO_ERROR) {
                            //                        fs_err(FUNC_FS_FILEINFO, FUNC_FSH_PROBEBUF, rc, THISFILE, __LINE__);
                            //                        return rc;
                            //                    } /* end if */
                            if ((rc = DevHlp32_VirtToLin(peaop->fpFEAList, __StackToFlat(&fpFEAList))) == NO_ERROR)
                            {
                                fpFEAList->cbList = 4;
                                rc = NO_ERROR;
                            }
                        }
                        else
                        {
                            rc = ERROR_BUFFER_OVERFLOW;
                        }
                    }
                    else
                    {
                        if ((rc = ino_to_fileinfo(
                                                     psffsd->f->f_inode,
                                                     pData,
                                                     parms->cbData,
                                                     __StackToFlat(&len),
                                                     parms->level,
                                                     0,
                                                     0,
                                                     0,
                                             0, TYPEOP_FILEINFO)) != NO_ERROR)
                        {
                            fs_log("FS_FILEINFO() - ino_to_fileinfo()");
//                      return rc;
                        }
                    }
                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                    {
                    }
                    else
                    {
                        rc = rc2;
                    }
                }               /* DevHlp_VMLock */
                break;

            default:
                fs_log("FS_FILEINFO() - Unknown flag");
                rc = ERROR_INVALID_PARAMETER;
                break;
        }


    }
    return rc;
}
