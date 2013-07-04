//
// $Header: e:\\netlabs.cvs\\ext2fs/include/linux/fs_proto.h,v 1.1 2001/05/09 17:43:38 umoeller Exp $
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




#ifndef _FS_PROTO_H
#define _FS_PROTO_H

//
// From vfs/buffer.c
//
#if defined(MINIFSD)
extern struct buffer_head * (*bread)(dev_t, blk_t, unsigned long);
extern void (*brelse)(struct buffer_head *);
extern struct buffer_head * (*getblk)(dev_t, blk_t, unsigned long);
extern struct buffer_head *get_hash_table(dev_t dev, blk_t block, unsigned long size);
#else
extern void brelse(struct buffer_head *buf);
extern void bforget(struct buffer_head *buf);
extern struct buffer_head * bread(dev_t dev, int block, int size);
extern struct buffer_head *bread_swapper(dev_t dev, blk_t block, unsigned long size);
extern struct buffer_head * getblk(dev_t dev, int block, int size);
extern struct buffer_head *get_hash_table(dev_t dev, int block, int size);
#endif
extern int sync_buffers(dev_t dev, int wait);

extern int init_bufs(struct super_block *p_volume);
extern void invalidate_buffers(dev_t dev);
extern void buffer_init(void);
extern int shrink_buffers(unsigned int priority);

extern int VFS_fsync(struct file *file);
extern void set_blocksize(dev_t dev, int size);

extern int sys_bdflush(int func, long data);
extern int sys_sync(void);

//
// From vfs/dcache.c
//
int dcache_lookup(struct inode * dir, const char * name, int len, unsigned long * ino);
void dcache_add(struct inode * dir, const char * name, int len, unsigned long ino);
unsigned long name_cache_init(unsigned long mem_start, unsigned long mem_end);

//
// From vfs/ll_rwblk.c
//
void ll_rw_block(int rw, int nr, struct buffer_head *bh[]);

//
// From vfs\rw.c
//
int VFS_readdir(struct file *file, struct dirent *dirent);
int VFS_read(struct file *file, char *buf, loff_t len, unsigned long *pLen);
int VFS_write(struct file *file, char *buf, loff_t len, unsigned long *pLen);

//
// From vfs/inode.c
//
extern struct inode * __iget(struct super_block * sb, ino_t nr, int crossmntp);
extern void iput(struct inode * inode);
extern void clear_inode(struct inode *inode);
extern struct inode *get_empty_inode(void);
extern void insert_inode_hash(struct inode *inode);
extern void sync_inodes(dev_t dev);
extern void invalidate_inodes(dev_t dev);
#if defined(MINIFSD)
extern void inode_init(void);
#else
extern unsigned long inode_init(unsigned long start, unsigned long end);
#endif
INLINE struct inode * iget(struct super_block * sb,ino_t nr)
{
#ifdef FS_TRACE
        kernel_printf("iget(%lu)", nr);
#endif
        return __iget(sb,nr,1);
}



//
// From vfs/f_table.c
//
#ifndef MINIFSD
unsigned long file_table_init(unsigned long start, unsigned long end);
void set_swapper_filp(struct file *f);
#endif
struct file *get_empty_filp(void);
int put_filp(struct file *p_file);

/*
 * vfs/namei.c
 */
asmlinkage int sys_link(struct inode *base, const char * oldname, const char * newname);
int do_unlink(struct inode *base, const char *name);
INLINE int sys_unlink(struct inode *base, const char *name) {
    return do_unlink(base, name);
}
int do_rmdir(struct inode *base, const char *name);
INLINE int sys_rmdir(struct inode *base, const char *name) {
    return do_rmdir(base, name);
}
int do_mkdir(struct inode *base, const char *name, int mode);
INLINE int sys_mkdir(struct inode *base, const char *name, int mode) {
    return do_mkdir(base, name, mode);
}
int do_rename(struct inode *srcbase, struct inode *dstbase, const char * oldname, const char * newname);
INLINE int sys_rename(struct inode *srcbase, struct inode *dstbase, const char * oldname, const char * newname) {
    return do_rename(srcbase, dstbase, oldname, newname);
}

int dir_namei(const char * pathname, int * namelen, const char ** name,
  	     struct inode * base, struct inode ** res_inode);

#endif /* __fs_proto_h */
