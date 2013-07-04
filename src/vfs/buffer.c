//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/buffer.c,v 1.1 2001/05/09 17:48:06 umoeller Exp $
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
 *  linux/fs/buffer.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting an interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it.
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif


#ifndef OS2
#include <linux\sched.h>
#include <linux\kernel.h>
#include <linux\major.h>
#include <linux\string.h>
#include <linux\locks.h>
#include <linux\errno.h>
#include <linux\malloc.h>

#include <asm\system.h>
#include <asm\segment.h>
#include <asm\io.h>
#else
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>
#include <builtin.h>

#include <os2\types.h>
#include <os2\fsd32.h>
#include <os2\devhlp32.h>
#include <os2\StackToFlat.h>
#include <os2\errors.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <os2\os2proto.h>

#include <linux\errno.h>
#include <os2\log.h>
#include <linux\sched.h>
#include <linux\locks.h>
#include <os2\volume.h>

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))


#define GFP_BUFFER   0            // Meaningless on OS/2
#define GFP_ATOMIC   0            // Meaningless on OS/2
#define GFP_KERNEL   0            // Meaningless on OS/2
#define GFP_NOBUFFER 0            // Meaningless on OS/2

#define UINT_MAX 0xFFFFFFFFUL

#define panic(msg) ext2_os2_panic(0, msg)
#define printk kernel_printf

#define MAJOR(dev) 0                  // shortcut just for messages
#define MINOR(dev) dev                // shortcut just for messages

#define jiffies (CURRENT_TIME * 30)   // jiffies unit seems to be the timer tick


#define FSHIFT   11             /* nr of bits of precision */
#define FIXED_1 (1<<FSHIFT)     /* 1.0 as fixed-point */

#define CALC_LOAD(load,exp,n) \
        load *= exp; \
        load += n*(FIXED_1-exp); \
        load >>= FSHIFT;


#endif /* OS2 */

#define NR_SIZES 4
static char buffersize_index[9] = {-1,  0,  1, -1,  2, -1, -1, -1, 3};
static short int bufferindex_size[NR_SIZES] = {512, 1024, 2048, 4096};

#define BUFSIZE_INDEX(X) ((int) buffersize_index[(X)>>9])
#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)


static int shrink_specific_buffers(unsigned int priority, int size);
static int grow_buffers(int pri, int size);
static int maybe_shrink_lav_buffers(int);

static long nr_hash = 0;  /* Size of hash table */
static struct buffer_head ** hash_table = 0;
#ifndef OS2
struct buffer_head ** buffer_pages;
#endif

static struct buffer_head * lru_list[NR_LIST] = {NULL, };
static struct buffer_head * free_list[NR_SIZES] = {NULL, };
static struct buffer_head * unused_list = NULL;
#ifndef OS2
static struct wait_queue * buffer_wait = NULL;
#else
static unsigned long buffer_wait = 0;
#endif
long nr_buffers = 0;
long nr_buffers_type[NR_LIST] = {0,};
long nr_buffers_size[NR_SIZES] = {0,};
long nr_buffers_st[NR_SIZES][NR_LIST] = {{0,},};
long buffer_usage[NR_SIZES] = {0,};  /* Usage counts used to determine load average */
long buffers_lav[NR_SIZES] = {0,};  /* Load average of buffer usage */
long nr_free[NR_SIZES] = {0,};
long buffermem = 0;
long nr_buffer_heads = 0;
extern int *blksize_size[];





/*
 * Maximum cache size - default is 4 * 61440 Kb
 */
long cache_size = 245760L;

extern long swap_in_progress;

INLINE unsigned long __get_free_page(int pri) {
    void  *page;
    int    rc;

    if (swap_in_progress)
        return 0;

    rc = DevHlp32_VMAlloc(4096, VMDHA_NOPHYSADDR, VMDHA_FIXED, __StackToFlat(&page));
    if (rc != NO_ERROR)
        return 0;
    return (unsigned long)page;
}

INLINE unsigned long get_free_page(int pri) {
    return __get_free_page(pri);
}
INLINE int free_page(unsigned long page) {
    return DevHlp32_VMFree((void *)page);
}


/* Here is the parameter block for the bdflush process. */
static void wakeup_bdflush(int);

#define N_PARAM 9
#define LAV

static union bdflush_param{
        struct {
                long nfract;  /* Percentage of buffer cache dirty to
                                activate bdflush */
                long ndirty;  /* Maximum number of dirty blocks to write out per
                                wake-cycle */
                long nrefill; /* Number of clean buffers to try and obtain
                                each time we call refill */
                long nref_dirt; /* Dirty buffer threshold for activating bdflush
                                  when trying to refill buffers. */
                long clu_nfract;  /* Percentage of buffer cache to scan to
                                    search for free clusters */
                long age_buffer;  /* Time for normal buffer to age before
                                    we flush it */
                long age_super;  /* Time for superblock to age before we
                                   flush it */
                long lav_const;  /* Constant used for load average (time
                                   constant */
                long lav_ratio;  /* Used to determine how low a lav for a
                                   particular size can go before we start to
                                   trim back the buffers */
        } b_un;
        unsigned long data[N_PARAM];
} bdf_prm = {{25, 500, 64, 256, 15, 3000, 500, 1884, 2}};

/* The lav constant is set for 1 minute, as long as the update process runs
   every 5 seconds.  If you change the frequency of update, the time
   constant will also change. */


/* These are the min and max parameter values that we will allow to be assigned */
static long bdflush_min[N_PARAM] = {  0,  10,    5,   25,  0,   100,   100, 1, 1};
static long bflush_max[N_PARAM] = {100,5000, 2000, 2000,100, 60000, 60000, 2047, 5};

#ifndef OS2
/*
 * Rewrote the wait-routines to use the "new" wait-queue functionality,
 * and getting rid of the cli-sti pairs. The wait-queue routines still
 * need cli-sti, but now it's just a couple of 386 instructions or so.
 *
 * Note that the real wait_on_buffer() is an inline function that checks
 * if 'b_wait' is set before calling this, so that the queues aren't set
 * up unnecessarily.
 */
void __wait_on_buffer(struct buffer_head * bh)
{
        struct wait_queue wait = { current, NULL };

        bh->b_count++;
        add_wait_queue(&bh->b_wait, &wait);
repeat:
        current->state = TASK_UNINTERRUPTIBLE;
        if (bh->b_lock) {
                schedule();
                goto repeat;
        }
        remove_wait_queue(&bh->b_wait, &wait);
        bh->b_count--;
        current->state = TASK_RUNNING;
}
#endif

/* Call sync_buffers with wait!=0 to ensure that the call does not
   return until all buffer writes have completed.  Sync() may return
   before the writes have finished; fsync() may not. */


/* Godamity-damn.  Some buffers (bitmaps for filesystems)
   spontaneously dirty themselves without ever brelse being called.
   We will ultimately want to put these in a separate list, but for
   now we search all of the lists for dirty buffers */

