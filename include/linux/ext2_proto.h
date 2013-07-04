//
// $Header: e:\\netlabs.cvs\\ext2fs/include/linux/ext2_proto.h,v 1.1 2001/05/09 17:43:38 umoeller Exp $
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



#ifndef __E2_PROTO_H
#define __E2_PROTO_H

//
// ext2/super.c
//
extern void ext2_panic (struct super_block * sb, const char *function,
			    const char *fmt, ...);

extern void ext2_error (struct super_block * sb, const char *function,
			    const char *fmt, ...);

extern void ext2_warning (struct super_block * sb, const char *function,
		   const char *fmt, ...);

void ext2_put_super (struct super_block * sb);

//
// ext2/inode.c
//
extern void ext2_discard_prealloc (struct inode * inode);
#ifndef MINIFSD
extern struct buffer_head * ext2_getblk (struct inode * inode, long block,
                                         int create, int * err);
#else
extern struct buffer_head * ext2_getblk (struct inode * inode, blk_t block,   
                                         blk_t create, int *err);              
#endif
#ifndef MINIFSD
extern struct buffer_head * ext2_bread (struct inode * inode, int block,
                                        int create, int *err);
#else
extern struct buffer_head * ext2_bread (struct inode * inode, blk_t block, 
                                        blk_t create, int *err);            
#endif
extern void ext2_write_inode (struct inode * inode);
extern void ext2_read_inode(struct inode *inode);
extern int ext2_sync_inode (struct inode *inode);
extern void ext2_put_inode (struct inode * inode);
extern int ext2_bmap (struct inode * inode, int block);

//
// ext2/bitmap.c
//
extern unsigned long ext2_count_free (pbuffer_head map, unsigned long numchars);

//
// ext2/balloc.c
//
extern void ext2_free_blocks (struct super_block * sb, unsigned long block,
		       unsigned long count);
extern int ext2_new_block (struct super_block * sb, unsigned long goal,
                           u32 * prealloc_count,
                           u32 * prealloc_block);
void ext2_check_blocks_bitmap (struct super_block * sb);

//
// ext2/dir.c
//
int ext2_check_dir_entry (char * function, struct inode * dir,
			  struct ext2_dir_entry * de, struct buffer_head * bh,
			  unsigned long offset);

//
// ext2/ialloc.c
//
extern struct inode * ext2_new_inode (const struct inode * dir, int mode);
extern void ext2_free_inode (struct inode * inode);
extern void ext2_check_inodes_bitmap (struct super_block * sb);

//
// ext2/namei.c
//
int ext2_mkdir (struct inode * dir, const char * name, int len, int mode);
int ext2_rmdir (struct inode * dir, const char * name, int len);
int ext2_create (struct inode * dir,const char * name, int len, int mode,
		 struct inode ** result);
int ext2_unlink (struct inode * dir, const char * name, int len);
int ext2_rename (struct inode * old_dir, const char * old_name, int old_len,
		 struct inode * new_dir, const char * new_name, int new_len);
int ext2_lookup (struct inode * dir, const char * name, int len,
                 struct inode ** result);
int ext2_link (struct inode * oldinode, struct inode * dir,
               const char * name, int len);
//
// ext2/truncate.c
//
void ext2_truncate (struct inode * inode);


//
// ext2/fsync.c
//
int ext2_sync_file (struct inode * inode, struct file * file);

//
// ext2/ioctl.c
//
int ext2_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
		unsigned long arg);

#endif
