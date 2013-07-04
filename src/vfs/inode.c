//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/inode.c,v 1.1 2001/05/09 17:48:06 umoeller Exp $
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
 *  linux/fs/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#ifndef OS2
#include <linux\stat.h>
#include <linux\sched.h>
#include <linux\kernel.h>
#include <linux\mm.h>
#include <linux\string.h>

#include <asm\system.h>
#else
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>

#include <os2\types.h>
#include <os2\DevHlp32.h>
#include <os2\StackToFlat.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\sched.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>
#include <os2\errors.h>
#include <os2\minifsd.h>

extern unsigned long event;
//#define const
#include <builtin.h>
#endif

long nr_iget = 0;
long nr_iput = 0;

#ifdef OS2
#define Panic(x) ext2_os2_panic(1, (x))
#endif

static struct inode_hash_entry {
        struct inode * inode;
        int updating;
} hash_table[NR_IHASH];

#ifndef OS2
static struct inode * first_inode;
static struct wait_queue * inode_wait = NULL;
static int nr_inodes = 0, nr_free_inodes = 0;
#else
static struct inode * first_inode;
static unsigned long inode_wait = 0;
long nr_inodes = 0, nr_free_inodes = 0;
#endif
#ifndef OS2
INLINE int const hashfn(dev_t dev, unsigned int i)
#else
INLINE int hashfn(dev_t dev, unsigned int i)
#endif
{
        return (dev ^ i) % NR_IHASH;
}


INLINE struct inode_hash_entry * const hash(dev_t dev, int i)
{
        return hash_table + hashfn(dev, i);
}

static void insert_inode_free(struct inode *inode)
{
        _disable();
        inode->i_next = first_inode;
        inode->i_prev = first_inode->i_prev;
        inode->i_next->i_prev = inode;
        inode->i_prev->i_next = inode;
        first_inode = inode;
        _enable();
}

static void remove_inode_free(struct inode *inode)
{
        _disable();
        if (first_inode == inode)
                first_inode = first_inode->i_next;
        if (inode->i_next)
                inode->i_next->i_prev = inode->i_prev;
        if (inode->i_prev)
                inode->i_prev->i_next = inode->i_next;
        inode->i_next = inode->i_prev = NULL;
        _enable();
}

void insert_inode_hash(struct inode *inode)
{
        struct inode_hash_entry *h;

        _disable();
        h = hash(inode->i_dev, inode->i_ino);

        inode->i_hash_next = h->inode;
        inode->i_hash_prev = NULL;
        if (inode->i_hash_next)
                inode->i_hash_next->i_hash_prev = inode;
        h->inode = inode;
        _enable();
}

static void remove_inode_hash(struct inode *inode)
{
        struct inode_hash_entry *h;

        _disable();
        h = hash(inode->i_dev, inode->i_ino);

        if (h->inode == inode)
                h->inode = inode->i_hash_next;
        if (inode->i_hash_next)
                inode->i_hash_next->i_hash_prev = inode->i_hash_prev;
        if (inode->i_hash_prev)
                inode->i_hash_prev->i_hash_next = inode->i_hash_next;
        inode->i_hash_prev = inode->i_hash_next = NULL;
        _enable();
}

static void put_last_free(struct inode *inode)
{
        remove_inode_free(inode);
        _disable();
        inode->i_prev = first_inode->i_prev;
        inode->i_prev->i_next = inode;
        inode->i_next = first_inode;
        inode->i_next->i_prev = inode;
        _enable();
}

