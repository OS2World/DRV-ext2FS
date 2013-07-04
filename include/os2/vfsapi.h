//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/vfsapi.h,v 1.1 2001/05/09 17:43:43 umoeller Exp $
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


#ifndef __VFSAPI_H
#define __VFSAPI_H

#include <linux\stat.h>

/*
 * DosFsCtl function range 0x8000 - 0xBFFF is reserved for local FSD use
 */
#define EXT2_OS2_STAT           0xA000                // vfs_stat()
#define EXT2_OS2_FSTAT          0xA001                // vfs_fstat()
#define EXT2_OS2_SYNC           0xA002                // vfs_sync()
#define EXT2_OS2_GETDATA        0xA003                // (private)
#define EXT2_OS2_BDFLUSH        0xA004                // vfs_bdflush()
#define EXT2_OS2_SHRINK_CACHE   0xA005                // (private)
#define EXT2_OS2_NORMALIZE_PATH 0xA006                // (private)
#define EXT2_OS2_LINK           0xA007                // vfs_link()

#define EXT2_OS2_IOCTL_EXT2_IOC_GETFLAGS   0xA008       // EXT2_IOC_GETFLAGS
#define EXT2_OS2_IOCTL_EXT2_IOC_SETFLAGS   0xA009   // EXT2_IOC_SETFLAGS
#define EXT2_OS2_IOCTL_EXT2_IOC_GETVERSION 0xA00A   // EXT2_IOC_GETVERSION
#define EXT2_OS2_IOCTL_EXT2_IOC_SETVERSION 0xA00B   // EXT2_IOC_SETVERSION

#define EXT2_OS2_SHOW_BUFFERS 0xB000
#define EXT2_OS2_DUMP_DIRTY_BUFFERS 0xB001

/*
 * DosFsCtl FSCTL_ERROR_INFO messages
 */
#define MAX_MSG        256
#define magic_msg   "EXT2-OS2 (Linux ext2fs IFS for OS/2) - The error code is NO ERROR"
#define default_msg "EXT2-OS2 (Linux ext2fs IFS for OS/2) - The error code is UNKNOWN"
struct fsctl_msg {
    short  length;
    char   msg[MAX_MSG];
};


/*
 * EXT2_OS2_GETDATA data structures
 */

struct buffer_data {
    long buffer_mem;
    long cache_size;
    long nr_buffer_heads;

    long nr_buffers_type[6];
    long nr_free[4];

};
#ifdef __KERNEL__
extern long buffermem;                // in vfs/buffer.c
extern long cache_size;               // in vfs/buffer.c
extern long nr_buffer_heads;          // in vfs/buffer.c
extern long nr_free[4];               // in vfs/buffer.c
extern long nr_buffers_type[6]; // in vfs/buffer.c
#endif

struct inode_data {
    long nr_inodes;
    long nr_free_inodes;
    long nr_iget;
    long nr_iput;
};


#ifdef __KERNEL__
extern long nr_inodes;         // in vfs/inode.c
extern long nr_free_inodes;    // in vfs/inode.c
extern long nr_iget;           // in vfs/inode.c
extern long nr_iput;           // in vfs/inode.c
#endif

struct file_data {
    long nhfiles;
    long nfreehfiles;
    long nusedhfiles;
};

#ifdef __KERNEL__
extern long nr_files;         // vfs/f_table.c
extern long nr_free_files;    // vfs/f_table.c
extern long nr_used_files;    // vfs/f_table.c
#endif

struct swapper_data {
    long nr_total_pgin;       // fs_swap.c
    long nr_total_pgout;      // fs_swap.c
    long nr_pgin;             // fs_swap.c
    long nr_pgout;            // fs_swap.c
};

#ifdef __KERNEL__
extern long nr_total_pgin;
extern long nr_total_pgout;
extern long nr_pgin;
extern long nr_pgout;
#endif

struct ext2_os2_data {
    struct buffer_data  b;
    struct inode_data   i;
    struct file_data    f;
    struct swapper_data s;
};

#ifndef __KERNEL__

    #ifdef __IBMC__
    #define VFSENTRY _System
    #else
    #define VFSENTRY
    #endif

    struct vfs_stat {
            unsigned short st_dev;
            unsigned short __pad1;
            unsigned long st_ino;
            unsigned short st_mode;
            unsigned short st_nlink;
            unsigned short st_uid;
            unsigned short st_gid;
            unsigned short st_rdev;
            unsigned short __pad2;
            unsigned long  st_size;
            unsigned long  st_blksize;
            unsigned long  st_blocks;
            unsigned long  st_atime;
            unsigned long  __unused1;
            unsigned long  st_mtime;
            unsigned long  __unused2;
            unsigned long  st_ctime;
            unsigned long  __unused3;
            unsigned long  __unused4;
            unsigned long  __unused5;
    };

    int VFSENTRY vfs_fstat(int fd, struct vfs_stat *s);
    int VFSENTRY vfs_stat(const char *pathname, struct vfs_stat *s);
    int VFSENTRY vfs_sync(void);
    int VFSENTRY vfs_link(const char *, const char *);

#endif

#endif
