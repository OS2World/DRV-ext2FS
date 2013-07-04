//
// $Header: e:\\netlabs.cvs\\ext2fs/src/ext2/inode.c,v 1.1 2001/05/09 17:53:02 umoeller Exp $
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

#ifdef __IBMC__
#pragma strings(readonly)
#endif


#ifdef OS2
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <linux\fs.h>
#include <linux\ext2_fs.h>
#include <linux\fs_proto.h>
#include <linux\ext2_proto.h>
#include <os2\os2proto.h>

#include <linux\stat.h>
#include <linux\errno.h>
#include <linux\sched.h>
#include <os2\errors.h>
#include <os2\log.h>
#include <linux\locks.h>
#endif


extern unsigned long event;




void ext2_put_inode (struct inode * inode)
{
#ifdef OS2
#ifdef FS_TRACE
        kernel_printf("ext2_put_inode - inode = %lu, i_nlink = %d", inode->i_ino, inode->i_nlink);
#endif
#endif
        ext2_discard_prealloc (inode);
        if (inode->i_nlink || inode->i_ino == EXT2_ACL_IDX_INO ||
            inode->i_ino == EXT2_ACL_DATA_INO)
                return;
        inode->i_size = 0;
        if (inode->i_blocks)
                ext2_truncate (inode);
        ext2_free_inode (inode);
}

#define inode_bmap(inode, nr) ((inode)->u.ext2_i.i_data[(nr)])

static int block_bmap (struct buffer_head * bh, int nr)
{
    int tmp;

    if (!bh)
        return 0;
    tmp = ((u32 *) bh->b_data)[nr];
    brelse (bh);
    return tmp;
}

/*
 * ext2_discard_prealloc and ext2_alloc_block are atomic wrt. the
 * superblock in the same manner as are ext2_free_blocks and
 * ext2_new_block.  We just wait on the super rather than locking it
 * here, since ext2_new_block will do the necessary locking and we
 * can't block until then.
 */
void ext2_discard_prealloc (struct inode *inode)
{
#ifdef EXT2_PREALLOCATE
        if (inode->u.ext2_i.i_prealloc_count) {
                int i = inode->u.ext2_i.i_prealloc_count;
                inode->u.ext2_i.i_prealloc_count = 0;
                ext2_free_blocks (inode->i_sb,
                                  inode->u.ext2_i.i_prealloc_block,
                                  i);
        }
#endif
}



struct buffer_head * ext2_bread (struct inode * inode, int block,
                                 int create, int *err)
{
        struct buffer_head * bh;
        bh = ext2_getblk (inode, block, create, err);
        if (!bh || bh->b_uptodate)
                return bh;
        ll_rw_block (READ, 1, __StackToFlat(&bh));
        wait_on_buffer (bh);
        if (bh->b_uptodate)
                return bh;
        brelse (bh);
        *err = -EIO;
        return NULL;

}

