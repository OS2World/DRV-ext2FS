//
// $Header: e:\\netlabs.cvs\\ext2fs/src/ext2/truncate.c,v 1.1 2001/05/09 17:53:03 umoeller Exp $
//

// 32 bits Linux ext2 file system driver for OS/2 WARP - Allows OS/2 to
// access your Linux ext2fs partitions as normal drive letters.
// Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


/*
 *  linux/fs/ext2/truncate.c
 *
 *  Copyright (C) 1992, 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                                  Laboratoire MASI - Institut Blaise Pascal
 *                                  Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/truncate.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * Real random numbers for secure rm added 94/02/18
 * Idea from Pierre del Perugia <delperug@gla.ecoledoc.ibp.fr>
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#ifdef OS2
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the Developer Connection Device Driver Kit version 2.0
#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsh32.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <os2\os2proto.h>

#include <linux\errno.h>
// #include <linux\fcntl.h>
#include <linux\sched.h>
#include <linux\stat.h>
#include <linux\locks.h>
#else
#include <linux\errno.h>
#include <linux\fs.h>
#include <linux\ext2_fs.h>
#include <linux\fcntl.h>
#include <linux\sched.h>
#include <linux\stat.h>
#include <linux\locks.h>
#include <linux\string.h>
#endif

static int ext2_secrm_seed = 152;        /* Random generator base */

#ifndef OS2
#define RANDOM_INT (ext2_secrm_seed = ext2_secrm_seed * 69069l +1)
#else
#define RANDOM_INT (ext2_secrm_seed = (int)((long)ext2_secrm_seed * 69069l +1l))
#endif
/*
 * Truncate has the most races in the whole filesystem: coding it is
 * a pain in the a**. Especially as I don't do any locking...
 *
 * The code may look a bit weird, but that's just because I've tried to
 * handle things like file-size changes in a somewhat graceful manner.
 * Anyway, truncating a file at the same time somebody else writes to it
 * is likely to result in pretty weird behaviour...
 *
 * The new code handles normal truncates (size = 0) as well as the more
 * general case (size = XXX). I hope.
 */

static int trunc_direct (struct inode * inode)
{
        u32 * p;
        int i, tmp;
        struct buffer_head * bh;
        unsigned long block_to_free = 0;
        unsigned long free_count = 0;
        int retry = 0;
        int blocks = inode->i_sb->s_blocksize / 512;
#define DIRECT_BLOCK ((inode->i_size + inode->i_sb->s_blocksize - 1) / \
                        inode->i_sb->s_blocksize)
        int direct_block = DIRECT_BLOCK;

