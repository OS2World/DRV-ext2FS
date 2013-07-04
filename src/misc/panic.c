//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/panic.c,v 1.1 2001/05/09 17:49:51 umoeller Exp $
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

#include <stdarg.h>
#define VA_START(ap, last) ap = ((va_list)__StackToFlat(&last)) + __nextword(last)
#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsh32.h>
#include <os2\devhlp32.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <linux\fs_proto.h>

static char panicmsg[] =
#ifdef MICROFSD
"                >>> EXT2-OS2 MICROFSD FATAL ERROR <<<                      \r\n"
#endif
#ifdef MINIFSD
"                 >>> EXT2-OS2 MINIFSD FATAL ERROR <<<                      \r\n"
#endif
#if !defined(MICROFSD) && !defined(MINIFSD)
"                 >>>>>>> EXT2-OS2 FATAL ERROR <<<<<<<                      \r\n"
#endif
"\r\n"
"%s\r\n"
"\r\n"
"Please write down the error message above and e-mail it to willm@ibm.net "
"along with your partition scheme (FDISK /QUERY) and a description "
"of the problem, then reboot your system and FIRST RUN E2FSCK FROM LINUX "
"ON ALL YOUR EXT2FS PARTITIONS.\r\n";

char scratch_buffer[1024] = {0};

#ifndef MINIFSD
INLINE void system_halt(char *msg, int size) {
    fsh32_interr(msg, size);
}
#else
int (far pascal *system_halt)() = MFSH_INTERR;
#endif

/*
 * This is the panic code : it will force an abrupt system halt. Buffers are flushed
 * according to the sync parameter (1 = flush). This flush is meaningless for mini FSD.
 */
void ext2_os2_panic(int sync, const char *fmt, ...) {
    va_list args;

    VA_START(args, fmt);
    vsprintf(scratch_buffer + 512, fmt, args);
    va_end(args);

    sprintf(scratch_buffer, panicmsg, scratch_buffer + 512);

#ifndef MINIFSD
    if (sync) {
        /*
         * Commits inodes and buffers to disk to limit damages if the buffer and inode management
         * code is OK.
         */
        sync_buffers(0, 1);
        sync_inodes(0);
        sync_buffers(0, 1);
    }
#endif

    /*
     * This call will stop the system and prompt the famous message "The system detected an
     * internal processing error at location ..." along with our error message.
     */
    system_halt(scratch_buffer, strlen(scratch_buffer) + 1);
}

