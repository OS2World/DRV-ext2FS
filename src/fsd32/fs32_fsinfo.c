
/*
 *@@sourcefile fs32_fsinfo.c:
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
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>

#pragma pack(1)
struct FSINFO16
{                               /* fsinf */
    ULONG ulVSN;
    VOLUMELABEL vol;
};

#pragma pack()

/*
 * struct fs32_fsinfo_parms {
 *     unsigned short level;
 *     unsigned short cbData;
 *     PTR16          pData;
 *     unsigned short hVPB;
 *     unsigned short flag;
 * };
 */
int FS32ENTRY fs32_fsinfo(struct fs32_fsinfo_parms *parms)
{
    char *pData;
    int rc;
    int rc2;

    FSALLOCATE *pfsil_alloc;
    struct super_block *sb;
    struct ext2_super_block *psb;
    struct vpfsi32 *pvpfsi;
    union vpfsd32 *pvpfsd;
    PTR16 pvpfsi16;
    PTR16 pvpfsd16;
    struct FSINFO16 *pvolser;
    VOLUMELABEL *label;
    char lock[12];
    unsigned long PgCount;


    if (trace_FS_FSINFO)
    {
        printk("FS_FSINFO pre-invocation : flag = %d, level = %d", parms->flag, parms->level);
    }

    switch (parms->flag)
    {

        case INFO_RETREIVE:
            switch (parms->level)
            {
                case FSIL_VOLSER:
                    if ((rc = fsh32_getvolparm(parms->hVPB, __StackToFlat(&pvpfsi16), __StackToFlat(&pvpfsd16))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(pvpfsi16, __StackToFlat(&pvpfsi))) == NO_ERROR)
                        {
                            if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(struct FSINFO16), (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {

                                    pvolser = (struct FSINFO16 *)pData;
                                    pvolser->ulVSN = pvpfsi->vpi_vid;
                                    strncpy(pvolser->vol.szVolLabel, pvpfsi->vpi_text, sizeof(pvolser->vol.szVolLabel));
                                    pvolser->vol.szVolLabel[sizeof(pvolser->vol.szVolLabel) - 1] = '\0';
                                    pvolser->vol.cch = (BYTE) strlen(pvpfsi->vpi_text);

                                    rc = NO_ERROR;

                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                    {
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }

                                }
                            }
                        }
                    }
                    break;

                case FSIL_ALLOC:
                    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                    {
                        rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(FSALLOCATE), (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount));
                        if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                        {
                            sb = getvolume(parms->hVPB);
                            psb = sb->u.ext2_sb.s_es;
                            pfsil_alloc = (FSALLOCATE *) pData;
//                          pfsil_alloc->idFileSystem      =  ????? ;
                            pfsil_alloc->cSectorUnit = sb->sectors_per_block;
                            pfsil_alloc->cUnit = psb->s_blocks_count;
                            pfsil_alloc->cUnitAvail = (psb->s_free_blocks_count > psb->s_r_blocks_count ? psb->s_free_blocks_count - psb->s_r_blocks_count : 0);
                            pfsil_alloc->cbSector = (UINT16) sb->sector_size;
                            rc = NO_ERROR;
                            if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                            {
                            }
                            else
                            {
                                rc = rc2;
                            }
                        }
                    }
                    break;

                default:
                    rc = ERROR_INVALID_PARAMETER;
                    break;
            }
            break;

        case INFO_SET:
            if (Read_Write)
            {
                if (parms->level == FSIL_VOLSER)
                {
                    if ((rc = fsh32_getvolparm(parms->hVPB, __StackToFlat(&pvpfsi16), __StackToFlat(&pvpfsd16))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(pvpfsi16, __StackToFlat(&pvpfsi))) == NO_ERROR)
                        {
                            if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, sizeof(struct FSINFO16), (void *)-1, __StackToFlat(lock), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {

                                    label = (VOLUMELABEL *) pData;
                                    sb = getvolume(parms->hVPB);
                                    psb = sb->u.ext2_sb.s_es;

                                    strncpy(pvpfsi->vpi_text, label->szVolLabel, sizeof(pvpfsi->vpi_text));
                                    pvpfsi->vpi_text[sizeof(pvpfsi->vpi_text) - 1] = '\0';
                                    strncpy(psb->s_volume_name, label->szVolLabel, sizeof(psb->s_volume_name));
                                    mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
                                    sb->s_dirt = 1;

                                    rc = NO_ERROR;

                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock))) == NO_ERROR)
                                    {
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }

                                }
                            }
                        }
                    }
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
            break;

        default:
            rc = ERROR_INVALID_PARAMETER;
            break;
    }

    if (trace_FS_FSINFO)
    {
        printk("FS_FSINFO post-invocation : flag = %d, level = %d, rc = %d", parms->flag, parms->level, rc);
    }

    return rc;
}