repeat:
        for (i = direct_block ; i < EXT2_NDIR_BLOCKS ; i++) {
                p = inode->u.ext2_i.i_data + i;
                tmp = *p;
                if (!tmp)
                        continue;
                if (inode->u.ext2_i.i_flags & EXT2_SECRM_FL)
                        bh = getblk (inode->i_dev, tmp,
                                     inode->i_sb->s_blocksize);
                else
                        bh = get_hash_table (inode->i_dev, tmp,
                                             inode->i_sb->s_blocksize);
                if (i < direct_block) {
                        brelse (bh);
                        goto repeat;
                }
                if ((bh && bh->b_count != 1) || tmp != *p) {
                        retry = 1;
                        brelse (bh);
                        continue;
                }
                *p = 0;
                inode->i_blocks -= blocks;
                inode->i_dirt = 1;
                if (inode->u.ext2_i.i_flags & EXT2_SECRM_FL) {
                        memset(bh->b_data, RANDOM_INT, inode->i_sb->s_blocksize);
                        mark_buffer_dirty(bh, 1);
                }
                brelse (bh);
                if (free_count == 0) {
                        block_to_free = tmp;
                        free_count++;
                } else if (free_count > 0 && block_to_free == tmp - free_count)
                        free_count++;
                else {
                        ext2_free_blocks (inode->i_sb, block_to_free, free_count);
                        block_to_free = tmp;
                        free_count = 1;
                }
/*                ext2_free_blocks (inode->i_sb, tmp, 1); */
        }
        if (free_count > 0)
                ext2_free_blocks (inode->i_sb, block_to_free, free_count);
        return retry;
}
static int trunc_indirect (struct inode * inode, int offset, u32 * p)
{
        int i, tmp;
        struct buffer_head * bh;
        struct buffer_head * ind_bh;
        u32 * ind;
        unsigned long block_to_free = 0;
        unsigned long free_count = 0;
        int retry = 0;
        int addr_per_block = EXT2_ADDR_PER_BLOCK(inode->i_sb);
        int blocks = inode->i_sb->s_blocksize / 512;
#define INDIRECT_BLOCK ((int)DIRECT_BLOCK - offset)
        int indirect_block = INDIRECT_BLOCK;

        tmp = *p;
        if (!tmp)
                return 0;
        ind_bh = bread (inode->i_dev, tmp, inode->i_sb->s_blocksize);
        if (tmp != *p) {
                brelse (ind_bh);
                return 1;
        }
        if (!ind_bh) {
                *p = 0;
                return 0;
        }
repeat:
        for (i = indirect_block ; i < addr_per_block ; i++) {
                if (i < 0)
                        i = 0;
                if (i < indirect_block)
                        goto repeat;
                ind = i + (u32 *) ind_bh->b_data;
                tmp = *ind;
                if (!tmp)
                        continue;
                if (inode->u.ext2_i.i_flags & EXT2_SECRM_FL)
                        bh = getblk (inode->i_dev, tmp,
                                     inode->i_sb->s_blocksize);
                else
                        bh = get_hash_table (inode->i_dev, tmp,
                                             inode->i_sb->s_blocksize);
                if (i < indirect_block) {
                        brelse (bh);
                        goto repeat;
                }
                if ((bh && bh->b_count != 1) || tmp != *ind) {
                        retry = 1;
                        brelse (bh);
                        continue;
                }
                *ind = 0;
                mark_buffer_dirty(ind_bh, 1);
                if (inode->u.ext2_i.i_flags & EXT2_SECRM_FL) {
                        memset(bh->b_data, RANDOM_INT, inode->i_sb->s_blocksize);
                        mark_buffer_dirty(bh, 1);
                }
                brelse (bh);
                if (free_count == 0) {
                        block_to_free = tmp;
                        free_count++;
                } else if (free_count > 0 && block_to_free == tmp - free_count)
                        free_count++;
                else {
                        ext2_free_blocks (inode->i_sb, block_to_free, free_count);
                        block_to_free = tmp;
                        free_count = 1;
                }
/*                ext2_free_blocks (inode->i_sb, tmp, 1); */
                inode->i_blocks -= blocks;
                inode->i_dirt = 1;
        }
        if (free_count > 0)
                ext2_free_blocks (inode->i_sb, block_to_free, free_count);
        ind = (u32 *) ind_bh->b_data;
        for (i = 0; i < addr_per_block; i++)
                if (*(ind++))
                        break;
        if (i >= addr_per_block)
                if (ind_bh->b_count != 1)
                        retry = 1;
                else {
                        tmp = *p;
                        *p = 0;
                        inode->i_blocks -= blocks;
                        inode->i_dirt = 1;
                        ext2_free_blocks (inode->i_sb, tmp, 1);
                }
        if (IS_SYNC(inode) && ind_bh->b_dirt) {
                ll_rw_block (WRITE, 1, __StackToFlat(&ind_bh));
                wait_on_buffer (ind_bh);
        }
        brelse (ind_bh);
        return retry;
}

static int trunc_dindirect (struct inode * inode, int offset,
                            u32 * p)
{
        int i, tmp;
        struct buffer_head * dind_bh;
        u32 * dind;
        int retry = 0;
        int addr_per_block = EXT2_ADDR_PER_BLOCK(inode->i_sb);
        int blocks = inode->i_sb->s_blocksize / 512;
#define DINDIRECT_BLOCK (((int)DIRECT_BLOCK - offset) / addr_per_block)
        int dindirect_block = DINDIRECT_BLOCK;

        tmp = *p;
        if (!tmp)
                return 0;
        dind_bh = bread (inode->i_dev, tmp, inode->i_sb->s_blocksize);
        if (tmp != *p) {
                brelse (dind_bh);
                return 1;
        }
        if (!dind_bh) {
                *p = 0;
                return 0;
        }
repeat:
        for (i = dindirect_block ; i < addr_per_block ; i++) {
                if (i < 0)
                        i = 0;
                if (i < dindirect_block)
                        goto repeat;
                dind = i + (u32 *) dind_bh->b_data;
                tmp = *dind;
                if (!tmp)
                        continue;
                retry |= trunc_indirect (inode, offset + (i * addr_per_block),
                                          dind);
                mark_buffer_dirty(dind_bh, 1);
        }
        dind = (u32 *) dind_bh->b_data;
        for (i = 0; i < addr_per_block; i++)
                if (*(dind++))
                        break;
        if (i >= addr_per_block)
                if (dind_bh->b_count != 1)
                        retry = 1;
                else {
                        tmp = *p;
                        *p = 0;
                        inode->i_blocks -= blocks;
                        inode->i_dirt = 1;
                        ext2_free_blocks (inode->i_sb, tmp, 1);
                }
        if (IS_SYNC(inode) && dind_bh->b_dirt) {
                ll_rw_block (WRITE, 1, __StackToFlat(&dind_bh));
                wait_on_buffer (dind_bh);
        }
        brelse (dind_bh);
        return retry;
}

