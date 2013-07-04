
/*
 *@@sourcefile fs32_init.c:
 *      32-bit C handler called from 16-bit entry point
 *      in src\interface.
 *
 *      FS_INIT gets called during CONFIG.SYS processing.
 *      As opposed to all other fsd exports, this is
 *      running on ring 3!
 *
 *      See src\interface\fs_init.asm for detailed remarks.
 */

/*
 *      Copyright (C) 1995, 1996, 1997  Matthieu WILLM (willm@ibm.net)
 *      Copyright (C) 2001 Ulrich M”ller.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file with this distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#ifdef __IBMC__
#pragma strings(readonly)
#endif

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\DevHlp32.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <os2\fsd32.h>
#include <os2\fsh32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>
#include <os2\bounds.h>

/* ******************************************************************
 *
 *   Global variables (DGROUP)
 *
 ********************************************************************/

int drive_options[26] = {0,};

extern unsigned long event;     // To be moved somewhere in a .h file

int Read_Write = 0;             // 0 read only - 1 read write

int auto_fsck = 1;              // 1 force e2fsck - 0 do not force e2fsck

char Use_Extended_Interface = 1;    // 1 use strategy 2 I/O if available - 0 use FSH_DOVOLIO I/O

char Case_Retensive = 0;        // 1 case retensive (like HPFS) - 0 case sensitive

char Errors_Panic = 0;
char write_through_support = 0;
char fs_strict_checking = 0;

// FLAT global and local infoseg pointers;
// these are now set in fs_init.asm V0.9.12 (2001-05-06) [umoeller]
struct InfoSegGDT *G_pSysInfoSeg = 0;     // OS/2 global infoseg
struct InfoSegLDT *G_pLocInfoSeg = 0;     // OS/2 local  infoseg

// time zone so that time stamps shown in OS/2 are the same as in Linux
long timezone = 0;              // Time zone (second shift from UTC)

// struct DevHelp32 DevHelp32 = {DEVHELP32_MAGIC, DEVHELP32_VERSION,};
        // This used to get filled by mwdd32.sys with the device helpers.
        // The way this used to work was that fsh32\fs_init.asm called
        // mwdd32.sys via IDC and gave it the address of this structure.
        // We now link statically instead. V0.9.12 (2001-04-29) [umoeller]

char *G_pNext;          // temporary pointer in fs32_init()

int G_fRing0Initialized = 0;       // added V0.9.12 (2001-05-06) [umoeller]

/* ******************************************************************
 *
 *   Forward declarations to strings in DATA32
 *
 ********************************************************************/

extern unsigned char DS32_SEPS[];
extern unsigned char DS32_CACHE[];
extern unsigned char DS32_Q[];
extern unsigned char DS32_NO_STRAT2[];
extern unsigned char DS32_CASE_RETENSIVE[];
extern unsigned char DS32_RW[];
extern unsigned char DS32_NO_AUTO_FSCK[];
extern unsigned char DS32_WRITE_THROUGH[];
extern unsigned char DS32_ERRORS[];

/* ******************************************************************
 *
 *   Ring-3 initialization (CODE32 segment)
 *
 ********************************************************************/

/*
 *  strtoul2:
 *      replacement strtoul, because the runtime version
 *      does not work on the 16-bit stack we have here.
 *
 *      Stolen from JFS sources, jfs_init_os2.c.
 */

unsigned int _Optlink strtoul2(char *string,
                               char **next,     // out: next char;
                                                // DO NOT USE STACK VARIABLE HERE
                               int  base)
{
    unsigned int    result = 0;

    if (base == 0)
    {
        base = 10;
        if (*string == '0')
        {
            if (*(++string) == 'x' || *string == 'X')
            {
                string++;
                base = 16;
            }
            else
                base = 8;
        }
    }
    while (*string)
    {
        if (base == 10)
        {
            if (*string < '0' || *string > '9')
                break;
            result = result*10 + (*string - '0');
        }
        else if (base == 8)
        {
            if (*string < '0' || *string > '7')
                break;
            result = result*8 + (*string - '0');
        }
        else if (base == 16)
        {
            if (*string >= '0' && *string <= '9')
                result = result*16 + (*string - '0');
            else if (*string >= 'a' && *string <= 'f')
                result = result*16 + (*string - 'a' + 10);
            else if (*string >= 'A' && *string <= 'F')
                result = result*16 + (*string - 'A' + 10);
            else
                break;
        }
        else    /* invalid base */
            return 0;
        string++;
    }

    if (next)
        *next = string;

    return result;
}

