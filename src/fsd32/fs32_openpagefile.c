
/*
 *@@sourcefile fs32_openpagefile.c:
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

static int lock_IFS_image(void)
{
    return NO_ERROR;
}

static int lock_swapper_filp(struct file *f)
{
    int rc;
    char lock_file[12];
    char lock_inode[12];
    char lock_super[12];
    char lock_gdesc[12];
    blk_t db_count;
    ULONG PgCount;

    if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_WRITE, f, sizeof(struct file), (void *)-1, __StackToFlat(lock_file), __StackToFlat(&PgCount))) == NO_ERROR)
    {
//    if ((rc = LockBuffer(f, sizeof(struct file), lock_file, LOCK_WRITE, &lock_file_lin)) == NO_ERROR) {
        /*
         * Locks the SWAPPER.DAT I-node into physical memory
         */
        if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_WRITE, f->f_inode, sizeof(struct inode), (void *)-1, __StackToFlat(lock_inode), __StackToFlat(&PgCount))) == NO_ERROR)
        {
//        if ((rc = LockBuffer(f->f_inode, sizeof(struct inode), lock_inode, LOCK_WRITE, &lock_inode_lin)) == NO_ERROR) {
            /*
             * Locks the SWAPPER.DAT superblock into physical memory
             */
            if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_WRITE, f->f_inode->i_sb, sizeof(struct super_block), (void *)-1, __StackToFlat(lock_super), __StackToFlat(&PgCount))) == NO_ERROR)
            {
//            if ((rc = LockBuffer(f->f_inode->i_sb, sizeof(struct super_block), lock_super, LOCK_WRITE, &lock_super_lin)) == NO_ERROR) {
                db_count = (f->f_inode->i_sb->u.ext2_sb.s_groups_count + EXT2_DESC_PER_BLOCK(f->f_inode->i_sb) - 1) /
                    EXT2_DESC_PER_BLOCK(f->f_inode->i_sb);
                /*
                 * Locks the SWAPPER.DAT superblock group descriptors into physical memory
                 */
                if ((rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_WRITE, f->f_inode->i_sb->u.ext2_sb.s_group_desc, db_count * sizeof(struct buffer_head *), (void *)-1, __StackToFlat(lock_gdesc), __StackToFlat(&PgCount))) == NO_ERROR)
                {
//                if ((rc = LockBuffer(f->f_inode->i_sb->u.ext2_sb.s_group_desc, db_count * sizeof (struct buffer_head *), lock_gdesc, LOCK_WRITE, &lock_gdesc_lin)) == NO_ERROR) {
                    /*
                     * ...
                     */
                }
                else
                {
                    kernel_printf("lock_swapper_filp - Couldn't lock group descriptors into memory");
                }
            }
            else
            {
                kernel_printf("lock_swapper_filp - Couldn't lock superblock into memory");
            }
        }
        else
        {
            kernel_printf("lock_swapper_filp - Couldn't lock SWAPPER.DAT I-node into memory");
        }
    }
    else
    {
        kernel_printf("lock_swapper_filp - Couldn't lock SWAPPER.DAT file structure into memory");
    }
    return rc;
}


extern int _fs32_opencreate();
extern int _fs32_allocatepagespace();

/*
 * struct fs32_openpagefile_parms {
 *     unsigned long  Reserved;
 *     unsigned short Attr;
 *     unsigned short OpenFlag;
 *     unsigned short OpenMode;
 *     PTR16 psffsd;
 *     PTR16 psffsi;
 *     PTR16 pName;
 *     PTR16 pcMaxReq;
 *     PTR16 pFlags;
 * };
 */