static int trunc_tindirect (struct inode * inode)
{
        int i, tmp;
        struct buffer_head * tind_bh;
        u32 * tind, * p;
        int retry = 0;
        int addr_per_block = EXT2_ADDR_PER_BLOCK(inode->i_sb);
        int blocks = inode->i_sb->s_blocksize / 512;
#define TINDIRECT_BLOCK (((int)DIRECT_BLOCK - (addr_per_block * addr_per_block + \
                          addr_per_block + EXT2_NDIR_BLOCKS)) / \
                          (addr_per_block * addr_per_block))
        int tindirect_block = TINDIRECT_BLOCK;

        p = inode->u.ext2_i.i_data + EXT2_TIND_BLOCK;
        if (!(tmp = *p))
                return 0;
        tind_bh = bread (inode->i_dev, tmp, inode->i_sb->s_blocksize);
        if (tmp != *p) {
                brelse (tind_bh);
                return 1;
        }
        if (!tind_bh) {
                *p = 0;
                return 0;
        }
repeat:
        for (i = tindirect_block ; i < addr_per_block ; i++) {
                if (i < 0)
                        i = 0;
                if (i < tindirect_block)
                        goto repeat;
                tind = i + (u32 *) tind_bh->b_data;
                retry |= trunc_dindirect(inode, EXT2_NDIR_BLOCKS +
                        addr_per_block + (i + 1) * addr_per_block * addr_per_block,
                        tind);
                mark_buffer_dirty(tind_bh, 1);
        }
        tind = (u32 *) tind_bh->b_data;
        for (i = 0; i < addr_per_block; i++)
                if (*(tind++))
                        break;
        if (i >= addr_per_block)
                if (tind_bh->b_count != 1)
                        retry = 1;
                else {
                        tmp = *p;
                        *p = 0;
                        inode->i_blocks -= blocks;
                        inode->i_dirt = 1;
                        ext2_free_blocks (inode->i_sb, tmp, 1);
                }
        if (IS_SYNC(inode) && tind_bh->b_dirt) {
                ll_rw_block (WRITE, 1, __StackToFlat(&tind_bh));
                wait_on_buffer (tind_bh);
        }
        brelse (tind_bh);
        return retry;
}

void ext2_truncate (struct inode * inode)
{
        int retry;

#ifdef OS2
        long cnt = 0;
#endif
#if defined (OS2) && defined (FS_TRACE)
        kernel_printf("ext2_truncate( %lu ) - size = %lu nblks = %lu", inode->i_ino, inode->i_size, inode->i_blocks);
#endif
        if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
            S_ISLNK(inode->i_mode)))
                return;
        if (IS_APPEND(inode) || IS_IMMUTABLE(inode))
                return;
        ext2_discard_prealloc(inode);
        while (1) {
                down(&inode->i_sem);
                retry = trunc_direct(inode);
                retry |= trunc_indirect (inode, EXT2_IND_BLOCK,
                        (u32 *) &inode->u.ext2_i.i_data[EXT2_IND_BLOCK]);
                retry |= trunc_dindirect (inode, EXT2_IND_BLOCK +
                        EXT2_ADDR_PER_BLOCK(inode->i_sb),
                        (u32 *) &inode->u.ext2_i.i_data[EXT2_DIND_BLOCK]);
                retry |= trunc_tindirect (inode);
                up(&inode->i_sem);
                if (!retry)
                        break;
                if (IS_SYNC(inode) && inode->i_dirt)
                        ext2_sync_inode (inode);
#ifndef OS2
                current->counter = 0;
                schedule ();
#else
                if (!(cnt++%100))
                    kernel_printf("ext2_truncate - %lu retries", cnt);
                fsh32_yield();
#endif
        }
        inode->i_mtime = inode->i_ctime = CURRENT_TIME;
        inode->i_dirt = 1;
}
