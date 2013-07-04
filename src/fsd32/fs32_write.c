
/*
 *@@sourcefile fs32_write.c:
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
 * struct fs32_write_parms {
 *     unsigned short IOflag;
 *     PTR16          pLen;
 *     PTR16          pData;
 *     PTR16          psffsd;
 *     PTR16          psffsi;
 * };
 */
int FS32ENTRY fs32_write(struct fs32_write_parms *parms)
{
    char *pData;
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;
    unsigned short *pLen;
    int rc2, err;
    unsigned long BytesWritten;
    char lock[12];
    unsigned long PgCount;


    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);
    pLen = VDHQueryLin(parms->pLen);

    if (trace_FS_WRITE)
    {
        kernel_printf("FS_WRITE( len = %u ) pre-invocation", *pLen);
    }

    if (Read_Write)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
        {
            /*
             * Locks the user buffer to check read access and to prevent other threads from freing it
             * behind us when we are sleeping. (this is a long term verify lock)
             */
            rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, *pLen, (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount));
            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
            {
                /*
                 * Gets the file structure from psffsd
                 */
                if ((psffsd->f) && (psffsd->f->f_magic == FILE_MAGIC))
                {
                    /*
                     * Tests if it is a regular file
                     */
                    if (S_ISREG(psffsd->f->f_inode->i_mode))
                    {
                        /*
                         * If the file position in psffsi is not consistent with the
                         * one in the file structure in psffsd, something went wrong => panic
                         */
                        if (psffsi->sfi_position == (unsigned long)psffsd->f->f_pos)
                        {
                            /*
                             * Now we do the actual write operation
                             */
                            err = VFS_write(psffsd->f, pData, (loff_t) * pLen, __StackToFlat(&BytesWritten));
                            *pLen = (unsigned short)BytesWritten;
                            psffsi->sfi_tstamp |= ST_PWRITE;
                            psffsi->sfi_size = psffsd->f->f_inode->i_size;
                            psffsi->sfi_position = psffsd->f->f_pos;
                            rc = err;
                        }
                        else
                        {
                            *pLen = 0;
                            rc = ERROR_WRITE_FAULT;     // Maybe we should FSH_INTERR ?

                        }       /* sfi_position != f_pos */

                    }
                    else
                    {
                        *pLen = 0;
                        rc = ERROR_ACCESS_DENIED;
                    }           /* !S_ISREG */
                }
                else
                {
                    rc = ERROR_INVALID_PARAMETER;
                }               /* psffsd->f = NULL */

                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                {
                    /*
                     * Nothing else to do
                     */
                }
                else
                {
                    rc = rc2;
                }               /* VMUnlock failed */
            }                   /* VMLock()  failed */
        }                       /* VirtToLin(pData) */

    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }

    if (trace_FS_WRITE)
    {
        kernel_printf("FS_WRITE( len = %u ) post-invocation (rc = %d)", *pLen, rc);
    }

    return rc;
}