void ext2_read_inode (struct inode * inode)
{
        struct buffer_head * bh;
        struct ext2_inode * raw_inode;
        unsigned long block_group;
        unsigned long group_desc;
        unsigned long desc;
        unsigned long block;
        struct ext2_group_desc * gdp;

#ifdef FS_TRACE
        kernel_printf("ext2_read_inode( %lu )", inode->i_ino);
#endif

        if ((inode->i_ino != EXT2_ROOT_INO && inode->i_ino != EXT2_ACL_IDX_INO &&
             inode->i_ino != EXT2_ACL_DATA_INO && inode->i_ino < EXT2_FIRST_INO) ||
            inode->i_ino > inode->i_sb->u.ext2_sb.s_es->s_inodes_count) {
                ext2_error (inode->i_sb, "ext2_read_inode",
                            "bad inode number: %lu", inode->i_ino);
                return;
        }
        block_group = (inode->i_ino - 1) / EXT2_INODES_PER_GROUP(inode->i_sb);
        if (block_group >= inode->i_sb->u.ext2_sb.s_groups_count)
                ext2_panic (inode->i_sb, "ext2_read_inode",
                            "group >= groups count");
        group_desc = block_group / EXT2_DESC_PER_BLOCK(inode->i_sb);
        desc = block_group % EXT2_DESC_PER_BLOCK(inode->i_sb);
        bh = inode->i_sb->u.ext2_sb.s_group_desc[group_desc];
        if (!bh)
                ext2_panic (inode->i_sb, "ext2_read_inode",
                            "Descriptor not loaded");
        gdp = (struct ext2_group_desc *) bh->b_data;
        block = gdp[desc].bg_inode_table +
                (((inode->i_ino - 1) % EXT2_INODES_PER_GROUP(inode->i_sb))
                 / EXT2_INODES_PER_BLOCK(inode->i_sb));
        if (!(bh = bread (inode->i_dev, block, inode->i_sb->s_blocksize)))
                ext2_panic (inode->i_sb, "ext2_read_inode",
                            "unable to read i-node block - "
                            "inode=%lu, block=%lu", inode->i_ino, block);
        raw_inode = ((struct ext2_inode *) bh->b_data) +
                (inode->i_ino - 1) % EXT2_INODES_PER_BLOCK(inode->i_sb);
        inode->i_mode = raw_inode->i_mode;
        inode->i_uid = raw_inode->i_uid;
        inode->i_gid = raw_inode->i_gid;
        inode->i_nlink = raw_inode->i_links_count;
        inode->i_size = raw_inode->i_size;
        inode->i_atime = raw_inode->i_atime;
        inode->i_ctime = raw_inode->i_ctime;
        inode->i_mtime = raw_inode->i_mtime;
        inode->u.ext2_i.i_dtime = raw_inode->i_dtime;
        inode->i_blksize = inode->i_sb->s_blocksize;
        inode->i_blocks = raw_inode->i_blocks;
        inode->i_version = ++event;
        inode->u.ext2_i.i_flags = raw_inode->i_flags;
        inode->u.ext2_i.i_faddr = raw_inode->i_faddr;
        inode->u.ext2_i.i_frag_no = raw_inode->i_frag;
        inode->u.ext2_i.i_frag_size = raw_inode->i_fsize;
        inode->u.ext2_i.i_osync = 0;
        inode->u.ext2_i.i_file_acl = raw_inode->i_file_acl;
        inode->u.ext2_i.i_dir_acl = raw_inode->i_dir_acl;
        inode->u.ext2_i.i_version = raw_inode->i_version;
        inode->u.ext2_i.i_block_group = block_group;
        inode->u.ext2_i.i_next_alloc_block = 0;
        inode->u.ext2_i.i_next_alloc_goal = 0;
        if (inode->u.ext2_i.i_prealloc_count)
                ext2_error (inode->i_sb, "ext2_read_inode",
                            "New inode has non-zero prealloc count!");
        if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
                inode->i_rdev = raw_inode->i_block[0];
        else for (block = 0; block < EXT2_N_BLOCKS; block++)
                inode->u.ext2_i.i_data[block] = raw_inode->i_block[block];
        brelse (bh);
        inode->i_op = NULL;
        if (inode->i_ino == EXT2_ACL_IDX_INO ||
            inode->i_ino == EXT2_ACL_DATA_INO)
                /* Nothing to do */ ;
        else if (S_ISREG(inode->i_mode))
                inode->i_op = &ext2_file_inode_operations;
        else if (S_ISDIR(inode->i_mode))
                inode->i_op = &ext2_dir_inode_operations;
#ifndef OS2
        else if (S_ISLNK(inode->i_mode))
                inode->i_op = &ext2_symlink_inode_operations;
        else if (S_ISCHR(inode->i_mode))
                inode->i_op = &chrdev_inode_operations;
        else if (S_ISBLK(inode->i_mode))
                inode->i_op = &blkdev_inode_operations;
        else if (S_ISFIFO(inode->i_mode))
                init_fifo(inode);
#endif
        if (inode->u.ext2_i.i_flags & EXT2_SYNC_FL)
                inode->i_flags |= MS_SYNCHRONOUS;
        if (inode->u.ext2_i.i_flags & EXT2_APPEND_FL)
                inode->i_flags |= S_APPEND;
        if (inode->u.ext2_i.i_flags & EXT2_IMMUTABLE_FL)
                inode->i_flags |= S_IMMUTABLE;
}


