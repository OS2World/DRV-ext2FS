
/*
 *@@sourcefile fs32_opencreate.c:
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
#include <os2\fsh32.h>
#include <os2\DevHlp32.h>
#include <os2\log.h>
#include <os2\trace.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <linux\fs_proto.h>
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>
#include <linux\fcntl.h>

#define THISFILE FILE_TEST_C    // obsolete
/***********************************************************************************/
/*** Some useful defines for FS_OPENCREATE() ...                                 ***/
/***********************************************************************************/

#define OPEN_ACCESS_MASK               0x0007   /* ---- ---- ---- -111 */
#define OPEN_ACCESS_EXECUTE            0x0003   /* ---- ---- ---- -100 */

#define OPEN_SHARE_MASK                0x0070   /* ---- ---- -111 ---- */
#define OPEN_LOCALITY_MASK             0x0700   /* ---- -111 ---- ---- */
#define OPEN_ACTION_EXIST_MASK         0x000F   /* ---- ---- ---- 1111 */
#define OPEN_ACTION_NEW_MASK           0x00F0   /* ---- ---- 1111 ---- */

#define FILE_NONFAT     0x0040  // File is non 8.3 compliant

extern char write_through_support;



/*
 * Skip drive letter
 */
char *Skip_drive(char *path)
{
    char *pathname = path;

    if (pathname[0] &&
        (((pathname[0] >= 'A') && (pathname[0] <= 'Z')) ||
         ((pathname[0] >= 'a') && (pathname[0] <= 'z'))) &&
        pathname[1] &&
        (pathname[1] == ':'))
    {
        pathname += 2;
    }
    return pathname;
}

/*
 * struct fs32_opencreate_parms {
 *     PTR16          pfgenflag;
 *     PTR16          pEABuf;
 *     unsigned short attr;
 *     PTR16          pAction;
 *     unsigned short openflag;
 *     unsigned long  openmode;
 *     PTR16          psffsd;
 *     PTR16          psffsi;
 *     unsigned short iCurDirEnd;
 *     PTR16          pName;
 *     PTR16          pcdfsd;
 *     PTR16          pcdfsi;
 * };
 */
