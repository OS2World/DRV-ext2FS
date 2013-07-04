
/*
 *@@sourcefile fs32_close.c:
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
#include <os2\errors.h>
#include <os2\trace.h>

/*
 * struct fs32_close_parms {
 *     PTR16 psffsd;
 *     PTR16 psffsi;
 *     unsigned short IOflag;
 *     unsigned short type;
 * };
 */
int FS32ENTRY fs32_close(struct fs32_close_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;

    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

    //
    // Gets the file structure from psffsd
    //
    if (psffsd->f == 0)
    {
        ext2_os2_panic(1, "FS_CLOSE - file is NULL");
    }

    if (trace_FS_CLOSE)
    {
        kernel_printf("FS_CLOSE(ino = %lu, type = %d)", psffsd->f->f_inode->i_ino, parms->type);
    }

    //
    // The doc isn't clear about the role of the 'type' parameter. It seems we must
    // only free the resources (file structure in sffsd) at FS_CL_FORSYS time. Otherwise
    // we'' receive an empty sffsd somewhere else !
    // For other 'type' values, maybe we could do a flush ...
    //
    if (parms->type != FS_CL_FORSYS)
    {
#ifdef FS_TRACE
        kernel_printf("***** Non final system close **** - sffsi->sfi_type = %d - Type = %d", psffsi->sfi_type, type);
#endif
        return NO_ERROR;
    }                           /* endif */

    //
    // Final close for the system
    //
    if ((parms->type == FS_CL_FORSYS) && (Read_Write) && (psffsd->f->f_inode->i_ino != INODE_DASD))
    {
        if (psffsd->f->f_op)
        {
            if (psffsd->f->f_op->release)
            {
                psffsd->f->f_op->release(psffsd->f->f_inode, psffsd->f);
            }
            else
            {
                ext2_os2_panic(1, "FS_CLOSE - psffsd->f->f_op->release == NULL : shouldn't occur in this release");
            }
        }
        else
        {
            ext2_os2_panic(1, "FS_CLOSE - psffsd->f->f_op == NULL : shouldn't occur in this release");
        }
    }

    //
    // Closes the file
    //
    if ((rc = vfs_close(psffsd->f)) != NO_ERROR)
    {
        fs_err(FUNC_FS_CLOSE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
        return rc;
    }
#if 0
    //
    // Clean up of sffsd (safety purposes ...)
    //
    memset(psffsd, 0, sizeof(union sffsd32));

#endif
    rc = NO_ERROR;

    return rc;
}