static int ext2_alloc_block (struct inode * inode, unsigned long goal)
{
#ifdef EXT2FS_DEBUG
        static unsigned long alloc_hits = 0, alloc_attempts = 0;
#endif
        unsigned long result;
        struct buffer_head * bh;


        wait_on_super (inode->i_sb);


#ifdef EXT2_PREALLOCATE
        if (inode->u.ext2_i.i_prealloc_count &&
            (goal == inode->u.ext2_i.i_prealloc_block ||
             goal + 1 == inode->u.ext2_i.i_prealloc_block))
        {
                result = inode->u.ext2_i.i_prealloc_block++;
                inode->u.ext2_i.i_prealloc_count--;
#ifdef EXT2FS_DEBUG
                ext2_debug ("preallocation hit (%lu/%lu).\n",
                            ++alloc_hits, ++alloc_attempts);
#endif
                /* It doesn't matter if we block in getblk() since
                   we have already atomically allocated the block, and
                   are only clearing it now. */
                if (!(bh = getblk (inode->i_sb->s_dev, result,
                                   inode->i_sb->s_blocksize))) {

                        ext2_error (inode->i_sb, "ext2_alloc_block",
                                    "cannot get block %lu", result);
                        return 0;
                }
                memset(bh->b_data, 0, inode->i_sb->s_blocksize);
                bh->b_uptodate = 1;
                mark_buffer_dirty(bh, 1);
                brelse (bh);
        } else {
                ext2_discard_prealloc (inode);
#ifdef EXT2FS_DEBUG
                ext2_debug ("preallocation miss (%lu/%lu).\n",
                            alloc_hits, ++alloc_attempts);
#endif
                if (S_ISREG(inode->i_mode))
                        result = ext2_new_block
                                (inode->i_sb, goal,
                                 &inode->u.ext2_i.i_prealloc_count,
                                 &inode->u.ext2_i.i_prealloc_block);
                else
                        result = ext2_new_block (inode->i_sb, goal, 0, 0);
        }
#else
        result = ext2_new_block (inode->i_sb, goal, 0, 0);
#endif
        return result;
}


int ext2_bmap (struct inode * inode, int block)
{
    int i;
    int addr_per_block = EXT2_ADDR_PER_BLOCK(inode->i_sb);

    if (block < 0) {
        ext2_warning (inode->i_sb, "ext2_bmap", "block < 0");
        return 0;
    }
    if (block >= EXT2_NDIR_BLOCKS + addr_per_block +
             addr_per_block * addr_per_block +
             addr_per_block * addr_per_block * addr_per_block) {
        ext2_warning (inode->i_sb, "ext2_bmap", "block > big");
        return 0;
    }
    if (block < EXT2_NDIR_BLOCKS)
        return inode_bmap (inode, block);
    block -= EXT2_NDIR_BLOCKS;
    if (block < addr_per_block) {
        i = inode_bmap (inode, EXT2_IND_BLOCK);
        if (!i)
            return 0;
        return block_bmap (bread (inode->i_dev, i,
                      inode->i_sb->s_blocksize), block);
    }
    block -= addr_per_block;
    if (block < addr_per_block * addr_per_block) {
        i = inode_bmap (inode, EXT2_DIND_BLOCK);
        if (!i)
            return 0;
        i = block_bmap (bread (inode->i_dev, i,
                       inode->i_sb->s_blocksize),
                block / addr_per_block);
        if (!i)
            return 0;
        return block_bmap (bread (inode->i_dev, i,
                      inode->i_sb->s_blocksize),
                   block & (addr_per_block - 1));
    }
    block -= addr_per_block * addr_per_block;
    i = inode_bmap (inode, EXT2_TIND_BLOCK);
    if (!i)
        return 0;
    i = block_bmap (bread (inode->i_dev, i, inode->i_sb->s_blocksize),
            block / (addr_per_block * addr_per_block));
    if (!i)
        return 0;
    i = block_bmap (bread (inode->i_dev, i, inode->i_sb->s_blocksize),
            (block / addr_per_block) & (addr_per_block - 1));
    if (!i)
        return 0;
    return block_bmap (bread (inode->i_dev, i, inode->i_sb->s_blocksize),
               block & (addr_per_block - 1));
}