int FS32ENTRY fs32_opencreate(struct fs32_opencreate_parms *parms)
{
    char *pName;
    struct cdfsi32 *pcdfsi;
    union cdfsd32 *pcdfsd;
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    unsigned short *pAction;
    int rc;
    struct super_block *sb;
    struct file *p_file, *dir;
    UINT32 openmode, DOSmode;
    UINT32 accessmode;
    UINT16 newflag, existflag;
    char component[CCHMAXPATH];
    char parent[CCHMAXPATH];
    struct inode *inode;
    struct inode *inode_parent;
    struct inode *base;
    char *tmp;
    ino_t ino_no;

    psffsi = VDHQueryLin(parms->psffsi);
    psffsd = VDHQueryLin(parms->psffsd);
    if (parms->pcdfsi.seg)
        pcdfsi = VDHQueryLin(parms->pcdfsi);
    else
        pcdfsi = 0;
    if (parms->pcdfsd.seg)
        pcdfsd = VDHQueryLin(parms->pcdfsd);
    else
        pcdfsd = 0;
    pName = VDHQueryLin(parms->pName);
    pAction = VDHQueryLin(parms->pAction);

    if (trace_FS_OPENCREATE)
    {
        kernel_printf("FS_OPENCREATE(%s)", pName);
    }


#ifdef FS_TRACE
    if (parms->ulOpenMode & OPEN_FLAGS_DASD)
    {
        fs_log("OPEN_FLAGS_DASD");
    }


    if (parms->ulOpenMode & OPEN_FLAGS_WRITE_THROUGH)
    {
        fs_log("OPEN_FLAGS_WRITE_THROUGH");
    }
    if (parms->ulOpenMode & OPEN_FLAGS_FAIL_ON_ERROR)
    {
        fs_log("OPEN_FLAGS_FAIL_ON_ERROR");
    }
    if (parms->ulOpenMode & OPEN_FLAGS_NO_CACHE)
    {
        fs_log("OPEN_FLAGS_NO_CACHE");
    }
    if (parms->ulOpenMode & OPEN_FLAGS_NOINHERIT)
    {
        fs_log("OPEN_FLAGS_NO_INHERIT");
    }
#endif
    accessmode = parms->ulOpenMode & OPEN_ACCESS_MASK;

    if (accessmode == OPEN_ACCESS_READONLY)
    {
#ifdef FS_TRACE
        fs_log("OPEN_ACCESS_READONLY");
#endif
        openmode = OPENMODE_READONLY;
    }

    if (accessmode == OPEN_ACCESS_WRITEONLY)
    {
#ifdef FS_TRACE
        fs_log("OPEN_ACCESS_WRITEONLY");
#endif
        openmode = OPENMODE_WRITEONLY;
    }

    if (accessmode == OPEN_ACCESS_READWRITE)
    {
#ifdef FS_TRACE
        fs_log("OPEN_ACCESS_READWRITE");
#endif
        openmode = OPENMODE_READWRITE;
    }

#ifdef FS_TRACE
    if (accessmode == OPEN_ACCESS_EXECUTE)
    {
        fs_log("OPEN_ACCESS_EXECUTE");
    }
#endif

    newflag = parms->openflag & OPEN_ACTION_NEW_MASK;

#ifdef FS_TRACE
    if (newflag == OPEN_ACTION_FAIL_IF_NEW)
    {
        fs_log("OPEN_ACTION_FAIL_IF_NEW");
    }
    if (newflag == OPEN_ACTION_CREATE_IF_NEW)
    {
        fs_log("OPEN_ACTION_CREATE_IF_NEW");
    }
#endif

    existflag = parms->openflag & OPEN_ACTION_EXIST_MASK;

#ifdef FS_TRACE
    if (existflag == OPEN_ACTION_OPEN_IF_EXISTS)
    {
        fs_log("OPEN_ACTION_OPEN_IF_EXISTS");
    }
    if (existflag == OPEN_ACTION_FAIL_IF_EXISTS)
    {
        fs_log("OPEN_ACTION_FAIL_IF_EXISTS");
    }
    if (existflag == OPEN_ACTION_REPLACE_IF_EXISTS)
    {
        fs_log("OPEN_ACTION_REPLACE_IF_EXISTS");
    }
#endif

    if ((!Read_Write) &&
        ((accessmode == OPEN_ACCESS_READWRITE) ||
         (accessmode == OPEN_ACCESS_WRITEONLY)))
    {
        fs_log("FS_OPENCREATE() - Write access not enabled");
        return ERROR_WRITE_PROTECT;
    }

    //
    // Direct access open of the whole device
    //
    if (parms->ulOpenMode & OPEN_FLAGS_DASD)
    {
        sb = getvolume(psffsi->sfi_hVPB);
        kernel_printf("OPEN_FLAGS_DASD");
        if ((p_file = _open_by_inode(sb, INODE_DASD, openmode)) == 0)
        {
            kernel_printf("FS_OPENCREATE() - couldn't DASD open %s", pName);
            return ERROR_OPEN_FAILED;
        }
        psffsd->f = p_file;
        psffsi->sfi_tstamp = ST_SCREAT | ST_PCREAT;
        psffsi->sfi_size = p_file->f_inode->i_size;
        psffsi->sfi_position = p_file->f_pos;
        date_unix2dos(p_file->f_inode->i_ctime, &(psffsi->sfi_ctime), &(psffsi->sfi_cdate));
        date_unix2dos(p_file->f_inode->i_atime, &(psffsi->sfi_atime), &(psffsi->sfi_adate));
        date_unix2dos(p_file->f_inode->i_mtime, &(psffsi->sfi_mtime), &(psffsi->sfi_mdate));
        return NO_ERROR;

    }

    //
    // Now that we treated the OPEN_FLAGS_DASD special case, lets treat the general case :
    // Try to open the file readonly
    // Success : the file exists
    //     if !S_ISDIR && !S_ISREG => error
    //     if OPEN_ACTION_FAIL_IF_EXISTS set => error
    //     if OPEN_ACTION_OPEN_IF_EXISTS set
    //         <test file attrs>
    //         change the open mode and return OK
    //     if OPEN_ACTION_REPLACE_IF_EXISTS set
    //         OPEN_ACCESS_READONLY or OPEN_ACCESS_EXECUTE set => error
    //         OPEN_ACCESS_READWRITE or OPEN_ACCESS_WRITEONLY set
    //             truncate
    //             change openmode and return
    // Failure : the file does not exist
    //     OPEN_ACCESS_READONLY or OPEN_ACCESS_EXECUTE set => error
    //     OPEN_ACCESS_READWRITE or OPEN_ACCESS_WRITEONLY set
    //         if OPEN_ACTION_CREATE_IF_NEW set
    //             try to create the file
    //             open the file and return
    //         if OPEN_ACTION_FAIL_IF_NEW   set => error
    rc = ERROR_INVALID_PARAMETER;
    if (parms->iCurDirEnd != CURDIREND_INVALID)
    {
        tmp = pName + parms->iCurDirEnd;
        if ((pcdfsd->u.p_file) && (pcdfsd->u.p_file->f_magic == FILE_MAGIC))
        {
            base = pcdfsd->u.p_file->f_inode;
            if (base)
            {
                sb = base->i_sb;
                rc = NO_ERROR;
            }
        }
    }
    else
    {
        sb = getvolume(psffsi->sfi_hVPB);
        if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
        {
            tmp = skip_drive(pName);
            base = sb->s_mounted;
            if (base)
            {
                rc = NO_ERROR;
            }
        }
    }

    if (rc != NO_ERROR)
    {
        return rc;
    }

    p_file = open_by_name(base, tmp, openmode);
    if (p_file)
    {                           // The file exists
        //
        // If it's not a regular file or a directory we cannot open
        //

        if (!S_ISREG(p_file->f_inode->i_mode))
        {
//            kernel_printf("Can't FS_OPENCREATE - %s is not a regular file", pName);
            if ((rc = vfs_close(p_file)) != NO_ERROR)
            {
                fs_err(FUNC_FS_OPENCREATE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
                return rc;
            }
            return ERROR_ACCESS_DENIED;
        }
        //
        // if OPEN_ACTION_FAIL_IF_EXISTS set => error
        //
        if (existflag == OPEN_ACTION_FAIL_IF_EXISTS)
        {
#ifdef FS_TRACE
            fs_log("Can't FS_OPENCREATE() - File exists & OPEN_ACTION_FAIL_IF_EXISTS");
#endif
            if ((rc = vfs_close(p_file)) != NO_ERROR)
            {
                fs_err(FUNC_FS_OPENCREATE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
                return rc;
            }
            return ERROR_FILE_EXISTS;
        }

        //
        // if OPEN_ACTION_OPEN_IF_EXISTS : OK
        //
        if (existflag == OPEN_ACTION_OPEN_IF_EXISTS)
        {
            *pAction = FILE_EXISTED;
        }

        //
        // if OPEN_ACTION_REPLACE_IF_EXISTS : truncate
        //
        if (existflag == OPEN_ACTION_REPLACE_IF_EXISTS)
        {
            p_file->f_inode->i_size = psffsi->sfi_size;
            p_file->f_inode->i_op->truncate(p_file->f_inode);
            p_file->f_inode->i_dirt = 1;
            p_file->f_flags = O_TRUNC;
            *pAction = FILE_TRUNCATED;
#if 0
            psffsi->sfi_tstamp = ST_PWRITE | ST_SWRITE;
#else
            /*
             * Time stamping is done by inode routines - Only propagate value.
             */
            psffsi->sfi_tstamp = ST_PWRITE;
#endif

        }
    }
    else
    {                           // The file doesn't exist

        ExtractPath(pName, __StackToFlat(parent));
        ExtractName(pName, __StackToFlat(component));
        //
        // We try to open the parent dir
        //
        if ((dir = open_by_name(sb->s_mounted, Skip_drive(__StackToFlat(parent)), OPENMODE_READONLY)) == 0)
        {
//            kernel_printf("FS_OPENCREATE() - The parent directory %s doesn't seem to exist", parent);
            return ERROR_PATH_NOT_FOUND;
        }

        //
        // The parent dir exists
        //

        //
        // If the file is open for execution : error (it doesn't even exist)
        //
        if (accessmode == OPEN_ACCESS_EXECUTE)
        {
#ifdef FS_TRACE
            fs_log("Can't FS_OPENCREATE() - File doesn't exist & OPEN_ACCESS_EXECUTE");
#endif
            if ((rc = vfs_close(dir)) != NO_ERROR)
            {
                fs_err(FUNC_FS_OPENCREATE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
                return rc;
            }
            return ERROR_FILE_NOT_FOUND;
        }

        //
        // If the file is open for writing or readwrite ...
        //
        if ((accessmode == OPEN_ACCESS_READONLY) ||
            (accessmode == OPEN_ACCESS_READWRITE) ||
            (accessmode == OPEN_ACCESS_WRITEONLY))
        {
            if (newflag == OPEN_ACTION_FAIL_IF_NEW)
            {
#ifdef FS_TRACE
                fs_log("Can't FS_OPENCREATE() - File doesn't exist &  OPEN_ACTION_FAIL_IF_NEW");
#endif
                if ((rc = vfs_close(dir)) != NO_ERROR)
                {
                    fs_err(FUNC_FS_OPENCREATE, FUNC_CLOSE, rc, FILE_TEST_C, __LINE__);
                    return rc;
                }
                return ERROR_OPEN_FAILED;
            }

            if (newflag == OPEN_ACTION_CREATE_IF_NEW)
            {
//                ino_no = dir->f_inode->i_ino;
                inode_parent = dir->f_inode;
                inode_parent->i_count++;
                if ((rc = vfs_close(dir)) != NO_ERROR)
                {
                    fs_err(FUNC_FS_OPENCREATE, FUNC_CLOSE, rc, THISFILE, __LINE__);
                    return rc;
                }
//                inode_parent = iget(sb, ino_no);
                inode_parent->i_count++;
                down(&inode_parent->i_sem);
                rc = inode_parent->i_op->create(inode_parent, __StackToFlat(component), strlen(component), S_IRWXU | S_IFREG, __StackToFlat(&inode));
                up(&inode_parent->i_sem);
                if (rc)
                {
                    kernel_printf("Couldn't create %s", pName);
                    iput(inode_parent);
                    return rc;
                }
                ino_no = inode->i_ino;
                iput(inode_parent);
                iput(inode);
                if ((p_file = _open_by_inode(sb, ino_no, openmode)) == 0)
                {
                    kernel_printf("open_by_inode(%lu) failed in FS_OPENCREATE", ino_no);
                    return ERROR_OPEN_FAILED;
                }
                p_file->f_inode->i_size = psffsi->sfi_size;
                p_file->f_inode->i_dirt = 1;
                p_file->f_flags = O_CREAT;
                *pAction = FILE_CREATED;
#if 0
                psffsi->sfi_tstamp = ST_SCREAT | ST_PCREAT | ST_PWRITE | ST_SWRITE;
#else
                /*
                 * Time stamping is done by inode routines - Only propagate value.
                 */
                psffsi->sfi_tstamp = ST_PCREAT | ST_PWRITE;
#endif

            }

        }

    }





    psffsd->f = p_file;
    /*
     * Time stamping is done by inode routines - Only propagate value.
     */
#if 0
    psffsi->sfi_tstamp |= ST_PREAD | ST_SREAD;
#else
    /*
     * Time stamping is done by inode routines - Only propagate value.
     */
    psffsi->sfi_tstamp |= ST_PREAD;
#endif
    psffsi->sfi_size = p_file->f_inode->i_size;
    psffsi->sfi_position = p_file->f_pos;

//    kernel_printf("date = %u/%u/%u", (pCommon->dateCreate) & 31, (pCommon->dateCreate >> 5) & 15 , (pCommon->dateCreate) >> 9);

    date_unix2dos(p_file->f_inode->i_ctime, &(psffsi->sfi_ctime), &(psffsi->sfi_cdate));
    date_unix2dos(p_file->f_inode->i_atime, &(psffsi->sfi_atime), &(psffsi->sfi_adate));
    date_unix2dos(p_file->f_inode->i_mtime, &(psffsi->sfi_mtime), &(psffsi->sfi_mdate));

    psffsi->sfi_DOSattr = (unsigned char)Linux_To_DOS_Attrs(p_file->f_inode, __StackToFlat(component));
    if (write_through_support)
    {
        if ((parms->ulOpenMode & OPEN_FLAGS_WRITE_THROUGH) ||
            (parms->ulOpenMode & OPEN_FLAGS_NO_CACHE))
        {
            p_file->f_flags |= O_SYNC;
            p_file->f_inode->i_flags |= MS_SYNCHRONOUS;
        }
    }
    return NO_ERROR;

}
