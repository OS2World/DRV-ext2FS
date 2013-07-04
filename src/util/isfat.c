//
// $Header: e:\\netlabs.cvs\\ext2fs/src/util/isfat.c,v 1.1 2001/05/09 17:49:52 umoeller Exp $
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


#include <string.h>
#include <os2\types.h>
#ifndef MINIFSD
#include <os2\devhlp32.h>
#endif
#include <os2\os2proto.h>

#ifdef TEST_ISFAT
#include <stdio.h>
#endif

//
// Forbidden FAT characters (excluding '.')
// (From the OS/2 Command reference)
//
static char forbidden_chars[] = {
                                 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                                 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,
                                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
                                 '\\', '/', ':', '*', '?', '"', '<', '>', '|', ',',
                                 '+', '=', '[', ']', ';',
                                 '\0'
                                };

//
// Forbidden FAT file names
// (From the OS/2 Command reference)
//
static pchar forbidden_names[] = {
                                  "KBD$",
                                  "PRN",
                                  "NUL",
                                  "COM1",
                                  "COM2",
                                  "COM3",
                                  "COM4",
                                  "CLOCK$",
                                  "LPT1",
                                  "LPT2",
                                  "LPT3",
                                  "CON",
                                  "SCREEN$",
                                  "POINTER$",
                                  "MOUSE$"
                                 };
int nb_forbiden_names = 15;

//
// Tests if a file name (WITHOUT PATH) is FAT compliant
// 1 - FAT compliant
// 0 - non FAT compliant
//
// ... could be better but seems to work !
//
int isfat(pchar component) {
    int len = strlen(component);
    int i, j, pos;


    if (strcmp(component, ".") == 0) {          // "."
        return 1;
    }

    if (strcmp(component, "..") == 0) {         // ".."
        return 1;
    }

    if ((len > 12) || (len == 0)) {
        return 0;                   // lenght > 8.3 or null
    }

    for (i = 0;i < nb_forbiden_names  ; i++) {
       if (strcmp(component, forbidden_names[i]) == 0) {
           return 0;
       } /* endif */
    } /* endfor */

    if (strpbrk(component, forbidden_chars) > 0) {
        return 0;                   // Forbidden characters present
    }

    j = 0;
    pos = 0;
    for (i = 0; i < len; i++) {
        if (component[i] == '.') {
            j++;
        } else{
           if (j == 0) {
               pos++;
           } /* endif */
        }
    }

    if (j > 1) {                    // More than one '.'
        return 0;
    }

    if (j == 0) {                   // No '.'
        if (len > 8) {
            return 0;
        } else {
            return 1;
        }
    }

    if (j == 1) {
        if ((len - pos > 4) || (pos == 0)) {            // '.' in front or extension > 3
            return 0;
        } else {
            if (pos > 8) {
                return 0;                               // name > 8
            } else {
               return 1;
            } /* endif */
        }
    }

    return 0;
}


#ifdef TEST_ISFAT
int main(int argc, char **argv) {

    printf("%d\n", isfat(argv[1]));
}
#endif
