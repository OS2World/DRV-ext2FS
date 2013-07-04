
/*
 *@@sourcefile fs32_dopageio.c:
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
#include <os2\fsd32.h>
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>

#include <linux\stat.h>

#include <os2\os2proto.h>
#include <os2\ifsdbg.h>
#include <os2\filefind.h>
#include <os2\errors.h>
#include <os2\log.h>
#include <os2\volume.h>
#include <os2\os2misc.h>
#include <os2\trace.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <linux\sched.h>

long swap_in_progress = 0;
long nr_total_pgin = 0;
long nr_total_pgout = 0;
long nr_pgin = 0;
long nr_pgout = 0;

/*
 * struct fs32_dopageio_parms {
 *     PTR16 pPageCmdList;
 *     PTR16 psffsd;
 *     PTR16 psffsi;
 * };
 */
int FS32ENTRY fs32_dopageio(struct fs32_dopageio_parms *parms)
{
    struct PageCmdHeader *pPageCmdList;
    union sffsd32 *psffsd;
    int rc, i;
    struct file *f;
    int r = 0, w = 0;


    if ((rc = DevHlp32_VirtToLin(parms->psffsd, __StackToFlat(&psffsd))) == NO_ERROR)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pPageCmdList, __StackToFlat(&pPageCmdList))) == NO_ERROR)
        {
            if (trace_FS_DOPAGEIO)
            {
                kernel_printf("FS_DOPAGEIO pre-invocation - n=%d", (int)(pPageCmdList->OpCount));
            }

            for (i = 0; i < pPageCmdList->OpCount; i++)
            {
                if (pPageCmdList->PageCmdList[i].Cmd == PB_READ_X)
                    r++;
                if (pPageCmdList->PageCmdList[i].Cmd == PB_WRITE_X)
                    w++;
            }
            nr_total_pgin += r;
            nr_total_pgout += w;
            nr_pgin += r;
            nr_pgout += w;

            /*
             * Increments the swap flag
             */
            swap_in_progress++;

            /*
             * Gets the file structure from psffsd
             */
            if ((f = psffsd->f) != 0)
            {
                pPageCmdList->OutFlags = 0;
                for (i = 0; i < pPageCmdList->OpCount; i++)
                {
                    pPageCmdList->PageCmdList[i].Status = RH_NOT_QUEUED | RH_NO_ERROR;
                    pPageCmdList->PageCmdList[i].Error = NO_ERROR;
                }
                do_pageio(pPageCmdList, f);
//*************
                for (i = 0; i < pPageCmdList->OpCount; i++)
                {
                    if (!(pPageCmdList->PageCmdList[i].Status & RH_DONE))
                        ext2_os2_panic(0, "FS_DOPAGEIO - RLE %d not completed", i);
                }
//*************
                rc = NO_ERROR;
            }
            else
            {
                rc = ERROR_INVALID_PARAMETER;
            }

            /*
             * Decrements the swap flag
             */
            swap_in_progress--;

            if (trace_FS_DOPAGEIO)
            {
                kernel_printf("FS_DOPAGEIO post-invocation - in_progress=%ld rc = %d", swap_in_progress, rc);
            }
        }
    }

    return rc;
}
