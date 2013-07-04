//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/errors.h,v 1.1 2001/05/09 17:43:40 umoeller Exp $
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


// This file is for the moment completely OUT OF DATE


#ifndef __errors_h
#define __errors_h

#ifdef IFSDBG
char *func[] = {
                 "waitandlock_file",        /* 0  vinode.c   */
                 "FSH_SEMREQUEST",          /* 1  FSHELPER   */
                 "get_vinode",              /* 2  vinode.c   */
                 "alloc_vinode",            /* 3  bufmgmt.c  */
                 "FSH_SEMSET",              /* 4  FSHELPER   */
                 "unlock_file",             /* 5  vinode.c   */
                 "FSH_SEMCLEAR",            /* 6  FSHELPER   */
                 "syncfilebuf",             /* 7  files.c    */
                 "getblk",                  /* 8  indirect.c */
                 "read_blk",                /* 9  cache.c    */
                 "write_blk",               /* 10 cache.c    */
                 "reopen",                  /* 11 files.c    */
                 "rewind",                  /* 12 files.c    */
                 "open_by_inode",           /* 13 files.c    */
                 "alloc_hfile",             /* 14 bufmgmt.c  */
                 "readinode",               /* 15 inode.c    */
                 "alloc_filebuf",           /* 16 bufmgmt.c  */
                 "close",                   /* 17 files.c    */
                 "writeinode",              /* 18 inode.c    */
                 "free_vinode",             /* 19 vinode.c   */
                 "free_filebuf",            /* 20 bufmgmt.c  */
                 "free_hfile",              /* 21 bufmgmt.c  */
                 "read",                    /* 22 files.c    */
                 "write",                   /* 23 files.c    */
                 "setallocatedsize",        /* 24 indirect.c */
                 "seek",                    /* 25 files.c    */
                 "flush",                   /* 26 files.c    */
                 "L_malloc",                /* 27 fs_mem.c   */
                 "L_free",                  /* 28 fs_mem.c   */
                 "open_by_name",            /* 29 files.c    */
                 "FS_CHDIR",                /* 30 fs_dir.c   */
                 "FS_READ",                 /* 31 test.c     */
                 "FSH_PROBEBUF",            /* 32 FSHELPER   */
                 "FS_FINDFIRST",            /* 33 test.c     */
                 "FS_OPENCREATE",           /* 34 fs_misc.c  */
                 "FS_CLOSE",                /* 35 test.c     */
                 "FS_FINDCLOSE",            /* 36 test.c     */
                 "FS_FINDNEXT",             /* 37 test.c     */
                 "FS_WRITE",                /* 38 test.c     */
                 "FS_CHGFILEPTR",           /* 39 test.c     */
                 "FS_FILEATTRIBUTE",        /* 40 test.c     */
                 "FS_NEWSIZE",              /* 41 test.c     */
                 "createdirbloc",           /* 42 dirent.c   */
                 "FS_MKDIR",                /* 43 fs_dir.c   */
                 "mkdir",                   /* 44 dirent.c   */
                 "FS_RMDIR",                /* 45 fs_dir.c   */
                 "rmdir",                   /* 46 dirent.c   */
                 "FS_FINDFROMNAME",         /* 47 fs_find.c  */
                 "FS_PATHINFO",             /* 48 fs_find.c  */
                 "FS_FILEINFO",             /* 49 fs_find.c  */
                 "bread()",                 /* 50 bufmgmt.c  */
                 "brelse()",                /* 51 bufmgmt.c  */
                 "G_malloc",                /* 52 fs_mem.c   */
                 "G_free",                  /* 53 fs_mem.c   */
                 "FS_DELETE",               /* 54 fs_misc.c  */
                 ""
                };

char *sourcefile[] = {
                       "vfs/inode.c",       /* 0 */
                       "ext2/inode.c",      /* 1 */
                       "vfs/buffers.c",     /* 2 */
                       "files.c",           /* 3 */
                       "indirect.c",        /* 4 */
                       "cache.c",           /* 5 */
                       "fs_mem.c",          /* 6 */
                       "fs_misc.c",         /* 7 */
                       "dirent.c",          /* 8 */
                       "fs_dir.c",          /* 9 */
                       "fs_find.c",         /* 10 */
                       ""
                      };
#endif

#define FILE_VINODE_C         0            /* vfs/inode.c   */
#define FILE_INODE_C          1           /* ext2/inode.c  */
#define FILE_BUFMGMT_C        2            /* vfs/bufmgmt.c */
#define FILE_FILES_C          3            /* files.c       */
#define FILE_INDIRECT_C       4            /* indirect.c    */
#define FILE_CACHE_C          5            /* cache.c       */
#define FILE_FS_MEM_C         6            /* fs_mem.c      */
#define FILE_TEST_C           7            /* fs_misc.c     */
#define FILE_DIRENT_C         8            /* dirent.c      */
#define FILE_FS_DIR_C         9            /* fs_dir.c      */
#define FILE_FS_FIND_C        10           /* fs_find.c     */