#ifndef OS2
void grow_inodes(void)
{
        struct inode * inode;
        int i;

        if (!(inode = (struct inode*) get_free_page(GFP_KERNEL)))
                return;

        i=PAGE_SIZE / sizeof(struct inode);
        nr_inodes += i;
        nr_free_inodes += i;

        if (!first_inode)
                inode->i_next = inode->i_prev = first_inode = inode++, i--;

        for ( ; i ; i-- )
                insert_inode_free(inode++);

}
#else
void grow_inodes(void)
{
        struct inode * inode;
        long i;
        int rc;

        /*
         * Swappable memory should be OK here.
         */
        rc = DevHlp32_VMAlloc(4096, VMDHA_NOPHYSADDR, VMDHA_SWAP, __StackToFlat(&inode));
        if (rc != NO_ERROR)
            return;
        memset(inode, 0, 4096);

        i=4096 / sizeof(struct inode);
        kernel_printf("********* grow_inode() from %lu to %lu", nr_inodes, nr_inodes + i);
        nr_inodes += i;
        nr_free_inodes += i;
#if 0
        if (!first_inode)
                inode->i_next = inode->i_prev = first_inode = inode++, i--;
#else
        if (!first_inode) {
                first_inode   = inode;
                inode->i_next = inode;
                inode->i_prev = inode;
                inode++;
                i--;
        }
        if ((!first_inode)         ||
            (!first_inode->i_prev) ||
            (!first_inode->i_next))
                Panic("grow_inodes - 1");
#endif

        for ( ; i ; i-- ) {
                insert_inode_free(inode++);
                if ((!first_inode) || (!first_inode->i_prev) || (!first_inode->i_next)) Panic("grow_inodes - 2");
        }
}
#endif
unsigned long inode_init(unsigned long start, unsigned long end)
{
        memset(hash_table, 0, sizeof(hash_table));
        first_inode = NULL;
        return start;
}

void __wait_on_inode(struct inode *);

INLINE void wait_on_inode(struct inode * inode)
{
        if (inode->i_lock)
                __wait_on_inode(inode);
}

INLINE void lock_inode(struct inode * inode)
{
        wait_on_inode(inode);
        inode->i_lock = 1;
}

INLINE void unlock_inode(struct inode * inode)
{
        inode->i_lock = 0;

        wake_up(&inode->i_wait);
}

/*
 * Note that we don't want to disturb any wait-queues when we discard
 * an inode.
 *
 * Argghh. Got bitten by a gcc problem with inlining: no way to tell
 * the compiler that the inline asm function 'memset' changes 'inode'.
 * I've been searching for the bug for days, and was getting desperate.
 * Finally looked at the assembler output... Grrr.
 *
 * The solution is the weird use of 'volatile'. Ho humm. Have to report
 * it to the gcc lists, and hope we can do this more cleanly some day..
 */
void clear_inode(struct inode * inode)
{
#ifndef OS2
        struct wait_queue * wait;
#endif

        wait_on_inode(inode);
        remove_inode_hash(inode);
        remove_inode_free(inode);
#ifndef OS2
        wait = ((volatile struct inode *) inode)->i_wait;
#endif
        if (inode->i_count)
                nr_free_inodes++;
        memset(inode,0,sizeof(*inode));
#ifndef OS2
        ((volatile struct inode *) inode)->i_wait = wait;
#endif
        insert_inode_free(inode);
}

#ifndef OS2
int fs_may_mount(dev_t dev)
{
        struct inode * inode, * next;
        int i;

        next = first_inode;
        for (i = nr_inodes ; i > 0 ; i--) {
                inode = next;
                next = inode->i_next;        /* clear_inode() changes the queues.. */
                if (inode->i_dev != dev)
                        continue;
                if (inode->i_count || inode->i_dirt || inode->i_lock)
                        return 0;
                clear_inode(inode);
        }
        return 1;
}
#endif

int fs_may_umount(dev_t dev, struct inode * mount_root)
{
        struct inode * inode;
        int i;

        inode = first_inode;
        for (i=0 ; i < nr_inodes ; i++, inode = inode->i_next) {
                if (inode->i_dev != dev || !inode->i_count)
                        continue;
                if (inode == mount_root && inode->i_count == 1)
                        continue;
                return 0;
        }
        return 1;
}

