
/*
 *@@sourcefile fs32_shutdown.c:
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
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\ext2_proto.h>
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>

/*
 * struct fs32_shutdown_parms {
 *     unsigned long  ulReserved;
 *     unsigned short usType;
 * };
 */
int FS32ENTRY fs32_shutdown(struct fs32_shutdown_parms *parms)
{
    int rc;


    if (trace_FS_SHUTDOWN)
    {
        kernel_printf("FS_SHUTDOWN - type = %d", parms->usType);
    }
    switch (parms->usType)
    {
        case SD_BEGIN:
            if (Read_Write)
            {
                sync_buffers(0, 1);
            }
            rc = NO_ERROR;
            break;

        case SD_COMPLETE:
            invalidate_supers();
            if (Read_Write)
            {
                sync_buffers(0, 1);
            }
            rc = NO_ERROR;
            break;

        default:
            rc = ERROR_INVALID_PARAMETER;
    }
    return rc;
}
