
/*
 *@@sourcefile fs32_exit.c:
 *      32-bit C handler called from 16-bit entry point
 *      in src\interface\fs_thunks.asm.
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

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsd32.h>
#include <os2\DevHlp32.h>
#include <os2\trace.h>
#include <os2\os2proto.h>

/*
 * struct fs32_exit_parms {
 *     unsigned short  pdb;
 *     unsigned short  pid;
 *     unsigned short  uid;
 * };
 */
int FS32ENTRY fs32_exit(struct fs32_exit_parms *parms)
{


    if (trace_FS_EXIT)
    {
        kernel_printf("FS_EXIT( uid = %u pid = %u pdb = %u )", parms->uid, parms->pid, parms->pdb);
    }

    return NO_ERROR;
}