#ifndef OS2
static int sync_buffers(dev_t dev, int wait)
{
        int i, retry, pass = 0, err = 0;
#else
int sync_buffers(dev_t dev, int wait) {
        int i, retry, pass = 0, err = 0;
#endif
        int nlist, ncount;
        struct buffer_head * bh, *next;

        /* One pass for no-wait, three for wait:
           0) write out all dirty, unlocked buffers;
           1) write out all dirty buffers, waiting if locked;
           2) wait for completion by waiting for all buffers to unlock. */
 repeat:
        retry = 0;
 repeat2:
        ncount = 0;
        /* We search all lists as a failsafe mechanism, not because we expect
           there to be dirty buffers on any of the other lists. */
        for(nlist = 0; nlist < NR_LIST; nlist++)
         {
         repeat1:
                 bh = lru_list[nlist];
                 if(!bh) continue;
                 for (i = nr_buffers_type[nlist]*2 ; i-- > 0 ; bh = next) {
                         if(bh->b_list != nlist) goto repeat1;
                         next = bh->b_next_free;
                         if(!lru_list[nlist]) break;
                         if (dev && bh->b_dev != dev)
                                  continue;
                         if (bh->b_lock)
                          {
                                  /* Buffer is locked; skip it unless wait is
                                     requested AND pass > 0. */
                                  if (!wait || !pass) {
                                          retry = 1;
                                          continue;
                                  }
                                  wait_on_buffer (bh);
                                  goto repeat2;
                          }
                         /* If an unlocked buffer is not uptodate, there has
                             been an IO error. Skip it. */
                         if (wait && bh->b_req && !bh->b_lock &&
                             !bh->b_dirt && !bh->b_uptodate) {
                                  err = 1;
                                  printk("Weird - unlocked, clean and not uptodate buffer on list %d %x %lu\n", nlist, bh->b_dev, bh->b_blocknr);
                                  continue;
                          }
                         /* Don't write clean buffers.  Don't write ANY buffers
                            on the third pass. */
                         if (!bh->b_dirt || pass>=2)
                                  continue;
                         /* don't bother about locked buffers */
                         if (bh->b_lock)
                                 continue;
                         bh->b_count++;
                         bh->b_flushtime = 0;
                         ll_rw_block(WRITE, 1, __StackToFlat(&bh));

                         if(nlist != BUF_DIRTY) {
                                 printk("[%d %x %ld] ", nlist, bh->b_dev, bh->b_blocknr);
                                 ncount++;
                         };
                         bh->b_count--;
                         retry = 1;
                 }
         }
        if (ncount) printk("sys_sync: %d dirty buffers not on dirty list\n", ncount);

        /* If we are waiting for the sync to succeed, and if any dirty
           blocks were written, then repeat; on the second pass, only
           wait for buffers being written (do not pass to write any
           more buffers on the second pass). */
        if (wait && retry && ++pass<=2)
                 goto repeat;
        return err;
}

void sync_dev(dev_t dev)
{
        sync_buffers(dev, 0);
#ifndef OS2
        sync_supers(dev);
#endif
        sync_inodes(dev);
        sync_buffers(dev, 0);
}

int fsync_dev(dev_t dev)
{
        sync_buffers(dev, 0);
#ifndef OS2
        sync_supers(dev);
#endif
        sync_inodes(dev);
        return sync_buffers(dev, 1);
}

asmlinkage int sys_sync(void)
{
        sync_dev(0);
        return 0;
}

int file_fsync (struct inode *inode, struct file *filp)
{
        return fsync_dev(inode->i_dev);
}

#ifndef OS2
asmlinkage int sys_fsync(unsigned int fd)
{
        struct file * file;
        struct inode * inode;

        if (fd>=NR_OPEN || !(file=current->files->fd[fd]) || !(inode=file->f_inode))
                return -EBADF;
        if (!file->f_op || !file->f_op->fsync)
                return -EINVAL;
        if (file->f_op->fsync(inode,file))
                return -EIO;
        return 0;
}
#else
/*
 * VFS_fsync()
 *
 * On OS/2 it is called by the DosResetBuffer system call. We directly get the struct file *file from
 * the FS_COMMIT parameters.
 *
 */
int VFS_fsync(struct file *file) {
    int rc;

    if (file && file->f_inode) {
        if (file->f_op && file->f_op->fsync) {
            rc = file->f_op->fsync(file->f_inode, file);
        } else {
            rc =  -EINVAL;
        }
    } else {
        rc = -EBADF;
    }
    return map_err(rc);
}
#endif

void invalidate_buffers(dev_t dev)
{
        int i;
        int nlist;
        struct buffer_head * bh;

        for(nlist = 0; nlist < NR_LIST; nlist++) {
                bh = lru_list[nlist];
                for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bh->b_next_free) {
                        if (bh->b_dev != dev)
                                continue;
                        wait_on_buffer(bh);
                        if (bh->b_dev != dev)
                                continue;
#ifndef OS2 // in OS/2 we must invalidate ALL buffers.
                        if (bh->b_count)
                                continue;
#endif
                        bh->b_flushtime = bh->b_uptodate =
                                bh->b_dirt = bh->b_req = 0;
                }
        }
}

#define _hashfn(dev,block) (((unsigned)(dev^block))%nr_hash)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

INLINE void remove_from_hash_queue(struct buffer_head * bh)
{
#ifdef XXXOS2
        _disable();
#endif
        if (bh->b_next)
                bh->b_next->b_prev = bh->b_prev;
        if (bh->b_prev)
                bh->b_prev->b_next = bh->b_next;
        if (hash(bh->b_dev,bh->b_blocknr) == bh)
                hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
        bh->b_next = bh->b_prev = NULL;
#ifdef XXXOS2
        _enable();
#endif
}

INLINE void remove_from_lru_list(struct buffer_head * bh)
{
#ifdef XXXOS2
        _disable();
#endif
        if (!(bh->b_prev_free) || !(bh->b_next_free))
                panic("VFS: LRU block list corrupted");
        if (bh->b_dev == 0xffff) panic("LRU list corrupted");
        bh->b_prev_free->b_next_free = bh->b_next_free;
        bh->b_next_free->b_prev_free = bh->b_prev_free;

        if (lru_list[bh->b_list] == bh)
                 lru_list[bh->b_list] = bh->b_next_free;
        if(lru_list[bh->b_list] == bh)
                 lru_list[bh->b_list] = NULL;
        bh->b_next_free = bh->b_prev_free = NULL;
#ifdef XXXOS2
        _enable();
#endif
}

INLINE void remove_from_free_list(struct buffer_head * bh)
{
        int isize = BUFSIZE_INDEX(bh->b_size);
#ifdef XXXOS2
        _disable();
#endif
        if (!(bh->b_prev_free) || !(bh->b_next_free))
                panic("VFS: Free block list corrupted");
        if(bh->b_dev != 0xffff) panic("Free list corrupted");
        if(!free_list[isize])
                 panic("Free list empty");
        nr_free[isize]--;
        if(bh->b_next_free == bh)
                 free_list[isize] = NULL;
        else {
                bh->b_prev_free->b_next_free = bh->b_next_free;
                bh->b_next_free->b_prev_free = bh->b_prev_free;
                if (free_list[isize] == bh)
                         free_list[isize] = bh->b_next_free;
        };
        bh->b_next_free = bh->b_prev_free = NULL;
#ifdef XXXOS2
        _enable();
#endif
}

INLINE void remove_from_queues(struct buffer_head * bh)
{
        if(bh->b_dev == 0xffff) {
                remove_from_free_list(bh); /* Free list entries should not be
                                              in the hash queue */
                return;
        };
        nr_buffers_type[bh->b_list]--;
        nr_buffers_st[BUFSIZE_INDEX(bh->b_size)][bh->b_list]--;
        remove_from_hash_queue(bh);
        remove_from_lru_list(bh);
}

INLINE void put_last_lru(struct buffer_head * bh)
{
        if (!bh)
                return;
        if (bh == lru_list[bh->b_list]) {
                lru_list[bh->b_list] = bh->b_next_free;
                return;
        }
        if(bh->b_dev == 0xffff) panic("Wrong block for lru list");
        remove_from_lru_list(bh);
/* add to back of free list */

#ifdef XXXOS2
        _disable();
#endif
        if(!lru_list[bh->b_list]) {
                lru_list[bh->b_list] = bh;
                lru_list[bh->b_list]->b_prev_free = bh;
        };

        bh->b_next_free = lru_list[bh->b_list];
        bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
        lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
        lru_list[bh->b_list]->b_prev_free = bh;
#ifdef XXXOS2
        _enable();
#endif
}

INLINE void put_last_free(struct buffer_head * bh)
{
        int isize;
        if (!bh)
                return;

        isize = BUFSIZE_INDEX(bh->b_size);
        bh->b_dev = 0xffff;  /* So it is obvious we are on the free list */
/* add to back of free list */
#ifdef XXXOS2
        _disable();
#endif

        if(!free_list[isize]) {
                free_list[isize] = bh;
                bh->b_prev_free = bh;
        };

        nr_free[isize]++;
        bh->b_next_free = free_list[isize];
        bh->b_prev_free = free_list[isize]->b_prev_free;
        free_list[isize]->b_prev_free->b_next_free = bh;
        free_list[isize]->b_prev_free = bh;
#ifdef XXXOS2
        _enable();
#endif

}

INLINE void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */

        if(bh->b_dev == 0xffff) {
                put_last_free(bh);
                return;
        };
        if(!lru_list[bh->b_list]) {
                lru_list[bh->b_list] = bh;
                bh->b_prev_free = bh;
        };
        if (bh->b_next_free) panic("VFS: buffer LRU pointers corrupted");
        bh->b_next_free = lru_list[bh->b_list];
        bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
        lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
        lru_list[bh->b_list]->b_prev_free = bh;
        nr_buffers_type[bh->b_list]++;
        nr_buffers_st[BUFSIZE_INDEX(bh->b_size)][bh->b_list]++;
/* put the buffer in new hash-queue if it has a device */
        bh->b_prev = NULL;
        bh->b_next = NULL;
        if (!bh->b_dev)
                return;
        bh->b_next = hash(bh->b_dev,bh->b_blocknr);
        hash(bh->b_dev,bh->b_blocknr) = bh;
        if (bh->b_next)
                bh->b_next->b_prev = bh;
}

