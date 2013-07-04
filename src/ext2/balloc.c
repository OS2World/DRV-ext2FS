//
// $Header: e:\\netlabs.cvs\\ext2fs/src/ext2/balloc.c,v 1.1 2001/05/09 17:53:01 umoeller Exp $
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
 *  linux/fs/ext2/balloc.c
 *
 *  Copyright (C) 1992, 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                                  Laboratoire MASI - Institut Blaise Pascal
 *                                  Universite Pierre et Marie Curie (Paris VI)
 *
 *  Enhanced block allocation by Stephen Tweedie (sct@dcs.ed.ac.uk), 1993
 */

/*
 * balloc.c contains the blocks allocation and deallocation routines
 */

/*
 * The free blocks are managed by bitmaps.  A file system contains several
 * blocks groups.  Each group contains 1 bitmap block for blocks, 1 bitmap
 * block for inodes, N blocks for the inode table and data blocks.
 *
 * The file system contains group descriptors which are located after the
 * super block.  Each descriptor contains the number of the bitmap block and
 * the free blocks count in the block.  The descriptors are loaded in memory
 * when a file system is mounted (see ext2_read_super).
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#ifdef OS2

#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>                // Standard MS Visual C++ string.h

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\os2proto.h>

#include <linux\fs.h>
#include <linux\ext2_fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_proto.h>
#include <linux\stat.h>
#include <linux\sched.h>
#include <linux\locks.h>

#include <asm\bitops.h>

#include <os2\errors.h>

#include <os2\log.h>

#else // OS2

#include <linux\fs.h>
#include <linux\ext2_fs.h>
#include <linux\stat.h>
#include <linux\sched.h>
#include <linux\string.h>
#include <linux\locks.h>

#include <asm\bitops.h>
#endif

#define in_range(b, first, len)                ((b) >= (first) && (b) <= (first) + (len) - 1)

static struct ext2_group_desc * get_group_desc (struct super_block * sb,
                                                unsigned int block_group,
                                                struct buffer_head ** bh)
{
        unsigned long group_desc;
        unsigned long desc;
        struct ext2_group_desc * gdp;

        if (block_group >= sb->u.ext2_sb.s_groups_count)
                ext2_panic (sb, "get_group_desc",
                            "block_group >= groups_count - "
                            "block_group = %d, groups_count = %lu",
                            block_group, sb->u.ext2_sb.s_groups_count);

        group_desc = block_group / EXT2_DESC_PER_BLOCK(sb);
        desc = block_group % EXT2_DESC_PER_BLOCK(sb);
        if (!sb->u.ext2_sb.s_group_desc[group_desc])
                ext2_panic (sb, "get_group_desc",
                            "Group descriptor not loaded - "
                            "block_group = %d, group_desc = %lu, desc = %lu",
                             block_group, group_desc, desc);
        gdp = (struct ext2_group_desc *)
              sb->u.ext2_sb.s_group_desc[group_desc]->b_data;
        if (bh)
                *bh = sb->u.ext2_sb.s_group_desc[group_desc];
        return gdp + desc;
}

static void read_block_bitmap (struct super_block * sb,
                               unsigned int block_group,
                               unsigned long bitmap_nr)
{
        struct ext2_group_desc * gdp;
        struct buffer_head * bh;

        gdp = get_group_desc (sb, block_group, NULL);
        bh = bread (sb->s_dev, gdp->bg_block_bitmap, sb->s_blocksize);
        if (!bh)
                ext2_panic (sb, "read_block_bitmap",
                            "Cannot read block bitmap - "
                            "block_group = %d, block_bitmap = %lu",
                            block_group, (unsigned long) gdp->bg_block_bitmap);
        sb->u.ext2_sb.s_block_bitmap_number[bitmap_nr] = block_group;
        sb->u.ext2_sb.s_block_bitmap[bitmap_nr] = bh;
}

/*
 * load_block_bitmap loads the block bitmap for a blocks group
 *
 * It maintains a cache for the last bitmaps loaded.  This cache is managed
 * with a LRU algorithm.
 *
 * Notes:
 * 1/ There is one cache per mounted file system.
 * 2/ If the file system contains less than EXT2_MAX_GROUP_LOADED groups,
 *    this function reads the bitmap without maintaining a LRU cache.
 */