/*
 * strlen2:
 *      replacement strlen, because the runtime version
 *      does not work on the 16-bit stack we have here.
 */

int _Optlink strlen2(char *s)
{
    int L;

    for (L = 0;
         *s++;
         L++)
        ;

    return L;
}

/*
 *@@ fs32_init:
 *      32-bit implementation for initializing the FSD.
 *
 *      Old implementation:
 *
 *      1)  FS_INIT ends up in fsh32\fs_init.asm (16-bit).
 *
 *      2)  fsh32\fs_init.asm opens mwdd32.sys, calls ioctl
 *          MWDD32_IOCTL_FUNCTION_INIT_FSD, which in turn
 *          fills DevHelp32 with the addresses of the
 *          DevHelp32 funcs and calls this function (fs32_init()).
 *          Then mwdd32.sys is closed again.
 *
 *      New implementation:
 *
 *      Since we now link statically, this func gets called
 *      directly from fsh32\fs_init.asm. This is now only
 *      responsible for parsing the IFS command line from
 *      CONFIG.SYS.
 *
 *      NOTES:
 *
 *      --  This is running at ring 3. Great care must
 *          be taken when using the DevHelp32_* calls, since
 *          many of them assume to be at ring 0 and hack the
 *          segment registers.
 *
 *      --  Even though we're at ring 3, SS is NOT equal to
 *          DS here.
 *
 *      --  Even worse, we still have the 16-bit stack here,
 *          so be very careful with stack variables.
 *
 *      --  __StackToFlat() DOES NOT WORK in this context.
 *
 *      --  90% of the C runtime funcs cannot handle our
 *          16-bit stack (such as memset and strlen) and
 *          produce a page fault.
 *
 *      --  We CANNOT access string constants in CONST32_RO,
 *          which produces page faults, from my testing.
 */

