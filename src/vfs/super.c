//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/super.c,v 1.1 2001/05/09 17:48:08 umoeller Exp $
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
#include <builtin.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\DevHlp32.h>
#include  <linux/fs.h>
#include <os2\os2proto.h>


long nr_supers           = 0;
long nr_free_supers      = 0;
long nr_used_supers      = 0;
long nr_busy_supers      = 0;

struct super_block *free_supers = 0;    // free (unused) superblocks
struct super_block *used_supers = 0;    // superblocks in use (mounted)
struct super_block *busy_supers = 0;    // busy superblocks found during shutdown (remounted ro)


/*
 * MUST be called with interrupts disabled
 */
INLINE void remove_from_list(struct super_block *sb, struct super_block **list_head, long *counter) {
#ifdef FS_SUPER_SANITY_CHECKS
    if (!sb)
        ext2_os2_panic(1, "remove_from_list - sb is NULL");
    if (sb->s_magic_internal != SUPER_MAGIC)
        ext2_os2_panic(1, "remove_from_list - invalid magic number (sb)");
    if (!list_head)
        ext2_os2_panic(1, "remove_from_list - list_head is NULL (sb)");
    if (!(*list_head) || !(*counter))
        ext2_os2_panic(1, "remove_from_list - used list empty (sb)");
    if (!(sb->s_prev) || !(sb->s_next))
        ext2_os2_panic(1, "remove_from_list - used superblock list corrupted");
    if (sb->s_list != list_head)
        ext2_os2_panic(1, "remove_from_list - sb in wrong list");
#endif

    sb->s_prev->s_next = sb->s_next;
    sb->s_next->s_prev = sb->s_prev;

    if (*list_head == sb)
        *list_head = sb->s_next;
    if(*list_head == sb)
         *list_head = 0;
    sb->s_next = 0;
    sb->s_prev = 0;
    sb->s_list = 0;
    (*counter) --;

}


/*
 * MUST be called with interrupts disabled
 */
INLINE void put_last_in_list(struct super_block *sb, struct super_block **list_head, long *counter) {
#ifdef FS_SUPER_SANITY_CHECKS
    if (!sb)
        ext2_os2_panic(1, "put_last_in_list - sb is NULL");
    if (sb->s_magic_internal != SUPER_MAGIC)
        ext2_os2_panic(1, "put_last_in_list - invalid magic number (sb)");
    if (sb->s_prev || sb->s_next)
        ext2_os2_panic(1, "put_last_in_list - used superblock list corrupted");
    if (!list_head)
        ext2_os2_panic(1, "put_last_in_list - list_head is NULL (sb)");
    if (sb->s_list)
        ext2_os2_panic(1, "put_last_in_list - sb already in a list");
#endif

    if(!(*list_head)) {
        *list_head = sb;
        (*list_head)->s_prev = sb;
    };

    sb->s_next = *list_head;
    sb->s_prev = (*list_head)->s_prev;
    (*list_head)->s_prev->s_next = sb;
    (*list_head)->s_prev = sb;
    sb->s_list = list_head;
    (*counter)++;

}

int put_super(struct super_block *sb) {
    _disable();

    if (sb->s_list == &used_supers)
        remove_from_list(sb, &used_supers, &nr_used_supers);
    else
        ext2_os2_panic(0, "put_super - sb not in used list");
    memset(sb, 0, sizeof(struct super_block));
    sb->s_magic_internal = SUPER_MAGIC;
    put_last_in_list(sb, &free_supers, &nr_free_supers);
    _enable();
    return NO_ERROR;
}

static int grow_supers(void) {
    struct super_block *sb;
    int                 i;
    int                 rc;

    i = 4096  / sizeof(struct super_block);

    rc = DevHlp32_VMAlloc(4096, VMDHA_NOPHYSADDR, VMDHA_SWAP, __StackToFlat(&sb));
    if (rc != NO_ERROR)
        return 0;
    memset(sb, 0, 4096);

    nr_supers += i;

    if (!free_supers) {
        _disable();
        sb->s_magic_internal = SUPER_MAGIC;
        sb->s_next = sb;
        sb->s_prev = sb;
        sb->s_list = &free_supers;
        free_supers = sb;
        nr_free_supers ++;
        sb ++;
        i --;
        _enable();
    }

    for (; i ; i--) {
        _disable();
        sb->s_magic_internal = SUPER_MAGIC;
        put_last_in_list(sb++, &free_supers, &nr_free_supers);
        _enable();
    }

    return 1;
}

struct super_block *get_empty_super(void) {
    struct super_block *sb;