static struct buffer_head * find_buffer(dev_t dev, int block, int size)
{
        struct buffer_head * tmp;

        for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
                if (tmp->b_dev==dev && tmp->b_blocknr==block)
                        if (tmp->b_size == size)
                                return tmp;
                        else {
                                printk("VFS: Wrong blocksize on device %d/%d\n",
                                                        MAJOR(dev), MINOR(dev));
                                return NULL;
                        }
        return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are reading them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head *get_hash_table(dev_t dev, int block, int size)
{
        struct buffer_head * bh;

        for (;;) {
                if (!(bh=find_buffer(dev,block,size)))
                        return NULL;
                bh->b_count++;
                wait_on_buffer(bh);
                if (bh->b_dev == dev && bh->b_blocknr == block && bh->b_size == size)
                        return bh;
                bh->b_count--;
        }
}



void set_blocksize(dev_t dev, int size)
{
        int i, nlist;
        struct buffer_head * bh, *bhnext;
#ifndef OS2
        if (!blksize_size[MAJOR(dev)])
                return;
#endif
        switch(size) {
                default: panic("Invalid blocksize passed to set_blocksize");
                case 512: case 1024: case 2048: case 4096:;
        }
#ifndef OS2
        if (blksize_size[MAJOR(dev)][MINOR(dev)] == 0 && size == BLOCK_SIZE) {
                blksize_size[MAJOR(dev)][MINOR(dev)] = size;
                return;
        }
        if (blksize_size[MAJOR(dev)][MINOR(dev)] == size)
                return;
#endif
        sync_buffers(dev, 2);
#ifndef OS2
        blksize_size[MAJOR(dev)][MINOR(dev)] = size;
#endif
  /* We need to be quite careful how we do this - we are moving entries
     around on the free list, and we can get in a loop if we are not careful.*/

        for(nlist = 0; nlist < NR_LIST; nlist++) {
                bh = lru_list[nlist];
                for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bhnext) {
                        if(!bh) break;
                        bhnext = bh->b_next_free;
                        if (bh->b_dev != dev)
                                 continue;
                        if (bh->b_size == size)
                                 continue;

                        wait_on_buffer(bh);
                        if (bh->b_dev == dev && bh->b_size != size) {
                                bh->b_uptodate = bh->b_dirt = bh->b_req =
                                         bh->b_flushtime = 0;
                        };
                        remove_from_hash_queue(bh);
                }
        }
}


#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)