int FS32ENTRY fs32_init(PSZ pszCmdLine)
{
    char *szParm;
    int rc;
    char *tz;
    char *pTmp1;
    int i;
    int Quiet = 0;
    unsigned long PgCount;

    /*
     *  command line parsing
     *
     */

    if (pszCmdLine)        // pszCmdLine is NULL if we have no params
    {
        PSZ p = pszCmdLine;
        strupr(pszCmdLine);     // this runtime func works

        while (*p != 0)
        {
            ULONG ulRemaining = 0;

            // look for - or /
            while (    *p
                    && *p != '-'
                    && *p != '/'
                  )
                ++p;

            if (*(p++) == 0)
            {
                break;
            }

            ulRemaining = strlen2(p);

            // CACHE:xxx --> maximum disk cache size
            if (    (ulRemaining > 6)
                 && (*p     == 'C')
                 && (*(p+1) == 'A')
                 && (*(p+2) == 'C')
                 && (*(p+3) == 'H')
                 && (*(p+4) == 'E')
                 && (*(p+5) == ':')
               )
            {
                cache_size = strtoul2(p+6,
                                      &G_pNext,     // DATA32, stack won't work
                                      0);
                p = G_pNext;

                cache_size = ((cache_size * 1024L + 61440L - 1L) / 61440L) * 61440L;
                if (cache_size < 245760L)
                    cache_size = 245760L;   // 4 * (65536 - 4096)

                continue;
            }

            // -Q --> quiet mode
            if (    (*p     == 'Q'))
            {
                Quiet = 1;
                ++p;
                continue;
            }

            // NO_STRAT2 --> do not use strategy 2 I/O
            if (    (ulRemaining >= 9)
                 && (*p     == 'N')
                 && (*(p+1) == 'O')
                 && (*(p+2) == '_')
                 && (*(p+3) == 'S')
                 && (*(p+4) == 'T')
                 && (*(p+5) == 'R')
                 && (*(p+6) == 'A')
                 && (*(p+7) == 'T')
                 && (*(p+8) == '2')
               )
            {
                p += 9;
                Use_Extended_Interface = 0;
                continue;
            }

            // CASE_RETENSIVE
            if (    (ulRemaining >= 4)
                 && (*p      == 'C')
                 && (*(p+ 1) == 'A')
                 && (*(p+ 2) == 'S')
                 && (*(p+ 3) == 'E')
                 && (*(p+ 4) == '_')
               )
            {
                p += 4;
                Case_Retensive = 1;
                continue;
            }

            // -RW --> enable read/write
            if (    (ulRemaining >= 2)
                 && (*p      == 'R')
                 && (*(p+ 1) == 'W')
               )
            {
                p += 2;
                Read_Write = 1;
                continue;
            }

            // -ERRORS:{CONTINUE|PANIC} --> error behavior
            if (    (ulRemaining > 7)
                 && (*p      == 'E')
                 && (*(p+ 1) == 'R')
                 && (*(p+ 2) == 'R')
                 && (*(p+ 3) == 'O')
                 && (*(p+ 4) == 'R')
                 && (*(p+ 5) == 'S')
                 && (*(p+ 6) == ':')
               )
            {
                p += 8;
                if (*(p + 7) == 'C')     // "CONTINUE"
                    Errors_Panic = 0;
                else if (*(p + 7) == 'P')     // "PANIC"
                    Errors_Panic = 1;
                continue;
            }
        }


#if 0
            //
            // Disable automatic e2fsck when Linux mounts an ext2fs partition "touched"
            // by OS/2.
            //
            ulLength = strlen(DS32_NO_AUTO_FSCK);
            if (strncmp(pTmp1, DS32_NO_AUTO_FSCK, ulLength) == 0)
            {
                auto_fsck = 0;
                continue;
            }

            //
            // OPEN_FLAGS_WRITETHROUGH and OPEN_FLAGS_NOCACHE support
            //
            ulLength = strlen(DS32_WRITE_THROUGH);
            if (strncmp(pTmp1, DS32_WRITE_THROUGH, ulLength) == 0)
            {
                write_through_support = 1;
                continue;
            }

            //
            // debug output port
            //
            if (strncmp(pTmp1, "OUTPUT=", sizeof("OUTPUT=") - 1) == 0)
            {
                if (strncmp(pTmp1 + sizeof("OUTPUT=") - 1, "COM1", sizeof("COM1") - 1) == 0)
                {
                    debug_com = 1;
                    debug_port = OUTPUT_COM1;
                }
                if (strncmp(pTmp1 + sizeof("OUTPUT=") - 1, "COM2", sizeof("COM2") - 1) == 0)
                {
                    debug_com = 1;
                    debug_port = OUTPUT_COM2;
                }
                continue;
            }

            //
            // IFS entry points to trace
            //
            // check_trace(pTmp1);

            // time zone
            if (strncmp(pTmp1, "TZ:", sizeof("TZ:") - 1) == 0)
            {
                timezone = strtol(pTmp1 + sizeof("TZ:") - 1,
                                  __StackToFlat(&pTmp1),
                                  0);
                if ((timezone < -1440) || (timezone > 1440))
                {
                    timezone = 0;   // Invalid time zone : default to UTC

                }
                else
                {
                    timezone *= -60;    // Converts in seconds from UTC

                }
                printk("\t Timezone = %d", timezone);
                continue;
            }

            //
            // Force strategy 2 I/O (if available) on removable media.
            //
            if (strncmp(pTmp1, "FORCE_STRAT2:", sizeof("FORCE_STRAT2:") - 1) == 0)
            {
                int d;

                d = strtol(pTmp1 + sizeof("FORCE_STRAT2:") - 1,
                           __StackToFlat(&pTmp1),
                           0);
                if ((d >= 0) && (d <= 26))
                {
                    printk("Forcing strategy 2 I/O on drive %d", d);
                    drive_options[d] = 1;
                }
                continue;
            }

            //
            // Unknown command line option
            //
            //                  DosWrite(1, "EXT2-OS2 : Unknown option ", sizeof("EXT2-OS2 : Unknown option ") - 1, &lg);
            //                  DosWrite(1, pTmp1, strlen(pTmp1), &lg);
            //                  DosWrite(1, "\r\n", sizeof("\r\n") - 1, &lg);
            //              }
        } // end for (pTmp1 = strpbrk(pszCmdLine, "-/");
#endif

    } // end if (pszCmdLine)

    rc = NO_ERROR;          // for now

    /*
     *  lock code and data segments into physical memory
     *      --> moved this to the asm code
     */

    /*
     * initialize the logging facility
     */

    BufPtr = 0;
    BufOpen = 0;
    // memset(BufMsg, 0, BUFMSGSIZ);        // this crashes on rep stosd....
    for (i = 0, pTmp1 = BufMsg;
         i < BUFMSGSIZ;
         i++, pTmp1++)
    {
        *pTmp1 = '\0';
    }

    /*
     * If booted from ext2-os2 MINIFSD.FSD, we must inherit files, inodes and superblocks.
     */

    /* if (parms->pMiniFSD.seg)
    {
        printk("\tstruct inode            : %d", sizeof(struct inode));
        printk("\tstruct super_block      : %d", sizeof(struct super_block));
        printk("\tstruct file             : %d", sizeof(struct file));
        printk("\tstruct ext2_inode       : %d", sizeof(struct ext2_inode));
        printk("\tstruct ext2_super_block : %d", sizeof(struct ext2_super_block));
        printk("\tstruct minifsd_data     : %d", sizeof(struct minifsd_to_fsd_data));
        printk("\tstruct ext2_sb_info     : %d", sizeof(struct ext2_sb_info));

        if ((rc = DevHlp32_VirtToLin(parms->pMiniFSD, __StackToFlat(&pMiniFSD))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(*(PTR16 *) pMiniFSD, __StackToFlat(&pMiniFSD))) == NO_ERROR)
            {
                if (pMiniFSD->mfsdata_magic == MINIFSD_DATA_MAGIC)
                {
                    // inherits inodes from the mini FSD
                    inherit_minifsd_inodes(pMiniFSD);
                    printk("inherit_minifsd_inodes");

                    // inherits files from the mini FSD
                    inherit_minifsd_files(pMiniFSD);
                    printk("inherit_minifsd_files");

                    // inherits superblocks (mounted volumes ) from the mini FSD
                    inherit_minifsd_supers(pMiniFSD);
                    printk("inherit_minifsd_supers");
                }
                else
                {
                    printk("pMiniFSD: bad magic");
                }
            }
        }
    } // end if (parms->pMiniFSD.seg)
    */

    // output banner --> moved this to asm code

    return rc;
}