    /*
     * NB: free_files is in a non swappable segment
     */
    do {
        _disable();
        sb = free_supers;
        if (sb) {
            remove_from_list(sb, &free_supers, &nr_free_supers);
            memset(sb, 0, sizeof(struct super_block));
            sb->s_magic_internal   = SUPER_MAGIC;
            put_last_in_list(sb, &used_supers, &nr_used_supers);
            _enable();
            return sb;
        }
        _enable();
    } while (grow_supers());

    return NULL;
}

/*
 * This routine is called ONLY during FS_SHUTDOWN processing. its purpose is to unmount all
 * mounted ext2 file systems.
 * for each superblock in use :
 *     if media is present (s_status = VOL_STATUS_MOUNTED)
 *         if media is busy
 *             remount ro
 *         else
 *             normal unmount
 *     else
 *         "hard" unmount
 */
void invalidate_supers(void) {
    int                 rc;
    struct super_block *sb;

    while((sb = used_supers) != 0) {
        if (sb->s_status == VOL_STATUS_MOUNTED) {
            kernel_printf("\tinvalidate_supers - unmounting 0x%04X (present)", sb->s_dev);
            rc = do_unmount(sb);
            if (rc == ERROR_DEVICE_IN_USE) {
                kernel_printf("\tdevice 0x%04X busy - remounting ro");
        release_files(sb);
        do_remount_sb(sb, MS_RDONLY, 0);
        sync_buffers(sb->s_dev, 1);
               _disable();
               if (sb->s_list == &used_supers)
                   remove_from_list(sb, &used_supers, &nr_used_supers);
               else
                   ext2_os2_panic(0, "put_super - sb not in used list");
               put_last_in_list(sb, &busy_supers, &nr_busy_supers);
               _enable();
            }
        } else {
            kernel_printf("\tinvalidate_supers - unmounting 0x%04X (NOT present)", sb->s_dev);
            put_super(sb);
        }
    }

}


extern void *_Pascal VDHQueryLin(PTR16);
extern void *VirtToLin(PTR16 virt);

extern void _System remove_from_list_16(PTR16, PTR16, PTR16, PTR16);

extern struct file_system_type ext2_fs_type;

void inherit_minifsd_supers(struct minifsd_to_fsd_data  *mfs_data) {
    struct super_block   *sb;
    struct super_block  **mfs_free;
    struct super_block  **mfs_used;
    long         *nr_mfs_used;
    long         *nr_mfs_free;
    PTR16         sb_16;
    PTR16         mfs_free_16;
    PTR16         mfs_used_16;
    PTR16         nr_mfs_used_16;
    PTR16         nr_mfs_free_16;

    nr_mfs_free    = (long *)VDHQueryLin(mfs_data->nfreesupers);
    nr_mfs_used    = (long *)VDHQueryLin(mfs_data->nusedsupers);
    nr_mfs_free_16 = mfs_data->nfreesupers;
    nr_mfs_used_16 = mfs_data->nusedsupers;

    nr_supers    += *(long *)VDHQueryLin(mfs_data->nsupers);

    mfs_used      = (struct super_block **)VDHQueryLin(mfs_data->used_supers);
    mfs_free      = (struct super_block **)VDHQueryLin(mfs_data->free_supers);
    mfs_used_16   = mfs_data->used_supers;
    mfs_free_16   = mfs_data->free_supers;


    while((sb = (struct super_block *)VirtToLin(sb_16 = *(PTR16 *)mfs_used)) != 0) {
        remove_from_list_16(mfs_data->super_remove_from_list, sb_16, mfs_used_16, nr_mfs_used_16);

        if (Read_Write)
            sb->s_flags &= ~MS_RDONLY;

        sb = ext2_fs_type.read_super(sb, 0, 0);
        if (!sb)
             ext2_os2_panic(1, "inherit_minifsd_supers : couldn't remount superblock");


        put_last_in_list(sb, &used_supers, &nr_used_supers);
    }
    if (*nr_mfs_used)
        ext2_os2_panic(1, "nr_mfs_used != 0 : %d %08X", *nr_mfs_used, (unsigned long)*mfs_used);

    while((sb = (struct super_block *)VirtToLin(sb_16 = *(PTR16 *)mfs_free)) != 0) {
        remove_from_list_16(mfs_data->super_remove_from_list, sb_16, mfs_free_16, nr_mfs_free_16);
        put_last_in_list(sb, &free_supers, &nr_free_supers);
    }
    if (*nr_mfs_free)
        ext2_os2_panic(1, "nr_mfs_free != 0");

}
