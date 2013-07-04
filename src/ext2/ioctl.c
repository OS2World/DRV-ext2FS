/*
 * linux/fs/ext2/ioctl.c
 *
 * Copyright (C) 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                           Laboratoire MASI - Institut Blaise Pascal
 *                           Universite Pierre et Marie Curie (Paris VI)
 */
#ifndef OS2
#include <asm\segment.h>

#include <linux\errno.h>
#include <linux\fs.h>
#include <linux\ext2_fs.h>
#include <linux\ioctl.h>
#include <linux\sched.h>
#include <linux\mm.h>
#else
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                    // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>


#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\devhlp32.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\errno.h>
#include <linux\ext2_fs.h>
#include <linux\ext2_proto.h>
#include <linux\fs_proto.h>
#include <linux\sched.h>
#include <linux\locks.h>
#include <linux\stat.h>
#include <linux\fcntl.h>
#include <os2\log.h>
#include <linux\ioctl.h>
#endif

#ifdef OS2
INLINE void put_fs_long(long s, long *d) {
    *d = s;
}
INLINE long get_fs_long(long *s) {
    return *s;
}
#endif

int ext2_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
        unsigned long arg)
{
    int err;
    unsigned long flags;

    ext2_debug ("cmd = %u, arg = %lu\n", cmd, arg);

    switch (cmd) {
    case EXT2_IOC_GETFLAGS:
#ifndef OS2
        /*
         * On OS/2 adressability has already been validated and buffer locked
         */
        if ((err = verify_area (VERIFY_WRITE, (long *) arg, sizeof(long))))
            return err;
#endif
        put_fs_long (inode->u.ext2_i.i_flags, (long *) arg);
        return 0;
    case EXT2_IOC_SETFLAGS:
        flags = get_fs_long ((long *) arg);
#ifndef OS2
        /*
         * On OS/2 we are always superuser
         */
        /*
         * Only the super-user can change the IMMUTABLE flag
         */
        if ((flags & EXT2_IMMUTABLE_FL) ^
            (inode->u.ext2_i.i_flags & EXT2_IMMUTABLE_FL)) {
            /* This test looks nicer. Thanks to Pauline Middelink */
            if (!fsuser())
                return -EPERM;
        } else
            if ((current->fsuid != inode->i_uid) && !fsuser())
                return -EPERM;
#endif
        if (IS_RDONLY(inode))
            return -EROFS;
        inode->u.ext2_i.i_flags = flags;
        if (flags & EXT2_APPEND_FL)
            inode->i_flags |= S_APPEND;
        else
            inode->i_flags &= ~S_APPEND;
        if (flags & EXT2_IMMUTABLE_FL)
            inode->i_flags |= S_IMMUTABLE;
        else
            inode->i_flags &= ~S_IMMUTABLE;
        inode->i_ctime = CURRENT_TIME;
        inode->i_dirt = 1;
        return 0;
    case EXT2_IOC_GETVERSION:
#ifndef OS2
        /*
         * On OS/2 adressability has already been validated and buffer locked
         */
        if ((err = verify_area (VERIFY_WRITE, (long *) arg, sizeof(long))))
            return err;
#endif
        put_fs_long (inode->u.ext2_i.i_version, (long *) arg);
        return 0;
    case EXT2_IOC_SETVERSION:
#ifndef OS2
        /*
         * On OS/2 we are always superuser
         */
        if ((current->fsuid != inode->i_uid) && !fsuser())
            return -EPERM;
#endif
        if (IS_RDONLY(inode))
            return -EROFS;
        inode->u.ext2_i.i_version = get_fs_long ((long *) arg);
        inode->i_ctime = CURRENT_TIME;
        inode->i_dirt = 1;
        return 0;
    default:
        return -EINVAL;
    }
}
