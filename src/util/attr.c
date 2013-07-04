//
// $Header: e:\\netlabs.cvs\\ext2fs/src/util/attr.c,v 1.1 2001/05/09 17:49:51 umoeller Exp $
//

// 32 bits OS/2 device driver and IFS support. Provides 32 bits kernel
// services (DevHelp) and utility functions to 32 bits OS/2 ring 0 code
// (device drivers and installable file system drivers).
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
#include <os2.h>        // From the "Developer Connection Device Driver Kit" version 2.0

#include <os2\types.h>
#include <os2\os2proto.h>
#include <linux\fs.h>
#include <linux\stat.h>

#define FILE_NONFAT     0x0040      // File is non 8.3 compliant

#ifndef MINIFSD
//
// Maps DOS SHRA attributes to Linux rwxrwxrwx file modes
//
// Handles only DOS SHRA attributes
//  A - ignored
//  S - ignored
//  H - ignored
//  R - S_IWUSR, S_IWGRP and S_IWOTH cleared
//     !R - S_IWUSR set
void DOS_To_Linux_Attrs(struct inode *inode, unsigned short DOS_attrs) {
    if (DOS_attrs & FILE_READONLY) {
        /*
         * Set file to read only if not already
         */
        if (((inode->i_mode & S_IWUSR)) ||
            ((inode->i_mode & S_IWGRP)) ||
            ((inode->i_mode & S_IWOTH))) {
            inode->i_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        inode->i_dirt  = 1;
        }
    } else {
        /*
         * Set file to read write if not already
         */
        if (!(((inode->i_mode & S_IWUSR)) ||
              ((inode->i_mode & S_IWGRP)) ||
              ((inode->i_mode & S_IWOTH)))) {
            inode->i_mode |= S_IWUSR;
        inode->i_dirt  = 1;
        }
    }
}
#endif /* #ifndef MINIFSD */

//
// Maps Linux file modes to DOS DSHRA attributes
//
unsigned short Linux_To_DOS_Attrs(struct inode *inode, char *component) {
    unsigned short mapped;

    mapped = FILE_NORMAL;

    if ((S_ISLNK(inode->i_mode))  ||
        (S_ISBLK(inode->i_mode))  ||
        (S_ISCHR(inode->i_mode))  ||
        (S_ISFIFO(inode->i_mode)) ||
        (S_ISSOCK(inode->i_mode))) {
        mapped |= FILE_SYSTEM;     /*** UNIXish special files are seen as SYSTEM files ***/
    } /* endif */

    if (S_ISDIR(inode->i_mode)) {
        mapped |= FILE_DIRECTORY;
    } /* endif */

    if ((!(inode->i_mode & S_IWUSR)) &&
        (!(inode->i_mode & S_IWGRP)) &&
        (!(inode->i_mode & S_IWOTH))) {
        mapped |= FILE_READONLY;
    }

    if (!isfat(component)) {
        mapped |= FILE_NONFAT;      // Non 8.3 file name
    }

    return mapped;
}