//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/request_list.c,v 1.1 2001/05/09 17:48:08 umoeller Exp $
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

#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <builtin.h>
#include <string.h>


#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsh32.h>
#include <os2\devhlp32.h>
#include <os2\fsd32.h>
#include <os2\magic.h>
#include <os2\os2misc.h>
#include <os2\request_list.h>
#include <os2\os2proto.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <os2\log.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\sched.h>
#include <linux\locks.h>


static long nr_reqlist      = 0;
static long nr_free_reqlist = 0;
static long nr_used_reqlist = 0;

static struct reqlist *free_reqlists = 0;
static struct reqlist *used_reqlists = 0;

#define REQLIST_WAIT (&free_reqlists)

/*
 * MUST be called with interrupts disabled
 */
INLINE void remove_from_list(struct reqlist *s, struct reqlist **list_head, long *counter) {
#ifdef FS_REQLIST_SANITY_CHECKS
    if (!s)
        ext2_os2_panic(0, "reqlist remove_from_list - s is NULL");
    if (s->s.s_magic != REQLIST_MAGIC)
        ext2_os2_panic(0, "reqlist remove_from_list - invalid magic number");
    if (!list_head)
        ext2_os2_panic(0, "reqlist remove_from_list - list_head is NULL");
    if (!(*list_head) || !(*counter))
        ext2_os2_panic(0, "reqlist remove_from_list - list empty");
    if (!(s->s.s_prev) || !(s->s.s_next))
        ext2_os2_panic(0, "reqlist remove_from_list - list corrupted");
    if (s->s.s_list != list_head)
        ext2_os2_panic(0, "reqlist remove_from_list - wrong list");
#endif

    s->s.s_prev->s.s_next = s->s.s_next;
    s->s.s_next->s.s_prev = s->s.s_prev;

    if (*list_head == s)
        *list_head = s->s.s_next;
    if(*list_head == s)
         *list_head = 0;
    s->s.s_next = 0;
    s->s.s_prev = 0;
    s->s.s_list = 0;
    (*counter) --;

}


/*
 * MUST be called with interrupts disabled
 */
INLINE void put_last_in_list(struct reqlist *s, struct reqlist **list_head, long *counter) {
#ifdef FS_REQLIST_SANITY_CHECKS
    if (!s)
        ext2_os2_panic(0, "reqlist put_last_in_list - s is NULL");
    if (s->s.s_magic != REQLIST_MAGIC)
        ext2_os2_panic(0, "reqlist put_last_in_list - invalid magic number");
    if (s->s.s_prev || s->s.s_next)
        ext2_os2_panic(0, "reqlist put_last_in_list - list corrupted");
    if (!list_head)
        ext2_os2_panic(0, "reqlist put_last_in_list - list_head is NULL");
    if (s->s.s_list)
        ext2_os2_panic(0, "reqlist put_last_in_list - already in a list");
#endif

    if(!(*list_head)) {
        *list_head = s;
        (*list_head)->s.s_prev = s;
    };

    s->s.s_next = *list_head;
    s->s.s_prev = (*list_head)->s.s_prev;
    (*list_head)->s.s_prev->s.s_next = s;
    (*list_head)->s.s_prev = s;
    s->s.s_list = list_head;
    (*counter)++;

}

//#pragma alloc_text(EXT2_FIXED_CODE, put_reqlist)
void put_reqlist(struct reqlist *s) {
    _disable();
    remove_from_list(s, &used_reqlists, &nr_used_reqlist);
    put_last_in_list(s, &free_reqlists, &nr_free_reqlist);
    _enable();
    wake_up(REQLIST_WAIT);
}

struct reqlist *get_reqlist(void) {
    struct reqlist *s;

    /*
     * NB: free_reqlist is in a non swappable segment
     */
    do {
        _disable();
        s = free_reqlists;
        if (s) {
            remove_from_list(s, &free_reqlists, &nr_free_reqlist);
            put_last_in_list(s, &used_reqlists, &nr_used_reqlist);
            _enable();
            return s;
        }
        _enable();
        printk("get_reqlist : no more request lists ... retrying");
        sleep_on(REQLIST_WAIT);
    } while (1);

    return 0;   // keeps compiler happy
}

#define NR_REQLIST_PER_SEG (61440L / sizeof(struct reqlist))
#define SZ_REQLIST_SEG (NR_REQLIST_PER_SEG * sizeof(struct reqlist))

/*
 *@@ reqlist_init:
 *      originally called from fs32_init().
 */

int reqlist_init(int nr_seg)
{
    int             i;
    int             j;
    struct reqlist *s;
    PTR16           virt;
    int             rc;

    if (nr_reqlist)
        return NO_ERROR;

    for (j = 0; j < nr_seg; j++) {
        rc = fsh32_segalloc(SA_FRING0,
                            SZ_REQLIST_SEG,
                            __StackToFlat(&(virt.seg)));
        if (rc != NO_ERROR)
            continue;
        virt.ofs = 0;
        rc = DevHlp32_VirtToLin(virt, __StackToFlat(&s));
        virt.ofs = sizeof(struct reqlist_hdr);                          // virt.seg:virt.ofs points to s->buf
        if (rc != NO_ERROR)
            ext2_os2_panic(0, "reqlist init - VirtToLin failed");

        i           = NR_REQLIST_PER_SEG;
        nr_reqlist += NR_REQLIST_PER_SEG;

        if (!free_reqlists)
        {
            _disable();
            memset(s, 0, sizeof(struct reqlist));
            s->s.s_magic  = REQLIST_MAGIC;
            s->s.s_next   = s;
            s->s.s_prev   = s;
            s->s.s_list   = &free_reqlists;
            s->s.s_self_virt.seg = virt.seg;
            s->s.s_self_virt.ofs = virt.ofs;
            s->s.s_self          = &(s->s_rwlist);
            free_reqlists = s;
            nr_free_reqlist ++;
            s ++;
            i --;
            virt.ofs += sizeof(struct reqlist);
            _enable();
        }

        for (; i ; i --)
        {
            _disable();
            memset(s, 0, sizeof(struct reqlist));
            s->s.s_magic = REQLIST_MAGIC;
            s->s.s_self_virt.seg = virt.seg;
            s->s.s_self_virt.ofs = virt.ofs;
            s->s.s_self          = &(s->s_rwlist);
            virt.ofs += sizeof(struct reqlist);
            put_last_in_list(s++, &free_reqlists, &nr_free_reqlist);
            _enable();
        }
    }
    return (nr_reqlist ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY);
}
