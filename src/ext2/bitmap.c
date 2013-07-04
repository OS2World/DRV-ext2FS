//
// $Header: e:\\netlabs.cvs\\ext2fs/src/ext2/bitmap.c,v 1.1 2001/05/09 17:53:01 umoeller Exp $
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
 *  linux/fs/ext2/bitmap.c
 *
 *  Copyright (C) 1992, 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                                  Laboratoire MASI - Institut Blaise Pascal
 *                                  Universite Pierre et Marie Curie (Paris VI)
 */
#ifdef __IBMC__
#pragma strings(readonly)
#endif

#ifdef OS2
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>        // From the "Developer Connection Device Driver Kit" version 2.0

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\os2proto.h>
#endif

#include <linux\fs.h>
#include <linux\ext2_fs.h>

static int nibblemap[] = {4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0};

unsigned long ext2_count_free (struct buffer_head * map, unsigned int numchars)
{
    unsigned long i;
    unsigned long sum = 0;

    if (!map)
        return (0);
    for (i = 0; i < numchars; i++)
        sum += nibblemap[map->b_data[i] & 0xf] +
            nibblemap[(map->b_data[i] >> 4) & 0xf];
    return (sum);
}