#ifndef OS2
int fs_may_remount_ro(dev_t dev)
{
        struct file * file;
        int i;

        /* Check that no files are currently opened for writing. */
        for (file = first_file, i=0; i<nr_files; i++, file=file->f_next) {
                if (!file->f_count || !file->f_inode ||
                    file->f_inode->i_dev != dev)
                        continue;
                if (S_ISREG(file->f_inode->i_mode) && (file->f_mode & 2))
                        return 0;
        }
        return 1;
}
#endif
static void write_inode(struct inode * inode)
{
#ifdef OS2
        if (inode->i_ino == INODE_DASD)
                return;
#endif
        if (!inode->i_dirt)
                return;
        wait_on_inode(inode);
        if (!inode->i_dirt)
                return;
        if (!inode->i_sb || !inode->i_sb->s_op || !inode->i_sb->s_op->write_inode) {
                inode->i_dirt = 0;
                return;
        }
        inode->i_lock = 1;
        inode->i_sb->s_op->write_inode(inode);
        unlock_inode(inode);
}

static void read_inode(struct inode * inode)
{
#ifdef OS2
        if (inode->i_ino == INODE_DASD) {
                inode->i_mode    = S_IFREG | S_IRUSR | S_IWUSR;
                inode->i_size    = inode->i_sb->sector_size * inode->i_sb->nb_sectors;
                inode->i_blksize = inode->i_sb->s_blocksize;
                inode->i_blocks  = inode->i_size / inode->i_sb->s_blocksize;
                inode->i_op      = &ext2_file_inode_operations;
                return;
        }
#endif
        lock_inode(inode);
        if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->read_inode)
                inode->i_sb->s_op->read_inode(inode);
        unlock_inode(inode);
}

#ifndef OS2
/* POSIX UID/GID verification for setting inode attributes */
int inode_change_ok(struct inode *inode, struct iattr *attr)
{
        /* Make sure a caller can chown */
        if ((attr->ia_valid & ATTR_UID) &&
            (current->fsuid != inode->i_uid ||
             attr->ia_uid != inode->i_uid) && !fsuser())
                return -EPERM;

        /* Make sure caller can chgrp */
        if ((attr->ia_valid & ATTR_GID) &&
            (!in_group_p(attr->ia_gid) && attr->ia_gid != inode->i_gid) &&
            !fsuser())
                return -EPERM;

        /* Make sure a caller can chmod */
        if (attr->ia_valid & ATTR_MODE) {
                if ((current->fsuid != inode->i_uid) && !fsuser())
                        return -EPERM;
                /* Also check the setgid bit! */
                if (!fsuser() && !in_group_p((attr->ia_valid & ATTR_GID) ? attr->ia_gid :
                                             inode->i_gid))
                        attr->ia_mode &= ~S_ISGID;
        }

        /* Check for setting the inode time */
        if ((attr->ia_valid & ATTR_ATIME_SET) &&
            ((current->fsuid != inode->i_uid) && !fsuser()))
                return -EPERM;
        if ((attr->ia_valid & ATTR_MTIME_SET) &&
            ((current->fsuid != inode->i_uid) && !fsuser()))
                return -EPERM;


        return 0;
}

/*
 * Set the appropriate attributes from an attribute structure into
 * the inode structure.
 */
void inode_setattr(struct inode *inode, struct iattr *attr)
{
        if (attr->ia_valid & ATTR_UID)
                inode->i_uid = attr->ia_uid;
        if (attr->ia_valid & ATTR_GID)
                inode->i_gid = attr->ia_gid;
        if (attr->ia_valid & ATTR_SIZE)
                inode->i_size = attr->ia_size;
        if (attr->ia_valid & ATTR_ATIME)
                inode->i_atime = attr->ia_atime;
        if (attr->ia_valid & ATTR_MTIME)
                inode->i_mtime = attr->ia_mtime;
        if (attr->ia_valid & ATTR_CTIME)
                inode->i_ctime = attr->ia_ctime;
        if (attr->ia_valid & ATTR_MODE) {
                inode->i_mode = attr->ia_mode;
                if (!fsuser() && !in_group_p(inode->i_gid))
                        inode->i_mode &= ~S_ISGID;
        }
        inode->i_dirt = 1;
}

/*
 * notify_change is called for inode-changing operations such as
 * chown, chmod, utime, and truncate.  It is guaranteed (unlike
 * write_inode) to be called from the context of the user requesting
 * the change.  It is not called for ordinary access-time updates.
 * NFS uses this to get the authentication correct.  -- jrs
 */

