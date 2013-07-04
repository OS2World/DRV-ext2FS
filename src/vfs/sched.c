//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/sched.c,v 1.1 2001/05/09 17:48:08 umoeller Exp $
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
#include <os2.h>

#include <os2\types.h>
#include <os2\DevHlp32.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\wait.h>

#ifdef __IBMC__
#include <builtin.h>
#endif

/* void sleep_on(void *wait) {
    _disable();
    DevHlp32_ProcBlock((unsigned long)wait, -1, 1);
}

void __down(struct semaphore * sem)
{
    _disable();
    while (sem->count <= 0) {
        DevHlp32_ProcBlock((unsigned long)(&(sem->wait)), -1, 1);
        _disable();
    }
    _enable();
} */