static int load__block_bitmap (struct super_block * sb,
                               unsigned int block_group)
{
        int i, j;
        unsigned long block_bitmap_number;
        struct buffer_head * block_bitmap;

        if (block_group >= sb->u.ext2_sb.s_groups_count)
                ext2_panic (sb, "load_block_bitmap",
                            "block_group >= groups_count - "
                            "block_group = %d, groups_count = %lu",
                            block_group, sb->u.ext2_sb.s_groups_count);

        if (sb->u.ext2_sb.s_groups_count <= EXT2_MAX_GROUP_LOADED) {
                if (sb->u.ext2_sb.s_block_bitmap[block_group]) {
                        if (sb->u.ext2_sb.s_block_bitmap_number[block_group] !=
                            block_group)
                                ext2_panic (sb, "load_block_bitmap",
                                            "block_group != block_bitmap_number");
                        else
                                return block_group;
                } else {
                        read_block_bitmap (sb, block_group, block_group);
                        return block_group;
                }
        }

        for (i = 0; i < sb->u.ext2_sb.s_loaded_block_bitmaps &&
                    sb->u.ext2_sb.s_block_bitmap_number[i] != block_group; i++)
                ;
        if (i < sb->u.ext2_sb.s_loaded_block_bitmaps &&
              sb->u.ext2_sb.s_block_bitmap_number[i] == block_group) {
                block_bitmap_number = sb->u.ext2_sb.s_block_bitmap_number[i];
                block_bitmap = sb->u.ext2_sb.s_block_bitmap[i];
                for (j = i; j > 0; j--) {
                        sb->u.ext2_sb.s_block_bitmap_number[j] =
                                sb->u.ext2_sb.s_block_bitmap_number[j - 1];
                        sb->u.ext2_sb.s_block_bitmap[j] =
                                sb->u.ext2_sb.s_block_bitmap[j - 1];
                }
                sb->u.ext2_sb.s_block_bitmap_number[0] = block_bitmap_number;
                sb->u.ext2_sb.s_block_bitmap[0] = block_bitmap;
        } else {
                if (sb->u.ext2_sb.s_loaded_block_bitmaps < EXT2_MAX_GROUP_LOADED)
                        sb->u.ext2_sb.s_loaded_block_bitmaps++;
                else
                        brelse (sb->u.ext2_sb.s_block_bitmap[EXT2_MAX_GROUP_LOADED - 1]);
                for (j = sb->u.ext2_sb.s_loaded_block_bitmaps - 1; j > 0;  j--) {
                        sb->u.ext2_sb.s_block_bitmap_number[j] =
                                sb->u.ext2_sb.s_block_bitmap_number[j - 1];
                        sb->u.ext2_sb.s_block_bitmap[j] =
                                sb->u.ext2_sb.s_block_bitmap[j - 1];
                }
                read_block_bitmap (sb, block_group, 0);
        }
        return 0;
}

#ifdef OS2
INLINE int load_block_bitmap (struct super_block *sb,
                                     unsigned int block_group)
#else
INLINE int load_block_bitmap (struct super_block *sb,
                                     unsigned int block_group)
#endif
{
        if (sb->u.ext2_sb.s_loaded_block_bitmaps > 0 &&
            sb->u.ext2_sb.s_block_bitmap_number[0] == block_group)
                return 0;

        if (sb->u.ext2_sb.s_groups_count <= EXT2_MAX_GROUP_LOADED &&
            sb->u.ext2_sb.s_block_bitmap_number[block_group] == block_group &&
            sb->u.ext2_sb.s_block_bitmap[block_group])
                return block_group;

        return load__block_bitmap (sb, block_group);
}