static struct buffer_head * inode_getblk (struct inode * inode, int nr,
                                          int create, int new_block, int * err)
{
        u32 * p;
        int tmp, goal = 0;
        struct buffer_head * result;
        int blocks = inode->i_sb->s_blocksize / 512;

        p = inode->u.ext2_i.i_data + nr;
repeat:
        tmp = *p;
        if (tmp) {
                result = getblk (inode->i_dev, tmp, inode->i_sb->s_blocksize);
                if (tmp == *p)
                        return result;
                brelse (result);
                goto repeat;
        }
        if (!create || new_block >=
            (current->rlim[RLIMIT_FSIZE].rlim_cur >>
             EXT2_BLOCK_SIZE_BITS(inode->i_sb))) {
                *err = -EFBIG;
                return NULL;
        }
        if (inode->u.ext2_i.i_next_alloc_block == new_block)
                goal = inode->u.ext2_i.i_next_alloc_goal;

#ifndef OS2
        ext2_debug ("hint = %d,", goal);
#endif
        if (!goal) {
                for (tmp = nr - 1; tmp >= 0; tmp--) {
                        if (inode->u.ext2_i.i_data[tmp]) {
                                goal = inode->u.ext2_i.i_data[tmp];
                                break;
                        }
                }
                if (!goal)
                        goal = (inode->u.ext2_i.i_block_group *
                                EXT2_BLOCKS_PER_GROUP(inode->i_sb)) +
                               inode->i_sb->u.ext2_sb.s_es->s_first_data_block;
        }

#ifndef OS2
        ext2_debug ("goal = %d.\n", goal);
#endif
        tmp = ext2_alloc_block (inode, goal);
        if (!tmp)
                return NULL;
        result = getblk (inode->i_dev, tmp, inode->i_sb->s_blocksize);
        if (*p) {
                ext2_free_blocks (inode->i_sb, tmp, 1);
                brelse (result);
                goto repeat;
        }
        *p = tmp;
        inode->u.ext2_i.i_next_alloc_block = new_block;
        inode->u.ext2_i.i_next_alloc_goal = tmp;
        inode->i_ctime = CURRENT_TIME;
        inode->i_blocks += blocks;

        if (IS_SYNC(inode) || inode->u.ext2_i.i_osync)
                ext2_sync_inode (inode);
        else
                inode->i_dirt = 1;
        return result;
}

static struct buffer_head * block_getblk (struct inode * inode,
                                          struct buffer_head * bh, int nr,
                                          int create, int blocksize,
                                          int new_block, int * err)
{
        int tmp, goal = 0;
        u32 * p;
        struct buffer_head * result;
        int blocks = inode->i_sb->s_blocksize / 512;

        if (!bh)
                return NULL;


        if (!bh->b_uptodate) {
                ll_rw_block (READ, 1, __StackToFlat(&bh));
                wait_on_buffer (bh);
                if (!bh->b_uptodate) {
                        brelse (bh);
                        return NULL;
                }
        }


        p = (u32 *) bh->b_data + nr;
repeat:
        tmp = *p;
        if (tmp) {
                result = getblk (bh->b_dev, tmp, blocksize);
                if (tmp == *p) {
                        brelse (bh);
                        return result;
                }
                brelse (result);
                goto repeat;
        }
        if (!create || new_block >=
            (current->rlim[RLIMIT_FSIZE].rlim_cur >>
             EXT2_BLOCK_SIZE_BITS(inode->i_sb))) {
                brelse (bh);
                *err = -EFBIG;
                return NULL;
        }
        if (inode->u.ext2_i.i_next_alloc_block == new_block)
                goal = inode->u.ext2_i.i_next_alloc_goal;
        if (!goal) {
                for (tmp = nr - 1; tmp >= 0; tmp--) {
                        if (((u32 *) bh->b_data)[tmp]) {
                                goal = ((u32 *)bh->b_data)[tmp];
                                break;
                        }
                }
                if (!goal)
                        goal = bh->b_blocknr;
        }
        tmp = ext2_alloc_block (inode, goal);
        if (!tmp) {
                brelse (bh);
                return NULL;
        }
        result = getblk (bh->b_dev, tmp, blocksize);
        if (*p) {
                ext2_free_blocks (inode->i_sb, tmp, 1);
                brelse (result);
                goto repeat;
        }
        *p = tmp;
        mark_buffer_dirty(bh, 1);

        if (IS_SYNC(inode) || inode->u.ext2_i.i_osync) {
                ll_rw_block (WRITE, 1, __StackToFlat(&bh));
                wait_on_buffer (bh);
        }

        inode->i_ctime = CURRENT_TIME;
        inode->i_blocks += blocks;
        inode->i_dirt = 1;
        inode->u.ext2_i.i_next_alloc_block = new_block;
        inode->u.ext2_i.i_next_alloc_goal = tmp;
        brelse (bh);
        return result;
}




