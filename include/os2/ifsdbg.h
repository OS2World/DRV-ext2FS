//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/ifsdbg.h,v 1.1 2001/05/09 17:43:41 umoeller Exp $
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


#ifndef __ifsdbg_h
#define __ifsdbg_h

#include <os2\types.h>

/*****************************************************/
/*** Fonctions support‚es par FS_FSCTL pour le log ***/
/*****************************************************/
#define IFSDBG_OPEN       16
#define IFSDBG_CLOSE      17
#define IFSDBG_READ       18
// #define IFSDBG_FLUSHCACHE 19
// #define IFSDBG_LAZY_WRITE 20
#define IFSDBG_SETUID     21
/*****************************************************/

#pragma pack(1)

#define LOG_FS_EXIT  0xFFFF
#define LOG_L_malloc 0xFFFE
#define LOG_L_free   0xFFFD
#define LOG_GET_ID   0xFFFC
#define LOG_FS_ERR   0xFFFB

typedef struct {
    UINT16 Type;
    UINT16 uid;
    UINT16 pid;
    UINT16 pdb;
} log_fs_exit;

typedef struct {
    UINT16 Type;
    UINT32 infunction;  /* function where error occured  */
    UINT32 errfunction; /* function returning error code */
    INT16    retcode;     /* return code from errfunction  */
    UINT32 sourcefile;  /* file where error occured      */
    UINT32 sourceline;  /* line number in sourcefile     */
} err_record;

#pragma pack()

#endif /* __ifsdbg_h */