void ext2_free_blocks (struct super_block * sb, unsigned long block,
                       unsigned long count)
{
        struct buffer_head * bh;
        struct buffer_head * bh2;
        unsigned long block_group;
        unsigned long bit;
        unsigned long i;
        int bitmap_nr;
        struct ext2_group_desc * gdp;
        struct ext2_super_block * es;

        if (!sb) {
                printk ("ext2_free_blocks: nonexistent device");
                return;
        }
        lock_super (sb);
        es = sb->u.ext2_sb.s_es;
        if (block < es->s_first_data_block ||
            (block + count) > es->s_blocks_count) {
                ext2_error (sb, "ext2_free_blocks",
                            "Freeing blocks not in datazone - "
                            "block = %lu, count = %lu", block, count);
                unlock_super (sb);
                return;
        }

        ext2_debug ("freeing block %lu\n", block);

        block_group = (block - es->s_first_data_block) /
                      EXT2_BLOCKS_PER_GROUP(sb);
        bit = (block - es->s_first_data_block) % EXT2_BLOCKS_PER_GROUP(sb);
        if (bit + count > EXT2_BLOCKS_PER_GROUP(sb))
                ext2_panic (sb, "ext2_free_blocks",
                            "Freeing blocks across group boundary - "
                            "Block = %lu, count = %lu",
                            block, count);
        bitmap_nr = load_block_bitmap (sb, block_group);
        bh = sb->u.ext2_sb.s_block_bitmap[bitmap_nr];
        gdp = get_group_desc (sb, block_group, __StackToFlat(&bh2));

        if (test_opt (sb, CHECK_STRICT) &&
            (in_range (gdp->bg_block_bitmap, block, count) ||
             in_range (gdp->bg_inode_bitmap, block, count) ||
             in_range (block, gdp->bg_inode_table,
                       sb->u.ext2_sb.s_itb_per_group) ||
             in_range (block + count - 1, gdp->bg_inode_table,
                       sb->u.ext2_sb.s_itb_per_group)))
                ext2_panic (sb, "ext2_free_blocks",
                            "Freeing blocks in system zones - "
                            "Block = %lu, count = %lu",
                            block, count);

        for (i = 0; i < count; i++) {
#ifdef OS2
                if (!clear_bit (bit + i, (int *)bh->b_data))
#else
                if (!clear_bit (bit + i, bh->b_data))
#endif
                        ext2_warning (sb, "ext2_free_blocks",
                                      "bit already cleared for block %lu",
                                      block);
                else {
                        gdp->bg_free_blocks_count++;
                        es->s_free_blocks_count++;
                }
        }

        mark_buffer_dirty(bh2, 1);
        mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);

        mark_buffer_dirty(bh, 1);

        if (sb->s_flags & MS_SYNCHRONOUS) {
                ll_rw_block (WRITE, 1, __StackToFlat(&bh));
                wait_on_buffer (bh);
        }

        sb->s_dirt = 1;
        unlock_super (sb);
        return;
}

/*
 * ext2_new_block uses a goal block to assist allocation.  If the goal is
 * free, or there is a free block within 32 blocks of the goal, that block
 * is allocated.  Otherwise a forward search is made for a free block; within
 * each block group the search first looks for an entire free byte in the block
 * bitmap, and then for any free bit if that fails.
 */