int notify_change(struct inode * inode, struct iattr *attr)
{
        int retval;

        if (inode->i_sb && inode->i_sb->s_op  &&
            inode->i_sb->s_op->notify_change)
                return inode->i_sb->s_op->notify_change(inode, attr);

        if ((retval = inode_change_ok(inode, attr)) != 0)
                return retval;

        inode_setattr(inode, attr);
        return 0;
}

/*
 * bmap is needed for demand-loading and paging: if this function
 * doesn't exist for a filesystem, then those things are impossible:
 * executables cannot be run from the filesystem etc...
 *
 * This isn't as bad as it sounds: the read-routines might still work,
 * so the filesystem would be otherwise ok (for example, you might have
 * a DOS filesystem, which doesn't lend itself to bmap very well, but
 * you could still transfer files to/from the filesystem)
 */
int bmap(struct inode * inode, int block)
{
        if (inode->i_op && inode->i_op->bmap)
                return inode->i_op->bmap(inode,block);
        return 0;
}
#endif

void invalidate_inodes(dev_t dev)
{
        struct inode * inode, * next;
        int i;

        next = first_inode;
        for(i = nr_inodes ; i > 0 ; i--) {
                inode = next;
                next = inode->i_next;                /* clear_inode() changes the queues.. */
                if (inode->i_dev != dev)
                        continue;
                if (inode->i_count || inode->i_dirt || inode->i_lock) {
#ifndef OS2
                        printk("VFS: inode busy on removed device %d/%d\n", MAJOR(dev), MINOR(dev));
#else
                        printk("VFS: inode %ld busy on removed device 0x%0X", inode->i_ino, dev);
#endif
                        continue;
                }
                clear_inode(inode);
        }
}

void hard_invalidate_inodes(dev_t dev)
{
        struct inode * inode, * next;
        int i;

        next = first_inode;
        for(i = nr_inodes ; i > 0 ; i--) {
                inode = next;
                next = inode->i_next;                /* clear_inode() changes the queues.. */
                if (inode->i_dev != dev)
                        continue;
                if (inode->i_count || inode->i_dirt || inode->i_lock) {
                        printk("VFS: inode %d busy on removed device 0x%04X", inode->i_ino, dev);
                }
                clear_inode(inode);
        }
}

void sync_inodes(dev_t dev)
{
        int i;

        struct inode * inode;

        inode = first_inode;
        for(i = 0; i < nr_inodes*2; i++, inode = inode->i_next) {
                if (dev && inode->i_dev != dev)
                        continue;
                wait_on_inode(inode);
                if (inode->i_dirt)
                        write_inode(inode);
        }
}

void iput(struct inode * inode)
{
#ifdef OS2
        nr_iput ++;
#endif
        if (!inode)
                return;
#ifdef FS_TRACE
        kernel_printf("iput(%lu) - prev. i_count = %d", inode->i_ino, inode->i_count);
#endif
        wait_on_inode(inode);
        if (!inode->i_count) {
   #ifndef OS2
                printk("VFS: iput: trying to free free inode\n");
                printk("VFS: device %d/%d, inode %lu, mode=0%07o\n",
                        MAJOR(inode->i_rdev), MINOR(inode->i_rdev),
                                        inode->i_ino, inode->i_mode);
#else
                printk("VFS: iput: trying to free free inode %lu", inode->i_ino);
#endif
                return;
        }
#ifndef OS2
        if (inode->i_pipe)
                wake_up_interruptible(&PIPE_WAIT(*inode));
#endif
repeat:
        if (inode->i_count>1) {
                inode->i_count--;
                return;
        }
        wake_up(&inode_wait);

#ifndef OS2
        if (inode->i_pipe) {
                unsigned long page = (unsigned long) PIPE_BASE(*inode);
                PIPE_BASE(*inode) = NULL;
                free_page(page);
        }
#endif
#ifdef OS2
        if (inode->i_ino != INODE_DASD) {
#endif
        if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->put_inode) {
                inode->i_sb->s_op->put_inode(inode);
                if (!inode->i_nlink)
                        return;
        }
        if (inode->i_dirt) {
                write_inode(inode);        /* we can sleep - so do again */
                wait_on_inode(inode);
                goto repeat;
        }
#ifdef OS2
        } else {
            clear_inode(inode);
            return;
        }