/* ******************************************************************
 *
 *   Ring-0 initialization (CODE32 segment)
 *
 ********************************************************************/

/*
 *@@ fs32_init_CheckRing0Initialize:
 *      ring-0 part of the initialization.
 *
 *      This gets called from fs32_mount() or
 *      fs32_fsctl, whichever comes first.
 *      This does the ring-0 part of the
 *      initialization, which would fail if
 *      we did these calls in fs32_init().
 *
 *      All this code used to be in fs32_init()
 *      in the original ext2-os2 when it was
 *      still running in ring 0.
 *
 *@@added V0.9.12 (2001-05-06) [umoeller]
 */

void _Optlink fs32_init_CheckRing0Initialize(void)
{
    // since this code is running at ring 0, we can call these
    // init funcs now, but we should only do this once
    if (!G_fRing0Initialized)
    {
        name_cache_init(0, 0);      // Directory entry cache initialization
                                    // vfs\dcache.c

        inode_init(0, 0);           // I-node table initialization
                                    // vfs\inode.c

        reqlist_init(2);            // Strategy 2 request list table initialization
                                    // vfs\reqlist.c

        file_table_init(0, 0);      // File table initialization
                                    // vfs\file_table.c

        buffer_init();              // Disk cache initialization
                                    // vfs\buffer.c

        // and don't do this again
        G_fRing0Initialized = 1;
    }
    // end V0.9.12 (2001-05-03) [umoeller]
}


