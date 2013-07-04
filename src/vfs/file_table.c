//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/file_table.c,v 1.1 2001/05/09 17:48:06 umoeller Exp $
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
 *  INSPIRED FROM : linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>

#include <string.h>
#ifdef __IBMC__
#include <builtin.h>
#endif
#ifdef MINIFSD
#include <dos.h>
#endif

#include <os2\types.h>
#ifndef MINIFSD
#include <os2\StackToFlat.h>
#include <os2\DevHlp32.h>
#endif
#include  <linux/fs.h>
#include  <linux/fs_proto.h>
#include <os2\os2proto.h>
#include <os2\minifsd.h>
#ifndef MINIFSD
#include <os2\StackToFlat.h>
#endif

extern unsigned long event;

long nr_files           = 0;
long nr_free_files      = 0;
long nr_used_files      = 0;
long nr_dead_files      = 0;

struct file *free_files = 0;
struct file *used_files = 0;
struct file *dead_files = 0;

#ifndef MINIFSD
static struct file *page_files    = 0;          // SWAPPER.DAT support
static long         nr_page_files = 0;          // SWAPPER.DAT support
#endif


/*
 * MUST be called with interrupts disabled
 */
#ifndef MINIFSD
INLINE void remove_from_list(struct file *f, struct file **list_head, long *counter) {
#else
static void remove_from_list(struct file *f, struct file **list_head, long *counter) {
#endif
#ifdef FS_FILE_SANITY_CHECKS
    if (!f)
        ext2_os2_panic(1, "remove_from_list - f is NULL");
    if (f->f_magic != FILE_MAGIC)
        ext2_os2_panic(1, "remove_from_list - invalid magic number");
    if (!list_head)
        ext2_os2_panic(1, "remove_from_list - list_head is NULL");
    if (!(*list_head) || !(*counter))
        ext2_os2_panic(1, "remove_from_list - used list empty");
    if (!(f->f_prev) || !(f->f_next))
        ext2_os2_panic(1, "remove_from_list - used file list corrupted");
    if (f->f_list != list_head)
        ext2_os2_panic(1, "remove_from_list - f in wrong list");
#endif

    f->f_prev->f_next = f->f_next;
    f->f_next->f_prev = f->f_prev;

    if (*list_head == f)
        *list_head = f->f_next;
    if(*list_head == f)
         *list_head = 0;
    f->f_next = 0;
    f->f_prev = 0;
    f->f_list = 0;
    (*counter) --;

}

#ifdef MINIFSD
void (*file_remove_from_list)(struct file *, struct file **, long *) = remove_from_list;
#endif

/*
 * MUST be called with interrupts disabled
 */
INLINE void put_last_in_list(struct file *f, struct file **list_head, long *counter) {
#ifdef FS_FILE_SANITY_CHECKS
    if (!f)
        ext2_os2_panic(1, "put_last_in_list - f is NULL");
    if (f->f_magic != FILE_MAGIC)
        ext2_os2_panic(1, "put_last_in_list - invalid magic number");
    if (f->f_prev || f->f_next)
        ext2_os2_panic(1, "put_last_in_list - used block list corrupted");
    if (!list_head)
        ext2_os2_panic(1, "put_last_in_list - list_head is NULL");
    if (f->f_list)
        ext2_os2_panic(1, "put_last_in_list - f already in a list");
#endif

    if(!(*list_head)) {
        *list_head = f;
        (*list_head)->f_prev = f;
    };

    f->f_next = *list_head;
    f->f_prev = (*list_head)->f_prev;
    (*list_head)->f_prev->f_next = f;
    (*list_head)->f_prev = f;
    f->f_list = list_head;
    (*counter)++;

}

int put_filp(struct file *f) {
    _disable();

    if (f->f_list == &used_files)
        remove_from_list(f, &used_files, &nr_used_files);
    else if (f->f_list == &dead_files)
        remove_from_list(f, &dead_files, &nr_dead_files);
#ifndef MINIFSD
    else if (f->f_list == &page_files)
        remove_from_list(f, &page_files, &nr_page_files);
#endif
    else
        ext2_os2_panic(0, "put_filp - f in invalid list");
    memset(f, 0, sizeof(struct file));
    f->f_magic = FILE_MAGIC;
    put_last_in_list(f, &free_files, &nr_free_files);
    _enable();
    return NO_ERROR;
}

static int grow_files(void) {
    struct file *f;
    long         i;
#ifndef MINIFSD
    int rc;
#endif

    i = 4096  / sizeof(struct file);

#ifndef MINIFSD
    rc = DevHlp32_VMAlloc(4096, VMDHA_NOPHYSADDR, VMDHA_SWAP, __StackToFlat(&f));
    if (rc != NO_ERROR)
        return 0;
#else
    f = (struct file *)G_malloc(4096);
    if (!f)
        return 0;
#endif
    memset(f, 0, 4096);

    nr_files += i;

    if (!free_files) {
        _disable();
        f->f_magic = FILE_MAGIC;
        f->f_next = f;
        f->f_prev = f;
        f->f_list = &free_files;
        free_files = f;
        nr_free_files ++;
        f ++;
        i --;
        _enable();
    }

    for (; i ; i--) {
        _disable();
        f->f_magic = FILE_MAGIC;
        put_last_in_list(f++, &free_files, &nr_free_files);
        _enable();
    }

    return 1;
}

struct file *get_empty_filp(void) {
    struct file *f;

    /*
     * NB: free_files is in a non swappable segment
     */
    do {
        _disable();
        f = free_files;
        if (f) {
            remove_from_list(f, &free_files, &nr_free_files);
            memset(f, 0, sizeof(struct file));
            f->f_magic   = FILE_MAGIC;
            f->f_count   = 1;
            f->f_version = ++event;
            put_last_in_list(f, &used_files, &nr_used_files);
            _enable();
            return f;
        }
        _enable();
    } while (grow_files());

    return NULL;
}


/*
 * The following routines are useless for a mini FSD
 */
#ifndef MINIFSD

unsigned long file_table_init(unsigned long start, unsigned long end) {
    return start;
}

void *VirtToLin(PTR16 virt) {
    int   rc;
    void *lin;

    rc = DevHlp32_VirtToLin(virt, __StackToFlat(&lin));

    if (rc != NO_ERROR) {
        lin = 0;
    }
    return lin;
}

extern void _System remove_from_list_16(PTR16, PTR16, PTR16, PTR16);
extern void *_Pascal VDHQueryLin(PTR16);

void inherit_minifsd_files(struct minifsd_to_fsd_data  *mfs_data) {
    struct file  *f;
    struct file **mfs_free;
    struct file **mfs_used;
    long         *nr_mfs_used;
    long         *nr_mfs_free;
    PTR16         f_16;
    PTR16         mfs_free_16;
    PTR16         mfs_used_16;
    PTR16         nr_mfs_used_16;
    PTR16         nr_mfs_free_16;

    nr_mfs_free    = (long *)VDHQueryLin(mfs_data->nfreehfiles);
    nr_mfs_used    = (long *)VDHQueryLin(mfs_data->nusedhfiles);
    nr_mfs_free_16 = mfs_data->nfreehfiles;
    nr_mfs_used_16 = mfs_data->nusedhfiles;

    nr_files     += *(int *)VDHQueryLin(mfs_data->nhfiles);

    mfs_used      = (struct file **)VDHQueryLin(mfs_data->used_hfiles);
    mfs_free      = (struct file **)VDHQueryLin(mfs_data->free_hfiles);
    mfs_used_16   = mfs_data->used_hfiles;
    mfs_free_16   = mfs_data->free_hfiles;


    while((f = (struct file *)VirtToLin(f_16 = *(PTR16 *)mfs_used)) != 0) {
        remove_from_list_16(mfs_data->file_remove_from_list, f_16, mfs_used_16, nr_mfs_used_16);
        f->f_inode = (struct inode *)VirtToLin(*(PTR16 *)(&(f->f_inode)));
        f->f_op = f->f_inode->i_op->default_file_ops;
        put_last_in_list(f, &used_files, &nr_used_files);
    }
    if (*nr_mfs_used)
        ext2_os2_panic(1, "nr_mfs_used != 0");

    while((f = (struct file *)VirtToLin(f_16 = *(PTR16 *)mfs_free)) != 0) {
        remove_from_list_16(mfs_data->file_remove_from_list, f_16, mfs_free_16, nr_mfs_free_16);
        put_last_in_list(f, &free_files, &nr_free_files);
    }
    if (*nr_mfs_free)
        ext2_os2_panic(1, "nr_mfs_free != 0");

}

/*
 * Called during FS_MOUNT - MOUNT_RELEASE processing. We put used files of the device
 * on a separate list, because we can get FS_CHDIR(CD_FREE) after the MOUNT_RELEASE ...
 */
void invalidate_files(struct super_block *sb, int iput_allowed) {
    struct file *f;
    struct file  token;
    int          found;
    int          pass;

    memset(&token, 0, sizeof(struct file));
    token.f_magic = FILE_MAGIC;
    _disable();
    put_last_in_list(__StackToFlat(&token), &used_files, &nr_used_files);
    _enable();

    found = 1;
    pass  = 0;
    while (found) {
        pass ++;
        found = 0;
        f = used_files;
        while (((f = used_files) != 0) && (f != __StackToFlat(&token))) {
            if (f->f_inode->i_sb == sb) {
                kernel_printf("\tinvalidate_files() - Found file in use on device %04X", sb->s_dev);
                if (iput_allowed)
                    iput(f->f_inode);
                _disable();
                remove_from_list(f, &used_files, &nr_used_files);
        memset(f, 0, sizeof(struct file));
        f->f_magic = FILE_MAGIC;
                put_last_in_list(f, &dead_files, &nr_dead_files);
                _enable();
                found ++;
            } else {
                _disable();
                remove_from_list(f, &used_files, &nr_used_files);
                put_last_in_list(f, &used_files, &nr_used_files);
                _enable();
            }
        }
    }
    _disable();
    remove_from_list(__StackToFlat(&token), &used_files, &nr_used_files);
    _enable();
}


void release_files(struct super_block *sb) {
    struct file *f;
    struct file  token;
    int          found;
    int          pass;

    memset(&token, 0, sizeof(struct file));
    token.f_magic = FILE_MAGIC;
    _disable();
    put_last_in_list(__StackToFlat(&token), &used_files, &nr_used_files);
    _enable();

    found = 1;
    pass  = 0;
    while (found) {
        pass ++;
        found = 0;
        f = used_files;
        while (((f = used_files) != 0) && (f != __StackToFlat(&token))) {
            if (f->f_inode->i_sb == sb) {
                kernel_printf("\trelease_files() - Found file in use on device 0x%04X", sb->s_dev);
        if ((f->f_op) && (f->f_op->release))
            f->f_op->release(f->f_inode, f);
                found ++;
            }
            _disable();
            remove_from_list(f, &used_files, &nr_used_files);
            put_last_in_list(f, &used_files, &nr_used_files);
            _enable();
        }
    }

    _disable();
    remove_from_list(__StackToFlat(&token), &used_files, &nr_used_files);
    put_last_in_list(__StackToFlat(&token), &page_files, &nr_page_files);
    _enable();


    found = 1;
    pass  = 0;
    while (found) {
        pass ++;
        found = 0;
        f = page_files;
        while (((f = page_files) != 0) && (f != __StackToFlat(&token))) {
            if (f->f_inode->i_sb == sb) {
                kernel_printf("\trelease_files() - Found page file in use on device 0x%04X", sb->s_dev);
        if ((f->f_op) && (f->f_op->release))
            f->f_op->release(f->f_inode, f);
                found ++;
            }
            _disable();
            remove_from_list(f, &page_files, &nr_page_files);
            put_last_in_list(f, &page_files, &nr_page_files);
            _enable();
        }
    }
}

/*
 * Puts an open file instance in the swapper list so that they can't be closed
 * by invalidate_all_files during FS_SHUTDOWN processing. This is required because
 * the swapper activity continues beyond FS_SHUTDOWN.
 */
void set_swapper_filp(struct file *f) {
    _disable();
    remove_from_list(f, &used_files, &nr_used_files);
    put_last_in_list(f, &page_files, &nr_page_files);
    _enable();
}


#endif /* MINIFSD not defined */