void refill_freelist(int size)
{
        struct buffer_head * bh, * tmp;
        struct buffer_head * __candidate[NR_LIST];
        struct buffer_head **candidate = __StackToFlat(__candidate);
        int isize = BUFSIZE_INDEX(size);
        int __buffers[NR_LIST];
        int *buffers = __StackToFlat(__buffers);
        int i;
        unsigned int best_time, winner;
        int needed;

        /* First see if we even need this.  Sometimes it is advantageous
         to request some blocks in a filesystem that we know that we will
         be needing ahead of time. */

        if (nr_free[isize] > 100)
                return;

        /* If there are too many dirty buffers, we wake up the update process
           now so as to ensure that there are still clean buffers available
           for user processes to use (and dirty) */

        /* We are going to try and locate this much memory */
        needed =bdf_prm.b_un.nrefill * size;

#ifndef OS2
        while (nr_free_pages > min_free_pages*2 && needed > 0 &&
#else
        while ( buffermem < cache_size && needed > 0 &&
#endif
               grow_buffers(GFP_BUFFER, size)) {
                needed -= PAGE_SIZE;
        }
        if(needed <= 0) return;

        /* See if there are too many buffers of a different size.
           If so, victimize them */

        while(maybe_shrink_lav_buffers(size))
         {
                 if(!grow_buffers(GFP_BUFFER, size)) break;
                 needed -= PAGE_SIZE;
                 if(needed <= 0) return;
         };

        /* OK, we cannot grow the buffer cache, now try and get some
           from the lru list */

        /* First set the candidate pointers to usable buffers.  This
           should be quick nearly all of the time. */

repeat0:
        for(i=0; i<NR_LIST; i++){
                if(i == BUF_DIRTY || i == BUF_SHARED ||
                   nr_buffers_type[i] == 0) {
                        candidate[i] = NULL;
                        buffers[i] = 0;
                        continue;
                }
                buffers[i] = nr_buffers_type[i];
                for (bh = lru_list[i]; buffers[i] > 0; bh = tmp, buffers[i]--)
                 {
                         if(buffers[i] < 0) panic("Here is the problem");
                         tmp = bh->b_next_free;
                         if (!bh) break;
#ifndef OS2
                         if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1 ||
#else
                         if (
#endif
                             bh->b_dirt) {
                                 refile_buffer(bh);
                                 continue;
                         };

                         if (bh->b_count || bh->b_size != size)
                                  continue;

                         /* Buffers are written in the order they are placed
                            on the locked list. If we encounter a locked
                            buffer here, this means that the rest of them
                            are also locked */
                         if(bh->b_lock && (i == BUF_LOCKED || i == BUF_LOCKED1)) {
                                 buffers[i] = 0;
                                 break;
                         }

                         if (BADNESS(bh)) continue;
                         break;
                 };
                if(!buffers[i]) candidate[i] = NULL; /* Nothing on this list */
                else candidate[i] = bh;
                if(candidate[i] && candidate[i]->b_count) panic("Here is the problem");
        }

 repeat:
        if(needed <= 0) return;

        /* Now see which candidate wins the election */

        winner = best_time = UINT_MAX;
        for(i=0; i<NR_LIST; i++){
                if(!candidate[i]) continue;
                if(candidate[i]->b_lru_time < best_time){
                        best_time = candidate[i]->b_lru_time;
                        winner = i;
                }
        }

        /* If we have a winner, use it, and then get a new candidate from that list */
        if(winner != UINT_MAX) {
                i = winner;
                bh = candidate[i];
                candidate[i] = bh->b_next_free;
                if(candidate[i] == bh) candidate[i] = NULL;  /* Got last one */
                if (bh->b_count || bh->b_size != size)
                         panic("Busy buffer in candidate list\n");
#ifndef OS2
                if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1)
                         panic("Shared buffer in candidate list\n");
#endif
                if (BADNESS(bh)) panic("Buffer in candidate list with BADNESS != 0\n");

                if(bh->b_dev == 0xffff) panic("Wrong list");
                remove_from_queues(bh);
                bh->b_dev = 0xffff;
                put_last_free(bh);
                needed -= bh->b_size;
                buffers[i]--;
                if(buffers[i] < 0) panic("Here is the problem");

                if(buffers[i] == 0) candidate[i] = NULL;

                /* Now all we need to do is advance the candidate pointer
                   from the winner list to the next usable buffer */
                if(candidate[i] && buffers[i] > 0){
                        if(buffers[i] <= 0) panic("Here is another problem");
                        for (bh = candidate[i]; buffers[i] > 0; bh = tmp, buffers[i]--) {
                                if(buffers[i] < 0) panic("Here is the problem");
                                tmp = bh->b_next_free;
                                if (!bh) break;
#ifndef OS2
                                if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1 ||
#else
                                if (
#endif
                                    bh->b_dirt) {

                                        refile_buffer(bh);
                                        continue;
                                };

                                if (bh->b_count || bh->b_size != size)
                                         continue;

                                /* Buffers are written in the order they are
                                   placed on the locked list.  If we encounter
                                   a locked buffer here, this means that the
                                   rest of them are also locked */
                                if(bh->b_lock && (i == BUF_LOCKED || i == BUF_LOCKED1)) {
                                        buffers[i] = 0;
                                        break;
                                }

                                if (BADNESS(bh)) continue;
                                break;
                        };
                        if(!buffers[i]) candidate[i] = NULL; /* Nothing here */
                        else candidate[i] = bh;
                        if(candidate[i] && candidate[i]->b_count)
                                 panic("Here is the problem");
                }

                goto repeat;
        }

        if(needed <= 0) return;

        /* Too bad, that was not enough. Try a little harder to grow some. */
#ifndef OS2
        if (nr_free_pages > 5) {
#else
        if (buffermem < cache_size) {
#endif
                if (grow_buffers(GFP_BUFFER, size)) {
                        needed -= PAGE_SIZE;
                        goto repeat0;
                };
        }

        /* and repeat until we find something good */
        if (!grow_buffers(GFP_ATOMIC, size))
                wakeup_bdflush(1);
        needed -= PAGE_SIZE;
        goto repeat0;
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algorithm is changed: hopefully better, and an elusive bug removed.
 *
 * 14.02.92: changed it to sync dirty buffers a bit: better performance
 * when the filesystem starts to get full of dirty blocks (I hope).
 */
struct buffer_head * getblk(dev_t dev, int block, int size)
{
        struct buffer_head * bh;
        int isize = BUFSIZE_INDEX(size);

        /* Update this for the buffer size lav. */
        buffer_usage[isize]++;

        /* If there are too many dirty buffers, we wake up the update process
           now so as to ensure that there are still clean buffers available
           for user processes to use (and dirty) */
repeat:
        bh = get_hash_table(dev, block, size);
        if (bh) {
                if (bh->b_uptodate && !bh->b_dirt)
                         put_last_lru(bh);
                if(!bh->b_dirt) bh->b_flushtime = 0;
                return bh;
        }

        while(!free_list[isize]) refill_freelist(size);

        if (find_buffer(dev,block,size))
                 goto repeat;

        bh = free_list[isize];
        remove_from_free_list(bh);

/* OK, FINALLY we know that this buffer is the only one of its kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
        bh->b_count=1;
        bh->b_dirt=0;
        bh->b_lock=0;
        bh->b_uptodate=0;
        bh->b_flushtime = 0;
        bh->b_req=0;
        bh->b_dev=dev;
        bh->b_blocknr=block;
        insert_into_queues(bh);
        return bh;
}

void set_writetime(struct buffer_head * buf, int flag)
{
        int newtime;

        if (buf->b_dirt){
                /* Move buffer to dirty list if jiffies is clear */
                newtime = jiffies + (flag ? bdf_prm.b_un.age_super :
                                     bdf_prm.b_un.age_buffer);
                if(!buf->b_flushtime || buf->b_flushtime > newtime)
                         buf->b_flushtime = newtime;
        } else {
                buf->b_flushtime = 0;
        }
}

/*
 * WARNING this routine is accessed in interrupt context.
 */
//#pragma alloc_text(EXT2_FIXED_CODE, refile_buffer)
void refile_buffer(struct buffer_head * buf){
        int dispose;
        if(buf->b_dev == 0xffff) panic("Attempt to refile free buffer\n");
        if (buf->b_dirt)
                dispose = BUF_DIRTY;
#ifndef OS2
        else if (mem_map[MAP_NR((unsigned long) buf->b_data)] > 1)
                dispose = BUF_SHARED;
#endif
        else if (buf->b_lock)
                dispose = BUF_LOCKED;
        else if (buf->b_list == BUF_SHARED)
                dispose = BUF_UNSHARED;
        else
                dispose = BUF_CLEAN;
        if(dispose == BUF_CLEAN) buf->b_lru_time = jiffies;
        if(dispose != buf->b_list)  {
                if(dispose == BUF_DIRTY || dispose == BUF_UNSHARED)
                         buf->b_lru_time = jiffies;
                if(dispose == BUF_LOCKED &&
                   (buf->b_flushtime - buf->b_lru_time) <= bdf_prm.b_un.age_super)
                         dispose = BUF_LOCKED1;
                remove_from_queues(buf);
                buf->b_list = dispose;
                insert_into_queues(buf);
                /*
                 * wakeup_bdflush CANNOT be entered in interrupt context, but refile_buffer
                 * is called in interrupt context ONLY for dispose = BUF_LOCKED, so we are safe
                 * here.
                 */
                if(dispose == BUF_DIRTY && nr_buffers_type[BUF_DIRTY] >
                   (nr_buffers - nr_buffers_type[BUF_SHARED]) *
                   bdf_prm.b_un.nfract/100)
                         wakeup_bdflush(0);
        }
}

void brelse(struct buffer_head * buf)
{
        if (!buf)
                return;
        wait_on_buffer(buf);

        /* If dirty, mark the time this buffer should be written back */
        set_writetime(buf, 0);
        refile_buffer(buf);

        if (buf->b_count) {
                if (--buf->b_count)
                        return;
                wake_up(&buffer_wait);
                return;
        }
        printk("VFS: brelse: Trying to free free buffer\n");
}

/*
 * bforget() is like brelse(), except is throws the buffer away
 */
void bforget(struct buffer_head * buf)
{
        wait_on_buffer(buf);
        if (buf->b_count != 1) {
                printk("Aieee... bforget(): count = %d\n", buf->b_count);
                return;
        }
#ifndef OS2
        if (mem_map[MAP_NR(buf->b_data)].count != 1) {
                printk("Aieee... bforget(): shared buffer\n");
                return;
        }
#endif
        mark_buffer_clean(buf);
        buf->b_count = 0;
        remove_from_queues(buf);
        buf->b_dev = 0xFFFF;
        put_last_free(buf);
        wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head * bread(dev_t dev, int block, int size)
{
        struct buffer_head * bh;

        if (!(bh = getblk(dev, block, size))) {
                printk("VFS: bread: READ error on device %d/%d\n",
                                                MAJOR(dev), MINOR(dev));
                return NULL;
        }
        if (bh->b_uptodate)
                return bh;
        ll_rw_block(READ, 1, __StackToFlat(&bh));
        wait_on_buffer(bh);
        if (bh->b_uptodate)
                return bh;
        brelse(bh);
        return NULL;
}

#ifndef OS2
/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */

#define NBUF 16

struct buffer_head * breada(dev_t dev, int block, int bufsize,
        unsigned int pos, unsigned int filesize)
{
        struct buffer_head * bhlist[NBUF];
        unsigned int blocks;
        struct buffer_head * bh;
        int index;
        int i, j;

        if (pos >= filesize)
                return NULL;

        if (block < 0 || !(bh = getblk(dev,block,bufsize)))
                return NULL;

        index = BUFSIZE_INDEX(bh->b_size);

        if (bh->b_uptodate)
                return bh;

        blocks = ((filesize & (bufsize - 1)) - (pos & (bufsize - 1))) >> (9+index);

        if (blocks > (read_ahead[MAJOR(dev)] >> index))
                blocks = read_ahead[MAJOR(dev)] >> index;
        if (blocks > NBUF)
                blocks = NBUF;

        bhlist[0] = bh;
        j = 1;
        for(i=1; i<blocks; i++) {
                bh = getblk(dev,block+i,bufsize);
                if (bh->b_uptodate) {
                        brelse(bh);
                        break;
                }
                bhlist[j++] = bh;
        }

        /* Request the read for these buffers, and then release them */
        ll_rw_block(READ, j, bhlist);

        for(i=1; i<j; i++)
                brelse(bhlist[i]);

        /* Wait for this buffer, and then continue on */
        bh = bhlist[0];
        wait_on_buffer(bh);
        if (bh->b_uptodate)
                return bh;
        brelse(bh);
        return NULL;
}
#endif

/*
 * See fs/inode.c for the weird use of volatile..
 */
static void put_unused_buffer_head(struct buffer_head * bh)
{
#ifndef OS2
        struct wait_queue * wait;

        wait = ((volatile struct buffer_head *) bh)->b_wait;
        memset(bh,0,sizeof(*bh));
        ((volatile struct buffer_head *) bh)->b_wait = wait;
#else
        memset(bh,0,sizeof(*bh));
#endif
        bh->b_next_free = unused_list;
        unused_list = bh;
}

static void get_more_buffer_heads(void)
{
        int i;
        struct buffer_head * bh;

        if (unused_list)
                return;

        if (!(bh = (struct buffer_head*) get_free_page(GFP_BUFFER)))
                return;
#ifdef OS2
    memset(bh, 0, PAGE_SIZE);
#endif
        for (nr_buffer_heads+=i=PAGE_SIZE/sizeof*bh ; i>0; i--) {
                bh->b_next_free = unused_list;        /* only make link */
                unused_list = bh++;
        }
}

static struct buffer_head * get_unused_buffer_head(void)
{
        struct buffer_head * bh;

        get_more_buffer_heads();
        if (!unused_list)
                return NULL;
        bh = unused_list;
        unused_list = bh->b_next_free;
        bh->b_next_free = NULL;
        bh->b_data = NULL;
        bh->b_size = 0;
        bh->b_req = 0;
        return bh;
}

/*
 * Create the appropriate buffers when given a page for data area and
 * the size of each buffer.. Use the bh->b_this_page linked list to
 * follow the buffers created.  Return NULL if unable to create more
 * buffers.
 */
static struct buffer_head * create_buffers(unsigned long page, unsigned long size)
{
        struct buffer_head *bh, *head;
        unsigned long offset;

#ifdef OS2
        struct PageList page_list;
    unsigned long   physaddr;
        if (DevHlp32_LinToPageList((void *)page, size, __StackToFlat(&page_list), 0) != NO_ERROR) {
            ext2_os2_panic(1, "LinToPageList failed");
        }
        physaddr = page_list.physaddr;
#endif

        head = NULL;
        offset = PAGE_SIZE;
        while ((offset -= size) < PAGE_SIZE) {
                bh = get_unused_buffer_head();
                if (!bh)
                        goto no_grow;
                bh->b_this_page = head;
                head = bh;
                bh->b_data = (char *) (page+offset);
                bh->b_size = size;
                bh->b_dev = 0xffff;  /* Flag as unused */
#ifdef OS2
        bh->b_physaddr = physaddr + offset;
#endif
        }
        return head;
/*
 * In case anything failed, we just free everything we got.
 */
no_grow:
        bh = head;
        while (bh) {
                head = bh;
                bh = bh->b_this_page;
                put_unused_buffer_head(head);
        }
        return NULL;
}

#ifndef OS2
static void read_buffers(struct buffer_head * bh[], int nrbuf)
{
        int i;
        int bhnum = 0;
        struct buffer_head * bhr[MAX_BUF_PER_PAGE];

        for (i = 0 ; i < nrbuf ; i++) {
                if (bh[i] && !bh[i]->b_uptodate)
                        bhr[bhnum++] = bh[i];
        }
        if (bhnum)
                ll_rw_block(READ, bhnum, bhr);
        for (i = 0 ; i < nrbuf ; i++) {
                if (bh[i]) {
                        wait_on_buffer(bh[i]);
                }
        }
}


/*
 * This actually gets enough info to try to align the stuff,
 * but we don't bother yet.. We'll have to check that nobody
 * else uses the buffers etc.
 *
 * "address" points to the new page we can use to move things
 * around..
 */
static unsigned long try_to_align(struct buffer_head ** bh, int nrbuf,
        unsigned long address)
{
        while (nrbuf-- > 0)
                brelse(bh[nrbuf]);
        return 0;
}

static unsigned long check_aligned(struct buffer_head * first, unsigned long address,
        dev_t dev, int *b, int size)
{
        struct buffer_head * bh[MAX_BUF_PER_PAGE];
        unsigned long page;
        unsigned long offset;
        int block;
        int nrbuf;
        int aligned = 1;

        bh[0] = first;
        nrbuf = 1;
        page = (unsigned long) first->b_data;
        if (page & ~PAGE_MASK)
                aligned = 0;
        for (offset = size ; offset < PAGE_SIZE ; offset += size) {
                block = *++b;
                if (!block)
                        goto no_go;
                first = get_hash_table(dev, block, size);
                if (!first)
                        goto no_go;
                bh[nrbuf++] = first;
                if (page+offset != (unsigned long) first->b_data)
                        aligned = 0;
        }
        if (!aligned)
                return try_to_align(bh, nrbuf, address);
#ifndef OS2
        mem_map[MAP_NR(page)]++;
#endif
        read_buffers(bh,nrbuf);                /* make sure they are actually read correctly */
        while (nrbuf-- > 0)
                brelse(bh[nrbuf]);
        free_page(address);
        ++current->mm->min_flt;
        return page;
no_go:
        while (nrbuf-- > 0)
                brelse(bh[nrbuf]);
        return 0;
}

static unsigned long try_to_load_aligned(unsigned long address,
        dev_t dev, int b[], int size)
{
        struct buffer_head * bh, * tmp, * arr[MAX_BUF_PER_PAGE];
        unsigned long offset;
        int isize = BUFSIZE_INDEX(size);
        int * p;
        int block;

        bh = create_buffers(address, size);
        if (!bh)
                return 0;
        /* do any of the buffers already exist? punt if so.. */
        p = b;
        for (offset = 0 ; offset < PAGE_SIZE ; offset += size) {
                block = *(p++);
                if (!block)
                        goto not_aligned;
                if (find_buffer(dev, block, size))
                        goto not_aligned;
        }
        tmp = bh;
        p = b;
        block = 0;
        while (1) {
                arr[block++] = bh;
                bh->b_count = 1;
                bh->b_dirt = 0;
                bh->b_flushtime = 0;
                bh->b_uptodate = 0;
                bh->b_req = 0;
                bh->b_dev = dev;
                bh->b_blocknr = *(p++);
                bh->b_list = BUF_CLEAN;
                nr_buffers++;
                nr_buffers_size[isize]++;
                insert_into_queues(bh);
                if (bh->b_this_page)
                        bh = bh->b_this_page;
                else
                        break;
        }
        buffermem += PAGE_SIZE;
        bh->b_this_page = tmp;
#ifndef OS2
        mem_map[MAP_NR(address)]++;
        buffer_pages[MAP_NR(address)] = bh;
#endif
        read_buffers(arr,block);
        while (block-- > 0)
                brelse(arr[block]);
        ++current->mm->maj_flt;
        return address;
not_aligned:
        while ((tmp = bh) != NULL) {
                bh = bh->b_this_page;
                put_unused_buffer_head(tmp);
        }
        return 0;
}

/*
 * Try-to-share-buffers tries to minimize memory use by trying to keep
 * both code pages and the buffer area in the same page. This is done by
 * (a) checking if the buffers are already aligned correctly in memory and
 * (b) if none of the buffer heads are in memory at all, trying to load
 * them into memory the way we want them.
 *
 * This doesn't guarantee that the memory is shared, but should under most
 * circumstances work very well indeed (ie >90% sharing of code pages on
 * demand-loadable executables).
 */
INLINE unsigned long try_to_share_buffers(unsigned long address,
        dev_t dev, int *b, int size)
{
        struct buffer_head * bh;
        int block;

        block = b[0];
        if (!block)
                return 0;
        bh = get_hash_table(dev, block, size);
        if (bh)
                return check_aligned(bh, address, dev, b, size);
        return try_to_load_aligned(address, dev, b, size);
}

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc. This also allows us to optimize memory usage by sharing code pages
 * and filesystem buffers..
 */
unsigned long bread_page(unsigned long address, dev_t dev, int b[], int size, int no_share)
{
        struct buffer_head * bh[MAX_BUF_PER_PAGE];
        unsigned long where;
        int i, j;

        if (!no_share) {
                where = try_to_share_buffers(address, dev, b, size);
                if (where)
                        return where;
        }
        ++current->mm->maj_flt;
         for (i=0, j=0; j<PAGE_SIZE ; i++, j+= size) {
                bh[i] = NULL;
                if (b[i])
                        bh[i] = getblk(dev, b[i], size);
        }
        read_buffers(bh,i);
        where = address;
         for (i=0, j=0; j<PAGE_SIZE ; i++, j += size, where += size) {
                if (bh[i]) {
                        if (bh[i]->b_uptodate)
                                memcpy((void *) where, bh[i]->b_data, size);
                        brelse(bh[i]);
                }
        }
        return address;
}
#endif /* !OS2 */

/*
 * Try to increase the number of buffers available: the size argument
 * is used to determine what kind of buffers we want.
 */
static int grow_buffers(int pri, int size)
{
        unsigned long page;
        struct buffer_head *bh, *tmp;
        struct buffer_head * insert_point;
        int isize;

        if ((size & 511) || (size > PAGE_SIZE)) {
                printk("VFS: grow_buffers: size = %d\n",size);
                return 0;
        }

        isize = BUFSIZE_INDEX(size);
#ifdef OS2
    if (buffermem + PAGE_SIZE > cache_size)
        return 0;
    buffermem += PAGE_SIZE;
#endif
        if (!(page = __get_free_page(pri)))
#ifndef OS2
                return 0;
#else
    {
            buffermem -= PAGE_SIZE;
        return 0;
        }
#endif

        bh = create_buffers(page, size);
        if (!bh) {
                free_page(page);
#ifdef OS2
        buffermem -= PAGE_SIZE;
#endif
                return 0;
        }

        insert_point = free_list[isize];

        tmp = bh;
        while (1) {
                nr_free[isize]++;
                if (insert_point) {
                        tmp->b_next_free = insert_point->b_next_free;
                        tmp->b_prev_free = insert_point;
                        insert_point->b_next_free->b_prev_free = tmp;
                        insert_point->b_next_free = tmp;
                } else {
                        tmp->b_prev_free = tmp;
                        tmp->b_next_free = tmp;
                }
                insert_point = tmp;
                ++nr_buffers;
                if (tmp->b_this_page)
                        tmp = tmp->b_this_page;
                else
                        break;
        }
        free_list[isize] = bh;
#ifndef OS2
        buffer_pages[MAP_NR(page)] = bh;
#endif
        tmp->b_this_page = bh;
        wake_up(&buffer_wait);
#ifndef OS2
        buffermem += PAGE_SIZE;
#endif
        return 1;
}


/* =========== Reduce the buffer memory ============= */

/*
 * try_to_free() checks if all the buffers on this particular page
 * are unused, and free's the page if so.
 */
static int try_to_free(struct buffer_head * bh, struct buffer_head ** bhp)
{
        unsigned long page;
        struct buffer_head * tmp, * p;
        int isize = BUFSIZE_INDEX(bh->b_size);

        *bhp = bh;
        page = (unsigned long) bh->b_data;
//#ifndef OS2
        page &= PAGE_MASK;
//#endif
        tmp = bh;
        do {
                if (!tmp)
                        return 0;
#ifndef OS2
                if (tmp->b_count || tmp->b_dirt || tmp->b_lock || tmp->b_wait)
#else
                if (tmp->b_count || tmp->b_dirt || tmp->b_lock)
#endif
                        return 0;
                tmp = tmp->b_this_page;
        } while (tmp != bh);
        tmp = bh;
        do {
                p = tmp;
                tmp = tmp->b_this_page;
                nr_buffers--;
                nr_buffers_size[isize]--;
                if (p == *bhp)
                  {
                    *bhp = p->b_prev_free;
                    if (p == *bhp) /* Was this the last in the list? */
                      *bhp = NULL;
                  }
                remove_from_queues(p);
                put_unused_buffer_head(p);
        } while (tmp != bh);
        buffermem -= PAGE_SIZE;
#ifndef OS2
        buffer_pages[MAP_NR(page)] = NULL;
#endif
        free_page(page);
#ifndef OS2
        return !mem_map[MAP_NR(page)];
#else
        return 1;
#endif
}


/*
 * Consult the load average for buffers and decide whether or not
 * we should shrink the buffers of one size or not.  If we decide yes,
 * do it and return 1.  Else return 0.  Do not attempt to shrink size
 * that is specified.
 *
 * I would prefer not to use a load average, but the way things are now it
 * seems unavoidable.  The way to get rid of it would be to force clustering
 * universally, so that when we reclaim buffers we always reclaim an entire
 * page.  Doing this would mean that we all need to move towards QMAGIC.
 */
static int maybe_shrink_lav_buffers(int size)
{
        int nlist;
        int isize;
        int total_lav, total_n_buffers, n_sizes;

        /* Do not consider the shared buffers since they would not tend
           to have getblk called very often, and this would throw off
           the lav.  They are not easily reclaimable anyway (let the swapper
           make the first move). */

        total_lav = total_n_buffers = n_sizes = 0;
        for(nlist = 0; nlist < NR_SIZES; nlist++)
         {
                 total_lav += buffers_lav[nlist];
                 if(nr_buffers_size[nlist]) n_sizes++;
                 total_n_buffers += nr_buffers_size[nlist];
                 total_n_buffers -= nr_buffers_st[nlist][BUF_SHARED];
         }

        /* See if we have an excessive number of buffers of a particular
           size - if so, victimize that bunch. */

        isize = (size ? BUFSIZE_INDEX(size) : -1);

        if (n_sizes > 1)
                 for(nlist = 0; nlist < NR_SIZES; nlist++)
                  {
                          if(nlist == isize) continue;
                          if(nr_buffers_size[nlist] &&
                             bdf_prm.b_un.lav_const * buffers_lav[nlist]*total_n_buffers <
                             total_lav * (nr_buffers_size[nlist] - nr_buffers_st[nlist][BUF_SHARED]))
                                   if(shrink_specific_buffers(6, bufferindex_size[nlist]))
                                            return 1;
                  }
        return 0;
}
/*
 * Try to free up some pages by shrinking the buffer-cache
 *
 * Priority tells the routine how hard to try to shrink the
 * buffers: 3 means "don't bother too much", while a value
 * of 0 means "we'd better get some free pages now".
 */
int shrink_buffers(unsigned int priority)
{
        if (priority < 2) {
                sync_buffers(0,0);
        }

        if(priority == 2) wakeup_bdflush(1);

        if(maybe_shrink_lav_buffers(0)) return 1;

        /* No good candidate size - take any size we can find */
        return shrink_specific_buffers(priority, 0);
}

static int shrink_specific_buffers(unsigned int priority, int size)
{
        struct buffer_head *bh;
        int nlist;
        int i, isize, isize1;

#ifdef DEBUG
        if(size) printk("Shrinking buffers of size %d\n", size);
#endif
        /* First try the free lists, and see if we can get a complete page
           from here */
        isize1 = (size ? BUFSIZE_INDEX(size) : -1);

        for(isize = 0; isize<NR_SIZES; isize++){
                if(isize1 != -1 && isize1 != isize) continue;
                bh = free_list[isize];
                if(!bh) continue;
                for (i=0 ; !i || bh != free_list[isize]; bh = bh->b_next_free, i++) {
                        if (bh->b_count || !bh->b_this_page)
                                 continue;
                        if (try_to_free(bh, __StackToFlat(&bh)))
                                 return 1;
                        if(!bh) break; /* Some interrupt must have used it after we
                                          freed the page.  No big deal - keep looking */
                }
        }

        /* Not enough in the free lists, now try the lru list */

        for(nlist = 0; nlist < NR_LIST; nlist++) {
        repeat1:
                if(priority > 3 && nlist == BUF_SHARED) continue;
                bh = lru_list[nlist];
                if(!bh) continue;
                i = 2*nr_buffers_type[nlist] >> priority;
                for ( ; i-- > 0 ; bh = bh->b_next_free) {
                        /* We may have stalled while waiting for I/O to complete. */
                        if(bh->b_list != nlist) goto repeat1;
                        if (bh->b_count || !bh->b_this_page)
                                 continue;
                        if(size && bh->b_size != size) continue;
                        if (bh->b_lock)
                                 if (priority)
                                          continue;
                                 else
                                          wait_on_buffer(bh);
                        if (bh->b_dirt) {
                                bh->b_count++;
                                bh->b_flushtime = 0;
                                ll_rw_block(WRITEA, 1, __StackToFlat(&bh));
                                bh->b_count--;
                                continue;
                        }
                        if (try_to_free(bh, __StackToFlat(&bh)))
                                 return 1;
                        if(!bh) break;
                }
        }
        return 0;
}

// #ifndef OS2
/* ================== Debugging =================== */

void show_buffers(void)
{
        struct buffer_head * bh;
        int found = 0, locked = 0, dirty = 0, used = 0, lastused = 0;
        int shared;
        int nlist, isize;

        printk("Buffer memory:   %6dkB\n",buffermem>>10);
        printk("Buffer heads:    %6d\n",nr_buffer_heads);
        printk("Buffer blocks:   %6d\n",nr_buffers);

        for(nlist = 0; nlist < NR_LIST; nlist++) {
          shared = found = locked = dirty = used = lastused = 0;
          bh = lru_list[nlist];
          if(!bh) continue;
          do {
                found++;
                if (bh->b_lock)
                        locked++;
                if (bh->b_dirt)
                        dirty++;
#ifndef OS2
                if(mem_map[MAP_NR(((unsigned long) bh->b_data))] !=1) shared++;
#endif
                if (bh->b_count)
                        used++, lastused = found;
                bh = bh->b_next_free;
              } while (bh != lru_list[nlist]);
        printk("Buffer[%d] mem: %d buffers, %d used (last=%d), %d locked, %d dirty %d shrd\n",
                nlist, found, used, lastused, locked, dirty, shared);
        };
        printk("Size    [LAV]     Free  Clean  Unshar     Lck    Lck1   Dirty  Shared\n");
        for(isize = 0; isize<NR_SIZES; isize++){
                printk("%5d [%5d]: %7d ", bufferindex_size[isize],
                       buffers_lav[isize], nr_free[isize]);
                for(nlist = 0; nlist < NR_LIST; nlist++)
                         printk("%7d ", nr_buffers_st[isize][nlist]);
                printk("\n");
        }
}
// #endif

#ifndef OS2
/* ====================== Cluster patches for ext2 ==================== */

/*
 * try_to_reassign() checks if all the buffers on this particular page
 * are unused, and reassign to a new cluster them if this is true.
 */
INLINE int try_to_reassign(struct buffer_head * bh, struct buffer_head ** bhp,
                           dev_t dev, unsigned int starting_block)
{
        unsigned long page;
        struct buffer_head * tmp, * p;

        *bhp = bh;
        page = (unsigned long) bh->b_data;
        page &= PAGE_MASK;
#ifndef OS2
        if(mem_map[MAP_NR(page)] != 1) return 0;
#endif
        tmp = bh;
        do {
                if (!tmp)
                         return 0;

                if (tmp->b_count || tmp->b_dirt || tmp->b_lock)
                         return 0;
                tmp = tmp->b_this_page;
        } while (tmp != bh);
        tmp = bh;

        while((unsigned long) tmp->b_data & (PAGE_SIZE - 1))
                 tmp = tmp->b_this_page;

        /* This is the buffer at the head of the page */
        bh = tmp;
        do {
                p = tmp;
                tmp = tmp->b_this_page;
                remove_from_queues(p);
                p->b_dev=dev;
                p->b_uptodate = 0;
                p->b_req = 0;
                p->b_blocknr=starting_block++;
                insert_into_queues(p);
        } while (tmp != bh);
        return 1;
}

/*
 * Try to find a free cluster by locating a page where
 * all of the buffers are unused.  We would like this function
 * to be atomic, so we do not call anything that might cause
 * the process to sleep.  The priority is somewhat similar to
 * the priority used in shrink_buffers.
 *
 * My thinking is that the kernel should end up using whole
 * pages for the buffer cache as much of the time as possible.
 * This way the other buffers on a particular page are likely
 * to be very near each other on the free list, and we will not
 * be expiring data prematurely.  For now we only cannibalize buffers
 * of the same size to keep the code simpler.
 */
static int reassign_cluster(dev_t dev,
                     unsigned int starting_block, int size)
{
        struct buffer_head *bh;
        int isize = BUFSIZE_INDEX(size);
        int i;

        /* We want to give ourselves a really good shot at generating
           a cluster, and since we only take buffers from the free
           list, we "overfill" it a little. */

        while(nr_free[isize] < 32) refill_freelist(size);

        bh = free_list[isize];
        if(bh)
                 for (i=0 ; !i || bh != free_list[isize] ; bh = bh->b_next_free, i++) {
                         if (!bh->b_this_page)        continue;
                         if (try_to_reassign(bh, __StackToFlat(&bh), dev, starting_block))
                                 return 4;
                 }
        return 0;
}

/* This function tries to generate a new cluster of buffers
 * from a new page in memory.  We should only do this if we have
 * not expanded the buffer cache to the maximum size that we allow.
 */
static unsigned long try_to_generate_cluster(dev_t dev, int block, int size)
{
        struct buffer_head * bh, * tmp, * arr[MAX_BUF_PER_PAGE];
        int isize = BUFSIZE_INDEX(size);
        unsigned long offset;
        unsigned long page;
        int nblock;

        page = get_free_page(GFP_NOBUFFER);
        if(!page) return 0;

        bh = create_buffers(page, size);
        if (!bh) {
                free_page(page);
                return 0;
        };
        nblock = block;
        for (offset = 0 ; offset < PAGE_SIZE ; offset += size) {
                if (find_buffer(dev, nblock++, size))
                         goto not_aligned;
        }
        tmp = bh;
        nblock = 0;
        while (1) {
                arr[nblock++] = bh;
                bh->b_count = 1;
                bh->b_dirt = 0;
                bh->b_flushtime = 0;
                bh->b_lock = 0;
                bh->b_uptodate = 0;
                bh->b_req = 0;
                bh->b_dev = dev;
                bh->b_list = BUF_CLEAN;
                bh->b_blocknr = block++;
                nr_buffers++;
                nr_buffers_size[isize]++;
                insert_into_queues(bh);
                if (bh->b_this_page)
                        bh = bh->b_this_page;
                else
                        break;
        }
        buffermem += PAGE_SIZE;
#ifndef OS2
        buffer_pages[MAP_NR(page)] = bh;
#endif
        bh->b_this_page = tmp;
        while (nblock-- > 0)
                brelse(arr[nblock]);
        return 4; /* ?? */
not_aligned:
        while ((tmp = bh) != NULL) {
                bh = bh->b_this_page;
                put_unused_buffer_head(tmp);
        }
        free_page(page);
        return 0;
}

unsigned long generate_cluster(dev_t dev, int b[], int size)
{
        int i, offset;

        for (i = 0, offset = 0 ; offset < PAGE_SIZE ; i++, offset += size) {
                if(i && b[i]-1 != b[i-1]) return 0;  /* No need to cluster */
                if(find_buffer(dev, b[i], size)) return 0;
        };

        /* OK, we have a candidate for a new cluster */

        /* See if one size of buffer is over-represented in the buffer cache,
           if so reduce the numbers of buffers */
        if(maybe_shrink_lav_buffers(size))
         {
                 int retval;
                 retval = try_to_generate_cluster(dev, b[0], size);
                 if(retval) return retval;
         };
#ifndef OS2
        if (nr_free_pages > min_free_pages*2)
#else
        if (buffermem < cache_size)
#endif
                 return try_to_generate_cluster(dev, b[0], size);
        else
                 return reassign_cluster(dev, b[0], size);
}

#endif /* OS2 */

/* ===================== Init ======================= */
#ifndef OS2
/*
 * This initializes the initial buffer free list.  nr_buffers_type is set
 * to one less the actual number of buffers, as a sop to backwards
 * compatibility --- the old code did this (I think unintentionally,
 * but I'm not sure), and programs in the ps package expect it.
 *                                         - TYT 8/30/92
 */
void buffer_init(void)
{
        int i;
        int isize = BUFSIZE_INDEX(BLOCK_SIZE);

        if (high_memory >= 4*1024*1024) {
                if(high_memory >= 16*1024*1024)
                         nr_hash = 16381;
                else
                         nr_hash = 4093;
        } else {
                nr_hash = 997;
        };

        hash_table = (struct buffer_head **) vmalloc(nr_hash *
                                                     sizeof(struct buffer_head *));


        buffer_pages = (struct buffer_head **) vmalloc(MAP_NR(high_memory) *
                                                     sizeof(struct buffer_head *));
        for (i = 0 ; i < MAP_NR(high_memory) ; i++)
                buffer_pages[i] = NULL;

        for (i = 0 ; i < nr_hash ; i++)
                hash_table[i] = NULL;
        lru_list[BUF_CLEAN] = 0;
        grow_buffers(GFP_KERNEL, BLOCK_SIZE);
        if (!free_list[isize])
                panic("VFS: Unable to initialize buffer free list!");
        return;
}
#else
void buffer_init(void) {
    int isize = BUFSIZE_INDEX(BLOCK_SIZE);
    unsigned long cache;
    unsigned long nbufs;
    long i;
    int rc;

    /*
     * Computes the hash table size, given the maximun cache size option on the IFS = command line,
     * and a default block size of 1 Kb.
     */
    cache = ((cache_size + PAGE_SIZE -1) / PAGE_SIZE) * PAGE_SIZE;
    nbufs = cache / BLOCK_SIZE;

    nr_hash = nbufs;

    kernel_printf("Disk block hash table has %lu entries", nr_hash);

    /*
     * Allocation of the disk block hash table
     */
    if (!hash_table) {
        /*
         * Hash table in NON swappable memory.
         */
        rc = DevHlp32_VMAlloc(nbufs * sizeof(struct buffer_head *), VMDHA_NOPHYSADDR, VMDHA_FIXED, (void **)(&hash_table));
        if (rc != NO_ERROR)
            ext2_os2_panic(1, "buffer_init() - Couldn't allocate disk block hash table");
        for (i = 0 ; i < nr_hash  ; i++) {
            hash_table[i] = 0;
        }
        lru_list[BUF_CLEAN] = 0;
        grow_buffers(GFP_KERNEL, BLOCK_SIZE);
        if (!free_list[isize])
                panic("VFS: Unable to initialize buffer free list!");
    }


}


#endif

/* ====================== bdflush support =================== */

/* This is a simple kernel daemon, whose job it is to provide a dynamically
 * response to dirty buffers.  Once this process is activated, we write back
 * a limited number of buffers to the disks and then go back to sleep again.
 * In effect this is a process which never leaves kernel mode, and does not have
 * any user memory associated with it except for the stack.  There is also
 * a kernel stack page, which obviously must be separate from the user stack.
 */
#ifndef OS2
struct wait_queue * bdflush_wait = NULL;
struct wait_queue * bdflush_done = NULL;
#else
unsigned long bdflush_wait = 0;
unsigned long bdflush_done = 0;
#endif

static int bdflush_running = 0;


static void wakeup_bdflush(int wait)
{
        if(!bdflush_running){
                printk("Warning - bdflush not running\n");
                sync_buffers(0,0);
                return;
        };
        wake_up(&bdflush_wait);
        if(wait) sleep_on(&bdflush_done);
}



/*
 * Here we attempt to write back old buffers.  We also try and flush inodes
 * and supers as well, since this function is essentially "update", and
 * otherwise there would be no way of ensuring that these quantities ever
 * get written back.  Ideally, we would have a timestamp on the inodes
 * and superblocks so that we could write back only the old ones as well
 */

asmlinkage int sync_old_buffers(void)
{
        int i, isize;
        int ndirty, nwritten;
        int nlist;
        int ncount;
        struct buffer_head * bh, *next;
#ifndef OS2
        sync_supers(0);
#endif
        sync_inodes(0);

        ncount = 0;
#ifdef DEBUG
        for(nlist = 0; nlist < NR_LIST; nlist++)
#else
        for(nlist = BUF_DIRTY; nlist <= BUF_DIRTY; nlist++)
#endif
        {
                ndirty = 0;
                nwritten = 0;
        repeat:
                bh = lru_list[nlist];
                if(bh)
                         for (i = nr_buffers_type[nlist]; i-- > 0; bh = next) {
                                 /* We may have stalled while waiting for I/O to complete. */
                                 if(bh->b_list != nlist) goto repeat;
                                 next = bh->b_next_free;
                                 if(!lru_list[nlist]) {
                                         printk("Dirty list empty %d\n", i);
                                         break;
                                 }

                                 /* Clean buffer on dirty list?  Refile it */
                                 if (nlist == BUF_DIRTY && !bh->b_dirt && !bh->b_lock)
                                  {
                                          refile_buffer(bh);
                                          continue;
                                  }

                                 if (bh->b_lock || !bh->b_dirt)
                                          continue;
                                 ndirty++;
                                 if(bh->b_flushtime > jiffies) continue;
                                 nwritten++;
                                 bh->b_count++;
                                 bh->b_flushtime = 0;
#ifdef DEBUG
                                 if(nlist != BUF_DIRTY) ncount++;
#endif
                                 ll_rw_block(WRITE, 1, __StackToFlat(&bh));
                                 bh->b_count--;
                         }
        }
#ifdef DEBUG
        if (ncount) printk("sync_old_buffers: %d dirty buffers not on dirty list\n", ncount);
        printk("Wrote %d/%d buffers\n", nwritten, ndirty);
#endif

        /* We assume that we only come through here on a regular
           schedule, like every 5 seconds.  Now update load averages.
           Shift usage counts to prevent overflow. */
        for(isize = 0; isize<NR_SIZES; isize++){
                CALC_LOAD(buffers_lav[isize], bdf_prm.b_un.lav_const, buffer_usage[isize]);
                buffer_usage[isize] = 0;
        };
        return 0;
}


/* This is the interface to bdflush.  As we get more sophisticated, we can
 * pass tuning parameters to this "process", to adjust how it behaves.  If you
 * invoke this again after you have done this once, you would simply modify
 * the tuning parameters.  We would want to verify each parameter, however,
 * to make sure that it is reasonable. */

asmlinkage int sys_bdflush(int func, long data)
{
        int i, error;
        int ndirty;
        int nlist;
        int ncount;
        struct buffer_head * bh, *next;

#ifndef OS2
        if (!suser())
                return -EPERM;

        if (func == 1)
                 return sync_old_buffers();

        /* Basically func 0 means start, 1 means read param 1, 2 means write param 1, etc */
        if (func >= 2) {
                i = (func-2) >> 1;
                if (i < 0 || i >= N_PARAM)
                        return -EINVAL;
                if((func & 1) == 0) {
                        error = verify_area(VERIFY_WRITE, (void *) data, sizeof(int));
                        if (error)
                                return error;
                        put_fs_long(bdf_prm.data[i], data);
                        return 0;
                };
                if (data < bdflush_min[i] || data > bdflush_max[i])
                        return -EINVAL;
                bdf_prm.data[i] = data;
                return 0;
        };
#endif
        if (bdflush_running)
                return -EBUSY; /* Only one copy of this running at one time */
        bdflush_running++;

        /* OK, from here on is the daemon */

        for (;;) {
#ifdef OS2
//                     struct buffer_head *__bhlist[32];
                     struct buffer_head *bhlist[32];
//                     struct buffer_head **bhlist = __StackToFlat(__bhlist);
                     int nr_bh = 0;
                     int k;
#endif
#ifdef DEBUG
                printk("bdflush() activated...");
#endif

                ncount = 0;
#ifdef DEBUG
                for(nlist = 0; nlist < NR_LIST; nlist++)
#else
                for(nlist = BUF_DIRTY; nlist <= BUF_DIRTY; nlist++)
#endif
                 {

                         ndirty = 0;
                 repeat:
                         bh = lru_list[nlist];
                         if(bh)
                                  for (i = nr_buffers_type[nlist]; i-- > 0 && ndirty < bdf_prm.b_un.ndirty;
                                       bh = next) {
                                          /* We may have stalled while waiting for I/O to complete. */
                                          if(bh->b_list != nlist) goto repeat;
                                          next = bh->b_next_free;
                                          if(!lru_list[nlist]) {
                                                  printk("Dirty list empty %d\n", i);
                                                  break;
                                          }

                                          /* Clean buffer on dirty list?  Refile it */
                                          if (nlist == BUF_DIRTY && !bh->b_dirt && !bh->b_lock)
                                           {
                                                   refile_buffer(bh);
                                                   continue;
                                           }

                                          if (bh->b_lock || !bh->b_dirt)
                                                   continue;
                                          /* Should we write back buffers that are shared or not??
                                             currently dirty buffers are not shared, so it does not matter */
                                          bh->b_count++;
                                          ndirty++;
                                          bh->b_flushtime = 0;
#ifdef OS2
                                          if ((!nr_bh) || (bh->b_dev == bhlist[0]->b_dev)) {
                                              bhlist[nr_bh++] = bh;
                                          } else {
                                              ll_rw_block(WRITE, nr_bh, __StackToFlat(bhlist));
                                              for (k = 0; k < nr_bh; k++) bhlist[k]->b_count --;
                                              nr_bh = 0;
                                              bhlist[nr_bh++] = bh;
                                          }
                                          if (nr_bh == 32) {
                                              /*
                                               * We send the request list to the block device when either the bh array is full,
                                               * or we encounter a buffer for another device
                                               */
                                              ll_rw_block(WRITE, nr_bh, __StackToFlat(bhlist));
                                              for (k = 0; k < nr_bh; k++) bhlist[k]->b_count --;
                                              nr_bh = 0;
                                          }
#else
                                          ll_rw_block(WRITE, 1, &bh);
#endif
#ifdef DEBUG
                                          if(nlist != BUF_DIRTY) ncount++;
#endif
//@@@                                     bh->b_count--;
                                  }
                 }
#ifdef OS2
                 if (nr_bh) {
                     ll_rw_block(WRITE, nr_bh, __StackToFlat(bhlist));
                     for (k = 0; k < nr_bh; k++) bhlist[k]->b_count --;
                     nr_bh = 0;
                 }
#endif

#ifdef DEBUG
                if (ncount) printk("sys_bdflush: %d dirty buffers not on dirty list\n", ncount);
                printk("sleeping again.\n");
#endif
                wake_up(&bdflush_done);

                /* If there are still a lot of dirty buffers around, skip the sleep
                   and flush some more */

                if((nr_buffers_type[BUF_DIRTY] < (nr_buffers - nr_buffers_type[BUF_SHARED]) *
                   bdf_prm.b_un.nfract/100) || (!buffermem)) {
#ifndef OS2
                           if (current->signal & (1 << (SIGKILL-1))) {
                                bdflush_running--;
                                   return 0;
                        }
                           current->signal = 0;
                        interruptible_sleep_on(&bdflush_wait);
#else
                        _disable();
                        DevHlp32_ProcBlock((unsigned long)&bdflush_wait, 10000, 1);
#endif
                }
        }
}


/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * End:
 */

#if 1

void dump_dirty_buffers(void) {

    struct buffer_head *bh, *next;
    printk("-------- Dirty buffers --------");
    bh = lru_list[BUF_DIRTY];
    if (bh) {
      next = bh->b_next_free;
      for (;;) {
        printk("lst=%d sz=%4d nr=%8d cnt=%d uptdt=%d dirt=%d lck=%d dev=%04X",
               bh->b_list, bh->b_size, bh->b_blocknr, bh->b_count,
               bh->b_uptodate, bh->b_dirt, bh->b_lock, bh->b_dev);
        bh=next;
        next=bh->b_next_free;
        if (bh == lru_list[BUF_DIRTY]) break;
      }
    }
}

#endif
