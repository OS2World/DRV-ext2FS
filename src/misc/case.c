//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/case.c,v 1.1 2001/05/09 17:49:50 umoeller Exp $
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
#include <os2\StackToFlat.h>
#include <os2\fsh32.h>
#include <os2\os2proto.h>
#include <os2\os2misc.h>

extern char Case_Retensive;

int is_case_retensive(void) {
    int    retval;
    union  fsh32_qsysinfo_parms p;

    retval = 0;
    if (Case_Retensive) {
        /*
         * Case retensivity set on the IFS command line
         */
        retval = 1;
    } else {
        /*
         * Case retensivity not set on the IFS command line, then is it a DOS box request ?
         * (DOS box are always case retensive)
         */
        if (fsh32_qsysinfo(2, __StackToFlat(&p)) == NO_ERROR) {
            if (p.process_info.pdb) {
                retval = 1;
            }
        } else {
            ext2_os2_panic(1, "FSH_QSYSINFO failed");
        }
    }

    return retval;
}
