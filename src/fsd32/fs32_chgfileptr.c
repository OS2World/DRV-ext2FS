
/*
 *@@sourcefile fs32_chgfileptr.c:
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


extern unsigned long event;

/*
 * struct fs32_chgfileptr_parms {
 *     unsigned short IOflag;
 *     unsigned short type;
 *     long           offset;
 *     PTR16          psffsd;
 *     PTR16          psffsi;
 * };
 */
int FS32ENTRY fs32_chgfileptr(struct fs32_chgfileptr_parms *parms)
{
    off_t newfileptr;
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    int rc;


    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);

/*******************************************************************/
/*** Gets the file structure from psffsd                         ***/
/*******************************************************************/
    if (psffsd->f == 0)
    {
        kernel_printf("FS_CHGFILEPTR() - file is NULL");
        return ERROR_INVALID_PARAMETER;
    }
/*******************************************************************/


    if (trace_FS_CHGFILEPTR)
    {
        kernel_printf("FS_CHFILEPTR : ino = %lu - type = %d", psffsd->f->f_inode->i_ino, parms->type);
    }
    switch (parms->type)
    {
        case CFP_RELBEGIN:
            newfileptr = parms->offset;
            break;

        case CFP_RELCUR:
            newfileptr = psffsi->sfi_position + parms->offset;
            break;

        case CFP_RELEND:
            newfileptr = psffsd->f->f_inode->i_size + parms->offset;
            break;

        default:
            kernel_printf("FS_CHFILEPTR( ino = %lu ) : unknown type", psffsd->f->f_inode->i_ino);
            return ERROR_INVALID_PARAMETER;
    }

    //
    // Offsets below 0 should normally be supported for DOS box requests
    //
    if (newfileptr < 0)
    {
        kernel_printf("FS_CHGFILEPTR - new file pointer is < 0");
        return ERROR_INVALID_PARAMETER;
    }

    psffsd->f->f_pos = newfileptr;
    psffsd->f->f_reada = 0;
    psffsd->f->f_version = ++event;

    psffsi->sfi_position = newfileptr;
    rc = NO_ERROR;

    return rc;
}