#endif
        inode->i_count--;
#ifndef OS2
        if (inode->i_mmap) {
                printk("iput: inode %lu on device %d/%d still has mappings.\n",
                        inode->i_ino, MAJOR(inode->i_dev), MINOR(inode->i_dev));
                inode->i_mmap = NULL;
        }
#endif
        nr_free_inodes++;
        return;
}

struct inode * get_empty_inode(void)
{
        struct inode * inode, * best;
        int i;

        if (nr_inodes < NR_INODE && nr_free_inodes < (nr_inodes >> 2))
                grow_inodes();
repeat:
        inode = first_inode;
        best = NULL;
        for (i = 0; i<nr_inodes; inode = inode->i_next, i++) {
                if (!inode->i_count) {
                        if (!best)
                                best = inode;
                        if (!inode->i_dirt && !inode->i_lock) {
                                best = inode;
                                break;
                        }
                }
        }
        if (!best || best->i_dirt || best->i_lock)
                if (nr_inodes < NR_INODE) {
                        grow_inodes();
                        goto repeat;
                }
        inode = best;

        if (!inode) {
                printk("VFS: No free inodes - contact Linus\n");
                sleep_on(&inode_wait);
                goto repeat;
        }

        if (inode->i_lock) {
                wait_on_inode(inode);
                goto repeat;
        }
        if (inode->i_dirt) {
                write_inode(inode);
                goto repeat;
        }
        if (inode->i_count)
                goto repeat;
        clear_inode(inode);
        inode->i_count = 1;
        inode->i_nlink = 1;
        inode->i_version = ++event;
        inode->i_sem.count = 1;
        nr_free_inodes--;
        if (nr_free_inodes < 0) {
                printk ("VFS: get_empty_inode: bad free inode count.\n");
                nr_free_inodes = 0;
        }
        return inode;
}

#ifndef OS2
struct inode * get_pipe_inode(void)
{
        struct inode * inode;
        extern struct inode_operations pipe_inode_operations;

        if (!(inode = get_empty_inode()))
                return NULL;
        if (!(PIPE_BASE(*inode) = (char*) __get_free_page(GFP_USER))) {
                iput(inode);
                return NULL;
        }
        inode->i_op = &pipe_inode_operations;
        inode->i_count = 2;        /* sum of readers/writers */
        PIPE_WAIT(*inode) = NULL;
        PIPE_START(*inode) = PIPE_LEN(*inode) = 0;
        PIPE_RD_OPENERS(*inode) = PIPE_WR_OPENERS(*inode) = 0;
        PIPE_READERS(*inode) = PIPE_WRITERS(*inode) = 1;
        PIPE_LOCK(*inode) = 0;
        inode->i_pipe = 1;
        inode->i_mode |= S_IFIFO | S_IRUSR | S_IWUSR;
        inode->i_uid = current->fsuid;
        inode->i_gid = current->fsgid;
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
        inode->i_blksize = PAGE_SIZE;
        return inode;
}
#endif