struct buffer_head * ext2_getblk (struct inode * inode, long block,
                                  int create, int * err)
{
        struct buffer_head * bh;
        unsigned long b;
        unsigned long addr_per_block = EXT2_ADDR_PER_BLOCK(inode->i_sb);

#ifdef OS2
        //
        // This is for OS/2 OPEN_FLAGS_DASD support (direct access to the whole device)
        //
        if(inode->i_ino == INODE_DASD) {
                return getblk(inode->i_sb->s_dev, block, inode->i_sb->s_blocksize);
        }
#endif

        *err = -EIO;
        if (block < 0) {
                ext2_warning (inode->i_sb, "ext2_getblk", "block < 0");
                return NULL;
        }
        if (block > EXT2_NDIR_BLOCKS + addr_per_block  +
                    addr_per_block * addr_per_block +
                    addr_per_block * addr_per_block * addr_per_block) {
                ext2_warning (inode->i_sb, "ext2_getblk", "block > big");
                return NULL;
        }
        /*
         * If this is a sequential block allocation, set the next_alloc_block
         * to this block now so that all the indblock and data block
         * allocations use the same goal zone
         */
#ifndef OS2
        ext2_debug ("block %lu, next %lu, goal %lu.\n", block,
                    inode->u.ext2_i.i_next_alloc_block,
                    inode->u.ext2_i.i_next_alloc_goal);
#endif
        if (block == inode->u.ext2_i.i_next_alloc_block + 1) {
                inode->u.ext2_i.i_next_alloc_block++;
                inode->u.ext2_i.i_next_alloc_goal++;
        }

        *err = -ENOSPC;
        b = block;
        if (block < EXT2_NDIR_BLOCKS)
                return inode_getblk (inode, block, create, b, err);
        block -= EXT2_NDIR_BLOCKS;
        if (block < addr_per_block) {
                bh = inode_getblk (inode, EXT2_IND_BLOCK, create, b, err);
                return block_getblk (inode, bh, block, create,
                                     inode->i_sb->s_blocksize, b, err);
        }
        block -= addr_per_block;
        if (block < addr_per_block * addr_per_block) {
                bh = inode_getblk (inode, EXT2_DIND_BLOCK, create, b, err);
                bh = block_getblk (inode, bh, block / addr_per_block, create,
                                   inode->i_sb->s_blocksize, b, err);
                return block_getblk (inode, bh, block & (addr_per_block - 1),
                                     create, inode->i_sb->s_blocksize, b, err);
        }
        block -= addr_per_block * addr_per_block;
        bh = inode_getblk (inode, EXT2_TIND_BLOCK, create, b, err);
        bh = block_getblk (inode, bh, block/(addr_per_block * addr_per_block),
                           create, inode->i_sb->s_blocksize, b, err);
        bh = block_getblk (inode, bh, (block/addr_per_block) & (addr_per_block - 1),
                           create, inode->i_sb->s_blocksize, b, err);
        return block_getblk (inode, bh, block & (addr_per_block - 1), create,
                             inode->i_sb->s_blocksize, b, err);
}


static struct buffer_head * ext2_update_inode (struct inode * inode)
{
        struct buffer_head * bh;
        struct ext2_inode * raw_inode;
        unsigned long block_group;
        unsigned long group_desc;
        unsigned long desc;
        unsigned long block;
        struct ext2_group_desc * gdp;