int ext2_new_block (struct super_block * sb, unsigned long goal,
                    u32 * prealloc_count,
                    u32 * prealloc_block)
{
        struct buffer_head * bh;
        struct buffer_head * bh2;
        char * p, * r;
        int i, j, k, tmp;
        unsigned long lmap;
        int bitmap_nr;
        struct ext2_group_desc * gdp;
        struct ext2_super_block * es;

#ifdef EXT2FS_DEBUG
        static int goal_hits = 0, goal_attempts = 0;
#endif
        if (!sb) {
                printk ("ext2_new_block: nonexistent device");
                return 0;
        }
        lock_super (sb);
        es = sb->u.ext2_sb.s_es;
#ifdef OS2
        if (es->s_free_blocks_count <= es->s_r_blocks_count) {
                kernel_printf("WARNING : ext2_new_block() - free block count (%lu) <= reserved block count (%lu)", es->s_free_blocks_count, es->s_r_blocks_count);
                unlock_super (sb);
                return 0;
        }
#else
        if (es->s_free_blocks_count <= es->s_r_blocks_count &&
            (!fsuser() && (sb->u.ext2_sb.s_resuid != current->fsuid) &&
             (sb->u.ext2_sb.s_resgid == 0 ||
              !in_group_p (sb->u.ext2_sb.s_resgid)))) {
                unlock_super (sb);
                return 0;
        }
#endif
#ifndef OS2
        ext2_debug ("goal=%lu.\n", goal);
#endif
repeat:
        /*
         * First, test whether the goal block is free.
         */
        if (goal < es->s_first_data_block || goal >= es->s_blocks_count)
                goal = es->s_first_data_block;
        i = (goal - es->s_first_data_block) / EXT2_BLOCKS_PER_GROUP(sb);
        gdp = get_group_desc (sb, i, __StackToFlat(&bh2));
        if (gdp->bg_free_blocks_count > 0) {
                j = ((goal - es->s_first_data_block) % EXT2_BLOCKS_PER_GROUP(sb));
#ifdef EXT2FS_DEBUG
                if (j)
                        goal_attempts++;
#endif
                bitmap_nr = load_block_bitmap (sb, i);
                bh = sb->u.ext2_sb.s_block_bitmap[bitmap_nr];
#ifndef OS2
                ext2_debug ("goal is at %d:%d.\n", i, j);
#endif
#ifdef OS2
                if (!test_bit(j, (int *)bh->b_data)) {
#else
                if (!test_bit(j, bh->b_data)) {
#endif
#ifdef EXT2FS_DEBUG
                        goal_hits++;
                        ext2_debug ("goal bit allocated.\n");
#endif
                        goto got_block;
                }
#ifndef XXXOS2
                if (j) {
                        /*
                         * The goal was occupied; search forward for a free
                         * block within the next 32 blocks
                         */
                        lmap = ((((unsigned long *) bh->b_data)[j >> 5]) >>
                                ((j & 31) + 1));
                        if (j < EXT2_BLOCKS_PER_GROUP(sb) - 32)
                                lmap |= (((unsigned long *) bh->b_data)[(j >> 5) + 1]) <<
                                 (31 - (j & 31));
                        else
                                lmap |= 0xffffffff << (31 - (j & 31));
                        if (lmap != 0xffffffffl) {
                                k = ffz(lmap) + 1;
                                if ((j + k) < EXT2_BLOCKS_PER_GROUP(sb)) {
                                        j += k;
                                        goto got_block;
                                }
                        }
                }
#else
                if (j) {
                        /*
                         * The goal was occupied; search forward for a free
                         * block within the next 32 blocks
                         */
                        lmap = ((((unsigned long *) bh->b_data)[j >> 5UL]) >>
                                ((j & 31UL) + 1UL));
                        if (j < EXT2_BLOCKS_PER_GROUP(sb) - 32UL)
                                lmap |= (((unsigned long *) bh->b_data)[(j >> 5UL) + 1UL]) <<
                                 (31UL - (j & 31UL));
                        else
                                lmap |= 0xffffffffUL << (31UL - (j & 31UL));
                        if (lmap != 0xffffffffUL) {
                                k = ffz(lmap) + 1UL;
                                if ((j + k) < EXT2_BLOCKS_PER_GROUP(sb)) {
                                        j += k;
                                        goto got_block;
                                }
                        }
                }
#endif
                ext2_debug ("Bit not found near goal\n");

                /*
                 * There has been no free block found in the near vicinity
                 * of the goal: do a search forward through the block groups,
                 * searching in each group first for an entire free byte in
                 * the bitmap and then for any free bit.
                 *
                 * Search first in the remainder of the current group; then,
                 * cyclicly search through the rest of the groups.
                 */
                p = ((char *) bh->b_data) + (j >> 3);
                r = memscan(p, 0, (EXT2_BLOCKS_PER_GROUP(sb) - j + 7) >> 3);
                k = (r - ((char *) bh->b_data)) << 3;
                if (k < EXT2_BLOCKS_PER_GROUP(sb)) {
                        j = k;
                        goto search_back;
                }
                k = find_next_zero_bit ((unsigned long *) bh->b_data,
                                        EXT2_BLOCKS_PER_GROUP(sb),
                                        j);
                if (k < EXT2_BLOCKS_PER_GROUP(sb)) {
                        j = k;
                        goto got_block;
                }
        }

        ext2_debug ("Bit not found in block group %d.\n", i);

        /*
         * Now search the rest of the groups.  We assume that
         * i and gdp correctly point to the last group visited.
         */
        for (k = 0; k < sb->u.ext2_sb.s_groups_count; k++) {
                i++;
                if (i >= sb->u.ext2_sb.s_groups_count)
                        i = 0;
                gdp = get_group_desc (sb, i, __StackToFlat(&bh2));
                if (gdp->bg_free_blocks_count > 0)
                        break;
        }
        if (k >= sb->u.ext2_sb.s_groups_count) {
                unlock_super (sb);
                kernel_printf("ext2_newx_block() - k (%lu) >= sb->u.ext2_sb.s_groups_count (%lu)", k, sb->u.ext2_sb.s_groups_count);
                return 0;
        }
        bitmap_nr = load_block_bitmap (sb, i);
        bh = sb->u.ext2_sb.s_block_bitmap[bitmap_nr];
        r = memscan(bh->b_data, 0, EXT2_BLOCKS_PER_GROUP(sb) >> 3);
        j = (r - bh->b_data) << 3;
        if (j < EXT2_BLOCKS_PER_GROUP(sb))
                goto search_back;
        else
                j = find_first_zero_bit ((unsigned long *) bh->b_data,
                                         EXT2_BLOCKS_PER_GROUP(sb));
        if (j >= EXT2_BLOCKS_PER_GROUP(sb)) {
                ext2_error (sb, "ext2_new_block",
                            "Free blocks count corrupted for block group %d", i);
                unlock_super (sb);
                return 0;
        }

search_back:
        /*
         * We have succeeded in finding a free byte in the block
         * bitmap.  Now search backwards up to 7 bits to find the
         * start of this group of free blocks.
         */
#ifndef OS2
        for (k = 0; k < 7 && j > 0 && !test_bit (j - 1, bh->b_data); k++, j--);
#else
        for (k = 0; k < 7 && j > 0 && !test_bit (j - 1, (int *)bh->b_data); k++, j--);
#endif

got_block:

        ext2_debug ("using block group %d(%d)\n", i, gdp->bg_free_blocks_count);

        tmp = j + i * EXT2_BLOCKS_PER_GROUP(sb) + es->s_first_data_block;

        if (test_opt (sb, CHECK_STRICT) &&
            (tmp == gdp->bg_block_bitmap ||
             tmp == gdp->bg_inode_bitmap ||
             in_range (tmp, gdp->bg_inode_table, sb->u.ext2_sb.s_itb_per_group)))
                ext2_panic (sb, "ext2_new_block",
                            "Allocating block in system zone - "
                            "block = %u", tmp);
#ifndef OS2
        if (set_bit (j, bh->b_data)) {
#else
        if (set_bit (j, (int *)bh->b_data)) {
#endif
                ext2_panic (sb, "ext2_new_block",
                              "bit already set for block %lu - bitmap = %lu, bit %lu", j, ((UINT32*)bh->b_data)[j / 32], j % 32);
                goto repeat;
        }
        ext2_debug ("found bit %d\n", j);

        /*
         * Do block preallocation now if required.
         */
#ifdef EXT2_PREALLOCATE
        if (prealloc_block) {
                *prealloc_count = 0;
                *prealloc_block = tmp + 1;
                for (k = 1;
                     k < 8 && (j + k) < EXT2_BLOCKS_PER_GROUP(sb); k++) {
#ifndef OS2
                        if (set_bit (j + k, bh->b_data))
#else
                        if (set_bit (j + k, (PINT32)bh->b_data))
#endif
                                break;
                        (*prealloc_count)++;
                }
                gdp->bg_free_blocks_count -= *prealloc_count;
                es->s_free_blocks_count -= *prealloc_count;
                ext2_debug ("Preallocated a further %lu bits.\n",
                            *prealloc_count);
        }
#endif

        j = tmp;

        mark_buffer_dirty(bh, 1);

        if (sb->s_flags & MS_SYNCHRONOUS) {
                ll_rw_block (WRITE, 1, __StackToFlat(&bh));
                wait_on_buffer (bh);
        }

        if (j >= es->s_blocks_count) {
                ext2_error (sb, "ext2_new_block",
                            "block >= blocks count - "
                            "block_group = %d, block=%d", i, j);
                unlock_super (sb);
                return 0;
        }
        if (!(bh = getblk (sb->s_dev, j, sb->s_blocksize))) {
                ext2_error (sb, "ext2_new_block", "cannot get block %d", j);
                unlock_super (sb);
                return 0;
        }
        memset(bh->b_data, 0, sb->s_blocksize);
        bh->b_uptodate = 1;
        mark_buffer_dirty(bh, 1);
        brelse (bh);
#ifdef EXT2_FS_DEBUG
        ext2_debug ("allocating block %d. "
                    "Goal hits %d of %d.\n", j, goal_hits, goal_attempts);
#endif
        gdp->bg_free_blocks_count--;
        mark_buffer_dirty(bh2, 1);
        es->s_free_blocks_count--;
        mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
        sb->s_dirt = 1;
        unlock_super (sb);
        return j;
}

unsigned long ext2_count_free_blocks (struct super_block * sb)
{
#ifdef EXT2FS_DEBUG
        struct ext2_super_block * es;
        unsigned long desc_count, bitmap_count, x;
        int bitmap_nr;
        struct ext2_group_desc * gdp;
        int i;

        lock_super (sb);
        es = sb->u.ext2_sb.s_es;
        desc_count = 0;
        bitmap_count = 0;
        gdp = NULL;
        for (i = 0; i < sb->u.ext2_sb.s_groups_count; i++) {
                gdp = get_group_desc (sb, i, NULL);
                desc_count += gdp->bg_free_blocks_count;
                bitmap_nr = load_block_bitmap (sb, i);
                x = ext2_count_free (sb->u.ext2_sb.s_block_bitmap[bitmap_nr],
                                     sb->s_blocksize);
                printk ("group %d: stored = %d, counted = %lu\n",
                        i, gdp->bg_free_blocks_count, x);
                bitmap_count += x;
        }
        printk("ext2_count_free_blocks: stored = %lu, computed = %lu, %lu\n",
               es->s_free_blocks_count, desc_count, bitmap_count);
        unlock_super (sb);
        return bitmap_count;
#else
        return sb->u.ext2_sb.s_es->s_free_blocks_count;
#endif
}

#ifndef OS2
static inline int block_in_use (unsigned long block,
                                struct super_block * sb,
                                unsigned char * map)
{
        return test_bit ((block - sb->u.ext2_sb.s_es->s_first_data_block) %
                         EXT2_BLOCKS_PER_GROUP(sb), map);
}
#else
INLINE INT32 block_in_use ( unsigned long block,
                                struct super_block * sb,
                                unsigned char *map)
{
//        return test_bit ((INT32)((block - sb->u.ext2_sb.s_es->s_first_data_block) %
//                         EXT2_BLOCKS_PER_GROUP(sb)), (PINT32)map);
        return test_bit ((block - sb->u.ext2_sb.s_es->s_first_data_block) %
                         EXT2_BLOCKS_PER_GROUP(sb), (int *)map);


}
#endif
void ext2_check_blocks_bitmap (struct super_block * sb)
{
        struct buffer_head * bh;
        struct ext2_super_block * es;
        unsigned long desc_count, bitmap_count, x;
        unsigned long desc_blocks;
        int bitmap_nr;
        struct ext2_group_desc * gdp;
        int i, j;

        lock_super (sb);
        es = sb->u.ext2_sb.s_es;
        desc_count = 0;
        bitmap_count = 0;
        gdp = NULL;
        desc_blocks = (sb->u.ext2_sb.s_groups_count + EXT2_DESC_PER_BLOCK(sb) - 1) /
                      EXT2_DESC_PER_BLOCK(sb);
        for (i = 0; i < sb->u.ext2_sb.s_groups_count; i++) {
                gdp = get_group_desc (sb, i, NULL);
                desc_count += gdp->bg_free_blocks_count;
                bitmap_nr = load_block_bitmap (sb, i);
                bh = sb->u.ext2_sb.s_block_bitmap[bitmap_nr];
#ifndef OS2
                if (!test_bit (0, bh->b_data))
#else
                if (!test_bit (0, (int *)bh->b_data))
#endif
                        ext2_error (sb, "ext2_check_blocks_bitmap",
                                    "Superblock in group %d is marked free", i);

                for (j = 0; j < desc_blocks; j++)
#ifndef OS2
                        if (!test_bit (j + 1, bh->b_data))
#else
                        if (!test_bit (j + 1, (int *)bh->b_data))
#endif
                                ext2_error (sb, "ext2_check_blocks_bitmap",
                                            "Descriptor block #%d in group "
                                            "%d is marked free", j, i);

                if (!block_in_use (gdp->bg_block_bitmap, sb, bh->b_data))
                        ext2_error (sb, "ext2_check_blocks_bitmap",
                                    "Block bitmap for group %d is marked free",
                                    i);

                if (!block_in_use (gdp->bg_inode_bitmap, sb, bh->b_data))
                        ext2_error (sb, "ext2_check_blocks_bitmap",
                                    "Inode bitmap for group %d is marked free",
                                    i);

                for (j = 0; j < sb->u.ext2_sb.s_itb_per_group; j++)
                        if (!block_in_use (gdp->bg_inode_table + j, sb, bh->b_data))
                                ext2_error (sb, "ext2_check_blocks_bitmap",
                                            "Block #%d of the inode table in "
                                            "group %d is marked free", j, i);

                x = ext2_count_free (bh, sb->s_blocksize);
                if (gdp->bg_free_blocks_count != x)
                        ext2_error (sb, "ext2_check_blocks_bitmap",
                                    "Wrong free blocks count for group %d, "
                                    "stored = %d, counted = %lu", i,
                                    gdp->bg_free_blocks_count, x);
                bitmap_count += x;
        }
        if (es->s_free_blocks_count != bitmap_count)
                ext2_error (sb, "ext2_check_blocks_bitmap",
                            "Wrong free blocks count in super block, "
                            "stored = %lu, counted = %lu",
                            (unsigned long) es->s_free_blocks_count, bitmap_count);
        unlock_super (sb);
}
