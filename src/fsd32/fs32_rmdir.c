
/*
 *@@sourcefile fs32_rmdir.c:
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
#include <os2\log.h>            /* Prototypes des fonctions de log.c                      */
#include <os2\volume.h>         /* Prototypes des fonctions de volume.c                   */
#include <os2\os2misc.h>
#include <os2\trace.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <linux\sched.h>


/*
 * struct fs32_rmdir_parms {
 *     unsigned short iCurDirEnd;
 *     PTR16          pName;
 *     PTR16          pcdfsd;
 *     PTR16          pcdfsi;
 * };
 */
int FS32ENTRY fs32_rmdir(struct fs32_rmdir_parms *parms)
{
    char *pName;
    struct cdfsi32 *pcdfsi;

//    union  cdfsd32 *pcdfsd;
    struct super_block *sb;
    int rc;


    pcdfsi = VDHQueryLin(parms->pcdfsi);
    pName = VDHQueryLin(parms->pName);

    if (trace_FS_RMDIR)
    {
        kernel_printf("FS_RMDIR pre-invocation : %s", pName);
    }

    if (Read_Write)
    {
        sb = getvolume(pcdfsi->cdi_hVPB);
        if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
        {
            /*
             * Skip drive letter
             */
            pName = skip_drive(pName);

            rc = sys_rmdir(sb->s_mounted, pName);
            rc = map_err(rc);
        }
        else
        {
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }

    if (trace_FS_RMDIR)
    {
        kernel_printf("FS_RMDIR post-invocation : %s, rc = %d", pName, rc);
    }

    return rc;
}