#define FUNC_WAITANDLOCK_FILE 0            /* waitadnlock_file() - vinode.c   */
#define FUNC_FSH_SEMREQUEST   1            /* FSH_SEMREQUEST()   - FSHELPER   */
#define FUNC_GET_VINODE       2            /* get_vinode()       - vinode.c   */
#define FUNC_ALLOC_VINODE     3            /* alloc_vinode()     - bufmgmt.c  */
#define FUNC_FSH_SEMSET       4            /* FSH_SEMSET()       - FSHELPER   */
#define FUNC_UNLOCK_FILE      5            /* unlock_file()      - vinode.c   */
#define FUNC_FSH_SEMCLEAR     6            /* FSH_SEMCLEAR()     - FSHELPER   */
#define FUNC_SYNCFILEBUF      7            /* syncfilebuf()      - files.c    */
#define FUNC_GETBLK           8            /* getblk()           - indirect.c */
#define FUNC_READ_BLK         9            /* read_blk()         - cache.c    */
#define FUNC_WRITE_BLK        10           /* write_blk()        - cache.c    */
#define FUNC_REOPEN           11           /* reopen()           - files.c    */
#define FUNC_REWIND           12           /* rewind()           - files.c    */
#define FUNC_OPEN_BY_INODE    13           /* open_by_inode()    - files.c    */
#define FUNC_ALLOC_HFILE      14           /* alloc_hfile()      - bufmgmt.c  */
#define FUNC_READINODE        15           /* readinode()        - inode.c    */
#define FUNC_ALLOC_FILEBUF    16           /* alloc_filebuf()    - bufmgmt.c  */
#define FUNC_CLOSE            17           /* close()            - files.c    */
#define FUNC_WRITEINODE       18           /* writeinode()       - inode.c    */
#define FUNC_FREE_VINODE      19           /* free_vinode()      - vinode.c   */
#define FUNC_FREE_FILEBUF     20           /* free_filebuf()     - bufmgmt.c  */
#define FUNC_FREE_HFILE       21           /* free_hfile()       - bufmgmt.c  */
#define FUNC_READ             22           /* read()             - files.c    */
#define FUNC_WRITE            23           /* write()            - files.c    */
#define FUNC_SETALLOCATEDSIZE 24           /* setallocatedsize() - indirect.c */
#define FUNC_SEEK             25           /* seek()             - files.c    */
#define FUNC_FLUSH            26           /* flush()            - files.c    */
#define FUNC_L_MALLOC         27           /* L_malloc()         - fs_mem.c   */
#define FUNC_L_FREE           28           /* L_free()           - fs_mem.c   */
#define FUNC_OPEN_BY_NAME     29           /* open_by_name()     - files.c    */
#define FUNC_FS_CHDIR         30           /* FS_CHDIR()         - fs_dir.c   */
#define FUNC_FS_READ          31           /* FS_READ()          - test.c     */
#define FUNC_FSH_PROBEBUF     32           /* FSH_PROBEBUF()     - FSHELPER   */
#define FUNC_FS_FINDFIRST     33           /* FS_FINDFIRST()     - test.c     */
#define FUNC_FS_OPENCREATE    34           /* FS_OPENCREATE()    - fs_misc.c  */
#define FUNC_FS_CLOSE         35           /* FS_CLOSE()         - test.c     */
#define FUNC_FS_FINDCLOSE     36           /* FS_FINDCLOSE()     - test.c     */
#define FUNC_FS_FINDNEXT      37           /* FS_FINDNEXT()      - test.c     */
#define FUNC_FS_WRITE         38           /* FS_WRITE()         - test.c     */
#define FUNC_FS_CHGFILEPTR    39           /* FS_CHGFILEPTR()    - test.c     */
#define FUNC_FS_FILEATTRIBUTE 40           /* FS_FILEATTRIBUTE() - test.c     */
#define FUNC_FS_NEWSIZE       41           /* FS_NEWSIZE()       - test.c     */
#define FUNC_CREATEDIRBLOC    42           /* createdirbloc()    - dirent.c   */
#define FUNC_FS_MKDIR         43           /* FS_MKDIR()         - fs_dir.c   */
#define FUNC_MKDIR            44           /* mkdir()            - dirent.c   */
#define FUNC_FS_RMDIR         45           /* FS_RMDIR()         - fs_dir.c   */
#define FUNC_RMDIR            46           /* rmdir()            - dirent.c   */
#define FUNC_FS_FINDFROMNAME  47           /* FS_FINDFROMNAME()  - fs_find.c  */
#define FUNC_FS_PATHINFO      48           /* FS_PATHINFO()      - fs_find.c  */
#define FUNC_FS_FILEINFO      49           /* FS_FILEINFO()      - fs_find.c  */
#define FUNC_BREAD            50           /* bread()            - bufmgmt.c  */
#define FUNC_BRELSE           51           /* brelse()           - bufmgmt.c  */
#define FUNC_G_malloc         52           /* G_malloc()         - fs_mem.c   */
#define FUNC_G_free           53           /* G_free()           - fs_mem.c   */
#define FUNC_FS_DELETE        54           /* FS_DELETE()        - fs_misc.c  */

#endif
