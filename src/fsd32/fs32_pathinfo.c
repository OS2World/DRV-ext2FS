
/*
 *@@sourcefile fs32_pathinfo.c:
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
#include <os2\filefind.h>

#define THISFILE FILE_TEST_C    //obsolete

/*
 * struct fs32_pathinfo_parms {
 *     unsigned short cbData;
 *     PTR16          pData;
 *     unsigned short level;
 *     unsigned short iCurDirEnd;
 *     PTR16          pName;
 *     PTR16          pcdfsd;
 *     PTR16          pcdfsi;
 *     unsigned short flag;
 * };
 */
int FS32ENTRY fs32_pathinfo(struct fs32_pathinfo_parms *parms)
{
    char *pData;
    char *pName;
    struct cdfsi32 *pcdfsi;

//    union  cdfsd32 *pcdfsd;
    int rc;
    int rc2, rc3;
    struct super_block *p_volume;
    struct file *p_file;
    UINT32 len, openmode = 0;
    PEAOP16 peaop;
    PFEALIST fpFEAList;
    char lock[12];
    unsigned long PgCount;



    if ((rc = DevHlp32_VirtToLin(parms->pcdfsi, __StackToFlat(&pcdfsi))) == NO_ERROR)
    {
//        if ((rc = DevHlp32_VirtToLin(parms->pcdfsd, __StackToFlat(&pcdfsd))) == NO_ERROR) {
        if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(parms->pName, __StackToFlat(&pName))) == NO_ERROR)
            {

                //
                // Verify the level is valid
                //
                if ((parms->level == FIL_STANDARD) ||
                    (parms->level == FIL_QUERYEASIZE) ||
                    (parms->level == FIL_QUERYEASFROMLIST) ||
                    (parms->level == FIL_QUERYALLEAS) ||    // Undocumented level - similar to level 3 but for full EA set
                     (parms->level == FIL_LEVEL7))
                {

                    openmode = (is_case_retensive()? OPENMODE_DOSBOX : 0);

                    //
                    // Retrieves the superblock from psffsi
                    //
                    if ((p_volume = getvolume(pcdfsi->cdi_hVPB)) != 0)
                    {

                        switch (parms->flag)
                        {
                            case PI_RETRIEVE:
                                if (trace_FS_PATHINFO)
                                {
                                    kernel_printf("FS_PATHINFO - PI_RETRIEVE - lvl = %u - %s", parms->level, pName);
                                }


                                switch (parms->level)
                                {
                                    case FIL_LEVEL7:
                                        if (strlen(pName) < parms->cbData)
                                        {
                                            if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                                            {
                                                memcpy(pData, pName, strlen(pName) + 1);
                                                rc = NO_ERROR;
                                                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                                {
                                                    /* Nothing else to do */
                                                }
                                                else
                                                {
                                                    kernel_printf("FS_FILEINFO : VMUnlock() returned %d", rc2);
                                                    rc = rc2;
                                                }   /* VMUnlock failed */
                                            }
                                        }
                                        else
                                        {
                                            rc = ERROR_BUFFER_OVERFLOW;
                                        }
                                        break;

                                    default:
                                        if ((p_file = open_by_name(p_volume->s_mounted, skip_drive(pName), openmode | OPENMODE_READONLY)) != 0)
                                        {
                                            if ((parms->level == FIL_QUERYEASFROMLIST) ||
                                            (parms->level == FIL_QUERYALLEAS))
                                            {
                                                peaop = (PEAOP16) pData;
                                                if ((rc = DevHlp32_VirtToLin(peaop->fpFEAList, __StackToFlat(&fpFEAList))) == NO_ERROR)
                                                {
                                                    if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, 4, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                                                    {
                                                        fpFEAList->cbList = 4;  // NO EAS for ext2

                                                        rc = NO_ERROR;
                                                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                                        {
                                                            /* Nothing else to do */
                                                        }
                                                        else
                                                        {
                                                            kernel_printf("FS_FILEINFO : VMUnlock() returned %d", rc2);
                                                            rc = rc2;
                                                        }   /* VMUnlock failed */
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                                                {
                                                    if ((rc = ino_to_fileinfo(
                                                              p_file->f_inode,
                                                                        pData,
                                                                parms->cbData,
                                                          __StackToFlat(&len),
                                                                 parms->level,
                                                                            0,
                                                                            0,
                                                                            0,
                                                                                 0, TYPEOP_FILEINFO)) == NO_ERROR)
                                                    {
                                                    }
                                                    else
                                                    {
                                                        fs_log("FS_PATHINFO() - ino_to_fileinfo()");
                                                    }
                                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                                    {
                                                        /* Nothing else to do */
                                                    }
                                                    else
                                                    {
                                                        kernel_printf("FS_FILEINFO : VMUnlock() returned %d", rc2);
                                                        rc = rc2;
                                                    }   /* VMUnlock failed */

                                                }
                                            }
                                            if ((rc2 = vfs_close(p_file)) == NO_ERROR)
                                            {
                                            }
                                            else
                                            {
                                                fs_err(FUNC_FS_PATHINFO, FUNC_CLOSE, rc, THISFILE, __LINE__);
                                                rc2 = rc;
                                            }
                                        }
                                        else
                                        {
#ifdef FS_TRACE
                                            fs_err(FUNC_FS_PATHINFO, FUNC_OPEN_BY_NAME, ERROR_PATH_NOT_FOUND, THISFILE, __LINE__);
#endif
                                            rc = ERROR_FILE_NOT_FOUND;
                                        }
                                        break;
                                }   /* end switch */
                                break;

                            case PI_SET:
                            case PI_SET + 0x10:
                                if (trace_FS_PATHINFO)
                                {
                                    kernel_printf("FS_PATHINFO - PI_SET - %s", pName);
                                }
                                rc = NO_ERROR;
                                if (Read_Write)
                                {
                                    if ((p_file = open_by_name(p_volume->s_mounted, skip_drive(pName), openmode | OPENMODE_READONLY)) != 0)
                                    {
                                        if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, parms->cbData, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount))) == NO_ERROR)
                                        {
                                            if ((rc = fileinfo_to_ino(pData, p_file->f_inode, parms->level, parms->cbData, 0)) == NO_ERROR)
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
                                                /*
                                                 * Nothing else to do ...
                                                 */
                                            }
                                            else
                                            {
                                                kernel_printf("FS_PATHINFO : VMUnlock() returned %d", rc2);
                                                rc = rc2;
                                            }   /* VMUnlock failed */
                                        }
                                        else
                                        {
                                            kernel_printf("FS_PATHINFO : LockUserBuffer() returned %d", rc);
                                        }

                                        if ((rc2 = vfs_close(p_file)) == NO_ERROR)
                                        {
                                            /*
                                             * Nothing else to do.
                                             */
                                        }
                                        else
                                        {
                                            fs_err(FUNC_FS_PATHINFO, FUNC_CLOSE, rc, THISFILE, __LINE__);
                                            rc = rc2;
                                        }
                                    }
                                    else
                                    {
#ifdef FS_TRACE
                                        fs_err(FUNC_FS_PATHINFO, FUNC_OPEN_BY_NAME, ERROR_PATH_NOT_FOUND, THISFILE, __LINE__);
#endif
                                        rc = ERROR_FILE_NOT_FOUND;
                                    }
                                }
                                else
                                {
                                    rc = ERROR_WRITE_PROTECT;
                                }
                                break;


                            default:
                                kernel_printf("FS_PATHINFO( %s ) - unknown flag %d", pName, parms->flag);
                                rc = ERROR_INVALID_PARAMETER;
                                break;
                        }       /* end switch */

                    }
                    else
                    {
                        rc = ERROR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    kernel_printf("FS_PATHINFO() - invalid level %u", parms->level);
                    rc = ERROR_INVALID_PARAMETER;
                }

            }
        }
//        }
    }
    return rc;
}
