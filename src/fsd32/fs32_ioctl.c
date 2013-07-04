
/*
 *@@sourcefile fs32_ioctl.c:
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
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>

/*
 * struct fs32_ioctl_parms {
 *     PTR16          pDataLenInOut;
 *     unsigned short lenData;
 *     PTR16          pData;
 *     PTR16          pParmLenInOut;
 *     unsigned short lenParm;
 *     PTR16          pParm;
 *     unsigned short func;
 *     unsigned short cat;
 *     PTR16           psffsd;
 *     PTR16           psffsi;
 * };
 */
int FS32ENTRY fs32_ioctl(struct fs32_ioctl_parms *parms)
{
    unsigned short *pDataLenInOut;
    unsigned short *pParmLenInOut;
    PTR16 pvpfsi16;
    PTR16 pvpfsd16;
    struct sffsi32 *psffsi;
    struct vpfsi32 *pvpfsi;
    union vpfsd32 *pvpfsd;

    int rc;

    if (trace_FS_IOCTL)
    {
        kernel_printf("FS_IOCTL pre-invocation : cat = 0x%0X, func = 0x%0X",
                        parms->cat, parms->func);
    }

    if ((rc = DevHlp32_VirtToLin(parms->psffsi,
                                  __StackToFlat(&psffsi))) == NO_ERROR)
    {
        if ((rc = fsh32_getvolparm(psffsi->sfi_hVPB,
                                    __StackToFlat(&pvpfsi16),
                                    __StackToFlat(&pvpfsd16))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(pvpfsi16,
                                        __StackToFlat(&pvpfsi))) == NO_ERROR)
            {
                rc = fsh32_devioctl(
                                       0,
                                       pvpfsi->vpi_hDEV,
                                       psffsi->sfi_selfsfn,     /* sfn */
                                       parms->cat,
                                       parms->func,
                                       parms->pParm,
                                       parms->lenParm,
                                       parms->pData,
                                       parms->lenData
                    );
            }
        }                       /* FSH_GETVOLPARM failed */

        if (parms->pParmLenInOut.seg)
        {
            if (DevHlp32_VirtToLin(parms->pParmLenInOut, __StackToFlat(&pParmLenInOut)) == NO_ERROR)
            {
                *pParmLenInOut = parms->lenParm;
            }
        }
        if (parms->pDataLenInOut.seg)
        {
            if (DevHlp32_VirtToLin(parms->pDataLenInOut, __StackToFlat(&pDataLenInOut)) == NO_ERROR)
            {
                *pDataLenInOut = parms->lenData;
            }
        }
    }

    if (trace_FS_IOCTL)
    {
        kernel_printf("FS_IOCTL post-invocation : rc = %d", rc);
    }

    return rc;
}
