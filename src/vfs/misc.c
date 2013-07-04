//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/misc.c,v 1.1 2001/05/09 17:48:07 umoeller Exp $
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


#include <os2\types.h>
#ifndef MINIFSD
#include <os2\devhlp32.h>
#endif
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\sched.h>
#include <linux\resource.h>
#include <linux\fs.h>

//
//  Linux block device read_ahead capabilities
//
long read_ahead[1] = {8};

struct task_struct current[1] = {
                                 { // current[0]
                                  { // current[0].rlim
                                    {0, 0},                            // current[0].rlim[0]
                                    {(long)(~0UL>>1UL), (long)(~0UL>>1UL)},
                                    {0, 0},
                                    {0, 0},
                                    {0, 0},
                                    {0, 0},
                                    {0, 0},
                                    {0, 0}
                                   }
                                  } // end current[0]
                                 };


