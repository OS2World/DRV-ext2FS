
/*
 *@@sourcefile fs32_findclose.c:
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

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <linux\fs.h>
#include <os2\os2proto.h>
#include <os2\fsd32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>
#include <os2\filefind.h>

/*
 * struct fs32_findclose_parms {
 *     PTR16               pfsfsd;
 *     PTR16               pfsfsi;
 * };
 */
int FS32ENTRY fs32_findclose(struct fs32_findclose_parms *parms)
{
    struct fsfsi32 *pfsfsi;
    struct fsfsd32 *pfsfsd;
    int rc;

    pfsfsi = VDHQueryLin(parms->pfsfsi);
    pfsfsd = VDHQueryLin(parms->pfsfsd);

    if (trace_FS_FINDCLOSE)
    {
        kernel_printf("FS_FINDCLOSE" " pre-invocation : %s", ((p_hfind) pfsfsd)->pName);
    }

    /*
     * Closes the search directory
     */
    if ((rc = vfs_close(((p_hfind) pfsfsd)->p_file)) == NO_ERROR)
    {
        /*
         * Frees the memory allocated for the search
         */
        if ((rc = DevHlp32_VMFree(((p_hfind) pfsfsd)->pName)) == NO_ERROR)
        {
            /*
             * Zero out struct FS dependant file search area for safety purposes.
             */
            memset(pfsfsd, 0, sizeof(struct fsfsd32));
        }
    }

    if (trace_FS_FINDCLOSE)
    {
        kernel_printf("FS_FINDCLOSE" " post-invocation : rc = %d", rc);
    }

    return rc;
}
