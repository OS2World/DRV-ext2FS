//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/volume.c,v 1.1 2001/05/09 17:49:51 umoeller Exp $
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
#include <os2\fsh32.h>
#include <os2\devhlp32.h>
#include <os2\StackToFlat.h>
#include <os2\os2proto.h>

#include <os2\log.h>         /* Prototypes des fonctions de log.c                      */
#include <os2\volume.h>      /* Prototypes des fonctions de volume.c                   */

extern struct super_block * FSH32ENTRY __getvolume(PTR16 pvpfsd);

struct super_block * getvolume(unsigned short hVPB)
{
    struct super_block *sb;
    int                 rc;
    PTR16               pvpfsi;
    PTR16               pvpfsd;

    sb = 0;
    if (hVPB) {
        if ((rc = fsh32_getvolparm(hVPB, __StackToFlat(&pvpfsi), __StackToFlat(&pvpfsd))) == NO_ERROR) {
            sb = __getvolume(pvpfsd);
        } else {
            kernel_printf("getvolume - rc = %d", rc);
        }
    } else {
        /*
         * This should NEVER occur
         */
        kernel_printf("getvolume called with hVPB = 0");
    }
    return sb;
}


