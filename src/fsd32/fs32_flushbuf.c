
/*
 *@@sourcefile fs32_flushbuf.c:
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
 * struct fs32_flushbuf_parms {
 *     unsigned short flag;
 *     unsigned short hVPB;
 * };
 */
int FS32ENTRY fs32_flushbuf(struct fs32_flushbuf_parms *parms)
{
    int rc;
    struct super_block *sb;


    if (trace_FS_FLUSHBUF)
    {
        kernel_printf("FS_FLUSHBUF - flag = %d, hVPB = 0x%0X", (int)(parms->flag), (int)(parms->hVPB));
    }

    if (Read_Write)
    {
        sb = getvolume(parms->hVPB);

        if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
        {
            if (sb->s_status == VOL_STATUS_MOUNTED)
            {
                kernel_printf("\tmedia is present");
                switch (parms->flag)
                {
                    case FLUSH_RETAIN:
                    case FLUSH_DISCARD:
                        sync_buffers(sb->s_dev, 1);
                        if ((sb->s_op) && (sb->s_op->write_super))
                            sb->s_op->write_super(sb);
                        sync_inodes(sb->s_dev);
                        sync_buffers(sb->s_dev, 1);
                        rc = NO_ERROR;
                        break;


                    default:
                        rc = ERROR_INVALID_PARAMETER;
                        break;
                }
            }
            else
            {
                rc = NO_ERROR;
            }
        }
        else
        {
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        rc = NO_ERROR;
    }

    if (trace_FS_FLUSHBUF)
    {
        kernel_printf("FS_FLUSHBUF - flag = %d, hVPB = 0x%0X, rc = %d", (int)(parms->flag), (int)(parms->hVPB), rc);
    }

    return rc;
}
