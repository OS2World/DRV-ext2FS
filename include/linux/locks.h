//
// $Header: e:\\netlabs.cvs\\ext2fs/include/linux/locks.h,v 1.1 2001/05/09 17:43:39 umoeller Exp $
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




#ifndef _LINUX_LOCKS_H
#define _LINUX_LOCKS_H

#ifndef OS2
#error This file is OS/2 specific
#endif

#ifndef __IBMC__
#error This file is IBM Visualage C++ specific
#endif

#include <builtin.h>

/*
 * Buffer cache locking - note that interrupts may only unlock, not
 * lock buffers.
 */
INLINE void wait_on_buffer(struct buffer_head * bh)
{
    _disable();
    while (bh->b_lock) {
        DevHlp32_ProcBlock((unsigned long)bh, -1, 1);
        _disable();
    }
    _enable();
}

INLINE void __wait_on_super(struct super_block * sb)
{
    _disable();
    while (sb->s_lock) {
        DevHlp32_ProcBlock((unsigned long)(&(sb->s_wait)), -1, 1);
        _disable();
    }
    _enable();
}
INLINE void lock_buffer(struct buffer_head * bh)
{
    _disable();
    while (__cxchg((volatile char *)(&(bh->b_lock)), 1)) {
        DevHlp32_ProcBlock((unsigned long)bh, -1, 1);
        _disable();
    }
    _enable();
}


INLINE void unlock_buffer(struct buffer_head * bh)
{
	bh->b_lock = 0;
	DevHlp32_ProcRun((unsigned long)bh);
}


/*
 * super-block locking. Again, interrupts may only unlock
 * a super-block (although even this isn't done right now.
 * nfs may need it).
 */
// extern void __wait_on_super(struct super_block *);

INLINE void wait_on_super(struct super_block * sb)
{
	if (sb->s_lock)
		__wait_on_super(sb);
}

INLINE void lock_super(struct super_block * sb)
{
	if (sb->s_lock)
		__wait_on_super(sb);
	sb->s_lock = 1;
}

INLINE void unlock_super(struct super_block * sb)
{
	sb->s_lock = 0;
	wake_up(&sb->s_wait);
}

#endif /* _LINUX_LOCKS_H */