int FS32ENTRY fs32_openpagefile(struct fs32_openpagefile_parms *parms)
{
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    char *pName;
    unsigned long *pFlags;
    unsigned long *pcMaxReq;
    struct fs32_opencreate_parms open_parms;
    struct fs32_allocatepagespace_parms alloc_parms;
    struct super_block *sb;
    int rc;

    if (Read_Write)
    {

        if ((rc = DevHlp32_VirtToLin(parms->psffsi, __StackToFlat(&psffsi))) == NO_ERROR)
        {
            sb = getvolume(psffsi->sfi_hVPB);
            if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
            {
                if (sb->s_strat2.seg)
                {
                    if ((rc = DevHlp32_VirtToLin(parms->psffsd, __StackToFlat(&psffsd))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(parms->pName, __StackToFlat(&pName))) == NO_ERROR)
                        {
                            if ((rc = DevHlp32_VirtToLin(parms->pFlags, __StackToFlat(&pFlags))) == NO_ERROR)
                            {
                                if ((rc = DevHlp32_VirtToLin(parms->pcMaxReq, __StackToFlat(&pcMaxReq))) == NO_ERROR)
                                {
                                    if (trace_FS_OPENPAGEFILE)
                                    {
                                        kernel_printf("FS_OPENPAGEFILE(%s) pre-invocation - pFlags=%lu", pName, *pFlags);
                                    }

                                    memset(&open_parms, 0, sizeof(struct fs32_opencreate_parms));

                                    open_parms.attr = parms->Attr;
                                    open_parms.openflag = parms->OpenFlag;
                                    open_parms.ulOpenMode = parms->OpenMode;
                                    open_parms.psffsd = parms->psffsd;
                                    open_parms.psffsi = parms->psffsi;
                                    open_parms.pcdfsi.seg = 0;
                                    open_parms.pcdfsd.seg = 0;
                                    open_parms.iCurDirEnd = -1;
                                    open_parms.pName = parms->pName;
                                    open_parms.pAction = parms->pcMaxReq;   // Hack ...

                                    rc = _fs32_opencreate(__StackToFlat(&open_parms));
                                    if (rc == NO_ERROR)
                                    {
                                        /*
                                         * Puts the file on the swapper list
                                         */
                                        set_swapper_filp(psffsd->f);

                                        /*
                                         * Marks the superblock as holding SWAPPER.DAT
                                         */
                                        psffsd->f->f_inode->i_sb->s_is_swapper_device = 1;

                                        *pcMaxReq = 0xFFFFFFFF;     /* no limit : we emulate for the moment */
                                        *pFlags |= PGIO_PADDR;
                                        if ((rc = lock_IFS_image()) == NO_ERROR)
                                        {
                                            kernel_printf("FS_OPENPAGEFILE - lock IFS image OK");
                                            if ((rc = lock_swapper_filp(psffsd->f)) == NO_ERROR)
                                            {
                                                kernel_printf("FS_OPENPAGEFILE - lock_swapper_filp OK");
                                                if (*pFlags & PGIO_FIRSTOPEN)
                                                {
                                                    alloc_parms.psffsi = parms->psffsi;
                                                    alloc_parms.psffsd = parms->psffsd;
                                                    alloc_parms.ulSize = psffsi->sfi_size;
                                                    alloc_parms.ulWantContig = 4096;

                                                    if ((rc = _fs32_allocatepagespace(__StackToFlat(&alloc_parms))) == NO_ERROR)
                                                    {
                                                        kernel_printf("FS_OPENPAGEFILE - FS_ALLOCATEPAGESPACE OK");
                                                        /*
                                                         * ...
                                                         */
                                                    }
                                                    else
                                                    {
                                                        kernel_printf("FS_OPENPAGEFILE - FS_ALLOCATEPAGESPACE failed");
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                kernel_printf("FS_OPENPAGEFILE - lock_swapper_filp failed");
                                            }
                                        }
                                        else
                                        {
                                            kernel_printf("FS_OPENPAGEFILE - lock IFS image failed");
                                        }
                                    }
                                    if (trace_FS_OPENPAGEFILE)
                                    {
                                        kernel_printf("FS_OPENPAGEFILE post-invocation - rc = %d", rc);
                                    }

                                }
                            }
                        }
                    }
                }
                else
                {
                    kernel_printf("FS_OPENPAGEFILE : device does not support strategy 2 I/O");
                    rc = ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                rc = ERROR_INVALID_PARAMETER;
            }
        }

    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }
    return rc;
}
