//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/log.c,v 1.1 2001/05/09 17:49:50 umoeller Exp $
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

#include <string.h>
#include <stdarg.h>
#define VA_START(ap, last) ap = ((va_list)__StackToFlat(&last)) + __nextword(last)
#include <builtin.h>


#include <os2\log.h>
#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\devhlp32.h>
#include <os2\fsh32.h>
#include <linux\fs.h>
#include <os2\os2misc.h>
#include <os2\os2proto.h>
#include <os2\ifsdbg.h>


static void output_com(char *bufptr, int port);

//#ifndef MINIFSD
//#pragma alloc_text(EXT2_FIXED_CODE, kernel_printf)
//#pragma alloc_text(EXT2_FIXED_CODE, fs_log)
//#pragma alloc_text(EXT2_FIXED_CODE, output_com)
//#endif

//
// Buffer used to communicate with ext2-os2.exe
//

char    BufMsg[BUFMSGSIZ] = {0};       // Circular buffer
UINT16  BufPtr = 0;                  // Buffer pointer
UINT16  BufOpen = 0;                 // Open flag (ext2-os2.exe present ?)
UINT32  BufSem = 0;                  // Buffer semaphore

char debug_com  = 0;             // output debug info to COM port
int  debug_port = OUTPUT_COM2;   // base I/O address of COM port

int fs_err(UINT32 infunction, UINT32 errfunction, int retcode, UINT32 sourcefile, UINT32 sourceline) {
    int         rc;
    err_record *Tmp;
    int         i;

    i = sizeof(err_record);
    if (i + BufPtr < BUFMSGSIZ) {
        Tmp = (err_record *)&(BufMsg[BufPtr]);
        Tmp->Type        = LOG_FS_ERR;
        Tmp->infunction  = infunction;  /* function where error occured  */
        Tmp->errfunction = errfunction; /* function returning error code */
        Tmp->retcode     = retcode;     /* return code from errfunction  */
        Tmp->sourcefile  = sourcefile;  /* file where error occured      */
        Tmp->sourceline  = sourceline;  /* line number in sourcefile     */
        BufPtr += i;
    } else {
        Tmp = (err_record *)BufMsg;
        Tmp->Type        = LOG_FS_ERR;
        Tmp->infunction  = infunction;  /* function where error occured  */
        Tmp->errfunction = errfunction; /* function returning error code */
        Tmp->retcode     = retcode;     /* return code from errfunction  */
        Tmp->sourcefile  = sourcefile;  /* file where error occured      */
        Tmp->sourceline  = sourceline;  /* line number in sourcefile     */
        BufPtr = i;
    } /* end if */

    //
    // Clears the log semaphore so that ext2-os2.exe can retrieve data
    //
    fsh32_semclear(&BufSem);

    return NO_ERROR;

}


/***********************************************************************************/
/*** fs_log()                                                                    ***/
/***********************************************************************************/
int fs_log(char *text)
{
    int rc;

    //
    // We copy the text into the circular buffer :
    //   - at the current position if it fits
    //   - at the buffer beginning otherwise
    //
    int i = strlen(text) + 1;
    if (i + BufPtr < BUFMSGSIZ)
    {
        strcpy(&(BufMsg[BufPtr]), text);
        BufPtr += i;
    }
    else
    {
        strcpy(BufMsg, text);
        BufPtr = i;
    } /* end if */

    //
    // Clears the log semaphore so that ext2-os2.exe can retrieve data
    //
    fsh32_semclear((void *)&BufSem);

#if defined (FS_TRACE) || defined (FS_TRACE_LOCKS)
    FSH_YIELD();
#endif
    return NO_ERROR;

} /*** fs_log() ****/

static void output_com(char *bufptr, int port)
{

    while (*bufptr) {
        while(!(_inp(port + 5) & 0x20)); // Waits for COM port to be ready
        _outp(port, *bufptr++);          // Outputs our character
    }
    while(!(_inp(port + 5) & 0x20));     // Waits for COM port to be ready
    _outp(port, '\r');                   // Outputs our character
    while(!(_inp(port + 5) & 0x20));     // Waits for COM port to be ready
    _outp(port, '\n');                   // Outputs our character
}

int kernel_printf(const char *fmt, ...)
{
    va_list arg;
    char  Buf[256];
    char *buf = __StackToFlat(Buf);
    char *tmp = buf;

    tmp += sprintf(buf,
                   "[%04X-%04X] ",
                   G_pLocInfoSeg->LIS_CurProcID,
                   G_pLocInfoSeg->LIS_CurThrdID);

    VA_START(arg, fmt);
    vsprintf(tmp, fmt, arg);
    va_end(arg);

    if (debug_com)
        output_com(buf, debug_port);

    return fs_log(buf);
}


int sprintf(char * buf, const char *fmt, ...)
{
    va_list args;
    int i;

    VA_START(args, fmt);
    i=vsprintf(buf,fmt,args);
    va_end(args);
    return i;
}