#ifndef OS2
struct inode * __iget(struct super_block * sb, int nr, int crossmntp)
{
        static struct wait_queue * update_wait = NULL;
#else
struct inode * __iget(struct super_block * sb, ino_t nr, int crossmntp)
{
        static unsigned long update_wait = 0;
#endif
        struct inode_hash_entry * h;
        struct inode * inode;
        struct inode * empty = NULL;
#ifdef FS_TRACE
        kernel_printf("iget(%lu)", nr);
#endif
        nr_iget ++;
        if (!sb)
#ifndef OS2
                panic("VFS: iget with sb==NULL");
#else
                Panic("VFS: iget with sb==NULL");
#endif
        h = hash(sb->s_dev, nr);
repeat:
        for (inode = h->inode; inode ; inode = inode->i_hash_next)
                if (inode->i_dev == sb->s_dev && inode->i_ino == nr)
                        goto found_it;
        if (!empty) {
                h->updating++;
                empty = get_empty_inode();
                if (!--h->updating)
                        wake_up(&update_wait);
                if (empty)
                        goto repeat;
                return (NULL);
        }
        inode = empty;
        inode->i_sb = sb;
        inode->i_dev = sb->s_dev;
        inode->i_ino = nr;
        inode->i_flags = sb->s_flags;
        put_last_free(inode);
        insert_inode_hash(inode);
        read_inode(inode);
        goto return_it;

found_it:
        if (!inode->i_count)
                nr_free_inodes--;
        inode->i_count++;
        wait_on_inode(inode);
        if (inode->i_dev != sb->s_dev || inode->i_ino != nr) {
                printk("Whee.. inode changed from under us. Tell Linus\n");
                iput(inode);
                goto repeat;
        }
#ifndef OS2
        if (crossmntp && inode->i_mount) {
                struct inode * tmp = inode->i_mount;
                tmp->i_count++;
                iput(inode);
                inode = tmp;
                wait_on_inode(inode);
        }
#endif
        if (empty)
                iput(empty);

return_it:
        while (h->updating)
                sleep_on(&update_wait);
        return inode;
}

/*
 * The "new" scheduling primitives (new as of 0.97 or so) allow this to
 * be done without disabling interrupts (other than in the actual queue
 * updating things: only a couple of 386 instructions). This should be
 * much better for interrupt latency.
 */
#ifndef OS2
static void __wait_on_inode(struct inode * inode)
{
        struct wait_queue wait = { current, NULL };

        add_wait_queue(&inode->i_wait, &wait);
repeat:
        current->state = TASK_UNINTERRUPTIBLE;
        if (inode->i_lock) {
                schedule();
                goto repeat;
        }
        remove_wait_queue(&inode->i_wait, &wait);
        current->state = TASK_RUNNING;
}
#else

void __wait_on_inode(struct inode * inode)
{
    _disable();
    while (inode->i_lock) {
        DevHlp32_ProcBlock((unsigned long)(&(inode->i_wait)), -1, 1);
        _disable();
    }
    _enable();
}
#endif

extern void *_Pascal VDHQueryLin(PTR16);

void inherit_minifsd_inodes(struct minifsd_to_fsd_data  *mfs_data) {
    int  i;
    struct inode  *inode;
    struct inode  *next;
    PTR16         *p_first_inode;
    long          *nr_inodes_minifsd;

    nr_inodes_minifsd = (long *)VDHQueryLin(mfs_data->nr_inodes);
    printk("nr_inodes %d", *nr_inodes_minifsd);

    p_first_inode = (PTR16 *)VDHQueryLin(mfs_data->first_inode);
    if (*nr_inodes_minifsd)
        next = (struct inode *)VDHQueryLin(*p_first_inode);
    for (i = 0; i < *nr_inodes_minifsd; i++) {
        inode = next;
        if (inode->i_next)
            next = (struct inode *)VDHQueryLin(*(PTR16 *)(&(inode->i_next)));
        else
            next = 0;

        /*
         * if the inode is in use, we inherit it.
         */
            nr_inodes ++;
            inode->i_next      = 0;
            inode->i_prev      = 0;
            inode->i_hash_next = 0;
            inode->i_hash_prev = 0;
            if (!first_inode) {
                first_inode    = inode;
                inode->i_next  = inode;
                inode->i_prev  = inode;
            } else {
                put_last_free(inode);
            }
            if (inode->i_count) {
                printk("found inode to inherit");
                insert_inode_hash(inode);
                if (S_ISREG(inode->i_mode))
                    inode->i_op = &ext2_file_inode_operations;
                else if (S_ISDIR(inode->i_mode))
                    inode->i_op = &ext2_dir_inode_operations;
                else
                    inode->i_op = 0;
                if (inode->i_sb)
                    inode->i_sb = (struct super_block *)VDHQueryLin(*(PTR16 *)(&(inode->i_sb)));
            } else {
                nr_free_inodes ++;
            }
        }

}