        if ((inode->i_ino != EXT2_ROOT_INO && inode->i_ino < EXT2_FIRST_INO) ||
            inode->i_ino > inode->i_sb->u.ext2_sb.s_es->s_inodes_count) {
                ext2_error (inode->i_sb, "ext2_write_inode",
                            "bad inode number: %lu", inode->i_ino);
                return 0;
        }
        block_group = (inode->i_ino - 1) / EXT2_INODES_PER_GROUP(inode->i_sb);
        if (block_group >= inode->i_sb->u.ext2_sb.s_groups_count)
                ext2_panic (inode->i_sb, "ext2_write_inode",
                            "group >= groups count");
        group_desc = block_group / EXT2_DESC_PER_BLOCK(inode->i_sb);
        desc = block_group % EXT2_DESC_PER_BLOCK(inode->i_sb);
        bh = inode->i_sb->u.ext2_sb.s_group_desc[group_desc];
        if (!bh)
                ext2_panic (inode->i_sb, "ext2_write_inode",
                            "Descriptor not loaded");
        gdp = (struct ext2_group_desc *) bh->b_data;
        block = gdp[desc].bg_inode_table +
                (((inode->i_ino - 1) % EXT2_INODES_PER_GROUP(inode->i_sb))
                 / EXT2_INODES_PER_BLOCK(inode->i_sb));
        if (!(bh = bread (inode->i_dev, block, inode->i_sb->s_blocksize)))
                ext2_panic (inode->i_sb, "ext2_write_inode",
                            "unable to read i-node block - "
                            "inode=%lu, block=%lu", inode->i_ino, block);
        raw_inode = ((struct ext2_inode *)bh->b_data) +
                (inode->i_ino - 1) % EXT2_INODES_PER_BLOCK(inode->i_sb);
        raw_inode->i_mode = inode->i_mode;
        raw_inode->i_uid = inode->i_uid;
        raw_inode->i_gid = inode->i_gid;
        raw_inode->i_links_count = inode->i_nlink;
        raw_inode->i_size = inode->i_size;
        raw_inode->i_atime = inode->i_atime;
        raw_inode->i_ctime = inode->i_ctime;
        raw_inode->i_mtime = inode->i_mtime;
        raw_inode->i_blocks = inode->i_blocks;
        raw_inode->i_dtime = inode->u.ext2_i.i_dtime;
        raw_inode->i_flags = inode->u.ext2_i.i_flags;
        raw_inode->i_faddr = inode->u.ext2_i.i_faddr;
        raw_inode->i_frag = inode->u.ext2_i.i_frag_no;
        raw_inode->i_fsize = inode->u.ext2_i.i_frag_size;
        raw_inode->i_file_acl = inode->u.ext2_i.i_file_acl;
        raw_inode->i_dir_acl = inode->u.ext2_i.i_dir_acl;
        raw_inode->i_version = inode->u.ext2_i.i_version;
        if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
                raw_inode->i_block[0] = inode->i_rdev;
        else for (block = 0; block < EXT2_N_BLOCKS; block++)
                raw_inode->i_block[block] = inode->u.ext2_i.i_data[block];
        mark_buffer_dirty(bh, 1);
        inode->i_dirt = 0;
        return bh;
}

void ext2_write_inode (struct inode * inode)
{
        struct buffer_head * bh;
        bh = ext2_update_inode (inode);
        brelse (bh);
}

int ext2_sync_inode (struct inode *inode)
{
        int err = 0;
        struct buffer_head *bh;

        bh = ext2_update_inode (inode);
        if (bh && bh->b_dirt)
        {
                ll_rw_block (WRITE, 1, __StackToFlat(&bh));
                wait_on_buffer (bh);
#ifndef OS2        // ll_rw_block is SYNCHRONOUS on OS/2
                if (bh->b_req && !bh->b_uptodate)
                {
                        printk ("IO error syncing ext2 inode [%04x:%08lx]\n",
                                inode->i_dev, inode->i_ino);
                        err = -1;
                }
#endif
        }
        else if (!bh)
                err = -1;
        brelse (bh);
        return err;
}
