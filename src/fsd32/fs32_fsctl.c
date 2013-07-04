
/*
 *@@sourcefile fs32_fsctl.c:
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
#include <os2\ctype.h>
#include <linux\ioctl.h>

/*
 * Error messages to return for func. FSCTL_FUNC_NEW_INFO
 */
static struct fsctl_msg ext2_os2_magic =
{sizeof(magic_msg), magic_msg};
static struct fsctl_msg ext2_os2_default =
{sizeof(default_msg), default_msg};

int fsctl_func_new_info(struct fs32_fsctl_parms *parms)
{
    int rc, rc2;
    char *pParm;
    char *pData;
    unsigned short *pLenParmOut;
    unsigned short *pLenDataOut;
    char lock_1[12];
    char lock_2[12];
    char lock_3[12];
    char lock_4[12];
    unsigned long PgCount;
    short error;

    if ((rc = DevHlp32_VirtToLin(parms->pParm, __StackToFlat(&pParm))) == NO_ERROR)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&pLenDataOut))) == NO_ERROR)
            {
                if ((rc = DevHlp32_VirtToLin(parms->plenParmOut, __StackToFlat(&pLenParmOut))) == NO_ERROR)
                {

                    rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pParm, sizeof(short), (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));

                    if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                    {

                        error = *((short *)pParm);
                        rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(struct fsctl_msg), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                        if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                        {

                            if (error == NO_ERROR)
                            {
                                memcpy(pData, &ext2_os2_magic, sizeof(ext2_os2_magic));
                            }
                            else
                            {
                                memcpy(pData, &ext2_os2_default, sizeof(ext2_os2_default));
                            }

                            rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pLenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_3), __StackToFlat(&PgCount));

                            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                            {
                                *pLenDataOut = sizeof(struct fsctl_msg);

                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pLenParmOut, sizeof(short), (void *)-1, __StackToFlat(lock_4), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {
                                    *pLenParmOut = 0;
                                    rc = NO_ERROR;
                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_4))) == NO_ERROR)
                                    {
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }
                                }
                                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_3))) == NO_ERROR)
                                {
                                }
                                else
                                {
                                    rc = rc2;
                                }
                            }
                            if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                            {
                            }
                            else
                            {
                                rc = rc2;
                            }

                        }
                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                        {
                        }
                        else
                        {
                            rc = rc2;
                        }

                    }
                }
            }
        }
    }
    return rc;
}

int ifsdbg_open(struct fs32_fsctl_parms *parms)
{
    int rc;

    if (BufOpen == 0)
    {
        BufOpen = 1;
        if (BufPtr == 0)
        {
            fsh32_semset(&BufSem);
        }
        rc = NO_ERROR;
    }
    else
    {
        rc = ERROR_DEVICE_IN_USE;
    }
    return rc;
}

int ifsdbg_close(struct fs32_fsctl_parms *parms)
{
    int rc;

    if (BufOpen == 1)
    {
        BufOpen = 0;
        rc = NO_ERROR;
    }
    else
    {
        rc = ERROR_NOT_READY;
    }
    return rc;
}

int ifsdbg_read(struct fs32_fsctl_parms *parms)
{
    int rc, rc2;
    char *pData;
    unsigned short *plenDataOut;
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;

    if (BufOpen == 1)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
            {
                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, parms->lenData, (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));
                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                {
                    rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                    if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                    {

                        if ((rc = fsh32_semwait(&BufSem, -1)) == NO_ERROR)
                        {
/*************************************************************/
/*** If no data is present, simply set the semaphore       ***/
/*************************************************************/
                            if (BufPtr == 0)
                            {
                                *plenDataOut = 0;
                                rc = NO_ERROR;
                            }
                            else
                            {

/*************************************************************/
/*** If the log data is smaller than the requested amount  ***/
/*** we copy them all                                      ***/
/*************************************************************/
                                if (BufPtr < parms->lenData)
                                {
                                    memcpy(pData, BufMsg, BufPtr + 1);
                                    *plenDataOut = BufPtr + 1;
                                    BufPtr = 0;
                                    rc = NO_ERROR;
                                }
                            }
/*************************************************************/
/*** We set the log semaphore                              ***/
/*** ext2-os2.exe will wait on it until some more data is  ***/
/*** present                                               ***/
/*************************************************************/
                            fsh32_semset(&BufSem);

                        }
                        else
                        {
                            printk("error fsh32_semwait %d", rc);
                            BufOpen = 0;
                        }

                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                        {
                        }
                        else
                        {
                            rc = rc2;
                        }

                    }
                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                    {
                    }
                    else
                    {
                        rc = rc2;
                    }
                }
            }
        }
    }
    else
    {
        rc = ERROR_NOT_READY;
    }
    return rc;
}

int ext2_os2_bdflush(struct fs32_fsctl_parms *parms)
{
    /*
     * We should never return from this call.
     */
    sys_bdflush(0, 0);

    return NO_ERROR;            // makes compiler happy

}



int ext2_os2_shrink_cache(struct fs32_fsctl_parms *parms)
{
    int forever = 1;

    while (forever)
    {
        if (buffermem)
            shrink_buffers(3);
        DevHlp32_ProcBlock((unsigned long)ext2_os2_shrink_cache, 2000, 1);
    }
    return NO_ERROR;            // makes compiler happy

}

int ext2_os2_getdata(struct fs32_fsctl_parms *parms)
{
    int rc, rc2;
    char *pData;
    unsigned short *plenDataOut;
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    struct ext2_os2_data *d;

    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
    {
        if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
        {
            rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(struct ext2_os2_data), (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));

            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
            {
                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                {

                    d = (struct ext2_os2_data *)pData;

                    /*
                     * Buffers statistics
                     */
                    d->b.buffer_mem = buffermem;
                    d->b.cache_size = cache_size;
                    d->b.nr_buffer_heads = nr_buffer_heads;

                    memcpy(&(d->b.nr_free), nr_free, sizeof(nr_free));
                    memcpy(&(d->b.nr_buffers_type), nr_buffers_type, sizeof(nr_buffers_type));

                    /*
                     * I-nodes statistics
                     */
                    d->i.nr_inodes = nr_inodes;
                    d->i.nr_free_inodes = nr_free_inodes;
                    d->i.nr_iget = nr_iget;
                    d->i.nr_iput = nr_iput;

                    /*
                     * Files statistics
                     */
                    d->f.nhfiles = nr_files;
                    d->f.nfreehfiles = nr_free_files;
                    d->f.nusedhfiles = nr_used_files;

                    /*
                     * Swapper statistics
                     */
                    d->s.nr_total_pgin = nr_total_pgin;
                    d->s.nr_total_pgout = nr_total_pgout;
                    d->s.nr_pgin = nr_pgin;
                    d->s.nr_pgout = nr_pgout;
                    nr_pgin = 0;
                    nr_pgout = 0;

                    *plenDataOut = sizeof(struct ext2_os2_data);

                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                    {
                    }
                    else
                    {
                        rc = rc2;
                    }

                }
                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                {
                }
                else
                {
                    rc = rc2;
                }
            }
        }
    }
    return rc;

}


//
// VFSAPI Library entry point : this is the vfs_sync() routine (standart sync() system call)
// Input :
//      pData (ignored)
//      pParm (ignored)
// Output :
//      pData (ignored)
//      pParm (ignored)
//
int ext2_os2_sync(struct fs32_fsctl_parms *parms)
{
    int rc;

    if (Read_Write)
    {
        sys_sync();
        rc = NO_ERROR;
    }
    else
    {
        rc = ERROR_WRITE_PROTECT;
    }
    return rc;
}

int ext2_os2_stat(struct fs32_fsctl_parms *parms)
{
    int rc, rc2;
    char *pData;
    unsigned short *plenDataOut;
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    struct ext2_os2_data *d;
    union argdat32 *pArgdat;
    struct super_block *sb;
    char *pPath;
    struct cdfsi32 *pcdfsi;

    if (parms->iArgType == FSCTL_ARG_CURDIR)
    {
        if (parms->lenData >= sizeof(struct new_stat))
        {
            if ((rc = DevHlp32_VirtToLin(parms->pArgdat, __StackToFlat(&pArgdat))) == NO_ERROR)
            {
                if ((rc = DevHlp32_VirtToLin(pArgdat->cd.pPath, __StackToFlat(&pPath))) == NO_ERROR)
                {
                    if ((rc = DevHlp32_VirtToLin(pArgdat->cd.pcdfsi, __StackToFlat(&pcdfsi))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                        {
                            if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(struct new_stat), (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {
                                    rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                                    if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                    {

                                        //
                                        // VFSAPI Library entry point : this is the vfs_stat() routine (standart stat() system call)
                                        // Input :
                                        //         pData must point to a buffer large enough to hold struct vfs_stat
                                        //      pParm (ignored)
                                        // Output :
                                        //         pData contains the struct vfs_stat if success, or garbage if failed
                                        //      pParm (ignored)
                                        //
                                        *plenDataOut = sizeof(struct new_stat);

                                        kernel_printf("FS_FSCTL(EXT2_OS2_STAT) - Path is %s", pPath);
                                        {
                                            struct file *f;
                                            struct new_stat *s;
                                            unsigned long DOSmode;

                                            //
                                            // Gets the superblock
                                            //
                                            sb = getvolume(pcdfsi->cdi_hVPB);

                                            if ((f = open_by_name(sb->s_mounted, skip_drive(pPath), OPENMODE_READONLY)) != NULL)
                                            {

                                                s = (struct new_stat *)pData;
                                                memset(s, 0, sizeof(struct new_stat));

                                                s->st_dev = f->f_inode->i_dev;
                                                s->st_ino = f->f_inode->i_ino;
                                                s->st_mode = f->f_inode->i_mode;
                                                s->st_nlink = f->f_inode->i_nlink;
                                                s->st_uid = f->f_inode->i_uid;
                                                s->st_gid = f->f_inode->i_gid;
                                                s->st_rdev = f->f_inode->i_rdev;
                                                s->st_size = f->f_inode->i_size;
//        if (inode->i_pipe)
                                                //                tmp.st_size = PIPE_SIZE(*inode);
                                                s->st_atime = f->f_inode->i_atime;
                                                s->st_mtime = f->f_inode->i_mtime;
                                                s->st_ctime = f->f_inode->i_ctime;
                                                s->st_blocks = f->f_inode->i_blocks;
                                                s->st_blksize = f->f_inode->i_blksize;
                                                vfs_close(f);
                                            }
                                            else
                                            {
                                                kernel_printf("FS_FSCTL() - path %s not found", pPath);
                                                return rc;
                                            }
                                        }


                                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                                        {
                                        }
                                        else
                                        {
                                            rc = rc2;
                                        }

                                    }
                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                                    {
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            rc = ERROR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    return rc;

}

int ext2_os2_fstat(struct fs32_fsctl_parms *parms)
{
    int rc, rc2;
    char *pData;
    unsigned short *plenDataOut;
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    struct ext2_os2_data *d;
    union argdat32 *pArgdat;
    struct super_block *sb;
    union sffsd32 *psffsd;

    if (parms->iArgType == FSCTL_ARG_FILEINSTANCE)
    {
        if (parms->lenData >= sizeof(struct new_stat))
        {


            if ((rc = DevHlp32_VirtToLin(parms->pArgdat, __StackToFlat(&pArgdat))) == NO_ERROR)
            {
                if ((rc = DevHlp32_VirtToLin(pArgdat->sf.psffsd, __StackToFlat(&psffsd))) == NO_ERROR)
                {
                    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
                        {
                            rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, sizeof(struct new_stat), (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));

                            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {

                                    //
                                    // VFSAPI Library entry point : this is the vfs_fstat() routine (standart fstat() system call)
                                    // Input :
                                    //         pData must point to a buffer large enough to hold struct vfs_stat
                                    //      pParm (ignored)
                                    // Output :
                                    //         pData contains the struct vfs_stat if success, or garbage if failed
                                    //      pParm (ignored)
                                    //

                                    *plenDataOut = sizeof(struct new_stat);

                                    {
                                        struct new_stat *s;
                                        struct inode *inode;

                                        inode = psffsd->f->f_inode;
                                        s = (struct new_stat *)pData;

                                        memset(s, 0, sizeof(struct new_stat));

                                        s->st_dev = inode->i_dev;
                                        s->st_ino = inode->i_ino;
                                        s->st_mode = inode->i_mode;
                                        s->st_nlink = inode->i_nlink;
                                        s->st_uid = inode->i_uid;
                                        s->st_gid = inode->i_gid;
                                        s->st_rdev = inode->i_rdev;
                                        s->st_size = inode->i_size;
//        if (inode->i_pipe)
                                        //                tmp.st_size = PIPE_SIZE(*inode);
                                        s->st_atime = inode->i_atime;
                                        s->st_mtime = inode->i_mtime;
                                        s->st_ctime = inode->i_ctime;
                                        s->st_blocks = inode->i_blocks;
                                        s->st_blksize = inode->i_blksize;
                                    }
                                    rc = NO_ERROR;


                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                                    {
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }

                                }
                                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                                {
                                }
                                else
                                {
                                    rc = rc2;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            rc = ERROR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    return rc;
}


/*
 * Called to return the normalized path name passed to it. Normalization is done
 * automatically by the kernel before entering this call, so we only need to copy
 * the pathname back to the user buffer.
 * Input :
 *      pData (ignored)
 *      pParm (ignored)
 * Output :
 *      pData : normalized path name
 *      pParm (ignored)
 *
 */
int ext2_os2_normalize_path(struct fs32_fsctl_parms *parms)
{
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    char *pData;
    unsigned short *plenDataOut;
    union argdat32 *pArgdat;
    int rc;
    int rc2;
    int len;
    char *pPath;

    if (parms->iArgType == FSCTL_ARG_CURDIR)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pArgdat, __StackToFlat(&pArgdat))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(pArgdat->cd.pPath, __StackToFlat(&pPath))) == NO_ERROR)
            {
                printk("ext2_os2_normalize_path - Path is %s", pPath);
                len = strlen(pPath);
                if (parms->lenData > len)
                {
                    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
                        {
                            rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, pData, len + 1, (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));
                            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {
                                    /*
                                     * Copy the path normalized by the kernel to the user data buffer
                                     */
                                    strcpy(pData, pPath);
                                    *plenDataOut = len + 1;
                                    rc = NO_ERROR;
                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                                    {
                                        /*
                                         * Nothing else
                                         */
                                    }
                                    else
                                    {
                                        printk("ext2_os2_normalize_path - could not unlock plenDataOut - rc = %d", rc2);
                                        rc = rc2;
                                    }
                                }
                                else
                                {
                                    printk("ext2_os2_normalize_path - could not lock plenDataOut - rc = %d", rc);
                                }
                                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                                {
                                    /*
                                     * Nothing else
                                     */
                                }
                                else
                                {
                                    printk("ext2_os2_normalize_path - could not unlock pData - rc = %d", rc2);
                                    rc = rc2;
                                }
                            }
                            else
                            {
                                printk("ext2_os2_normalize_path - could not lock pData - rc = %d", rc);
                            }
                        }
                        else
                        {
                            printk("ext2_os2_normalize_path - could not thunk plenDataOut - rc = %d", rc);
                        }
                    }
                    else
                    {
                        printk("ext2_os2_normalize_path - could not thunk pData - rc = %d", rc);
                    }
                }
                else
                {
                    printk("ext2_os2_normalize_path - data buffer too small");
                    rc = ERROR_BUFFER_OVERFLOW;
                }
            }
            else
            {
                printk("ext2_os2_normalize_path - could not thunk pPath - rc = %d", rc);
            }
        }
        else
        {
            printk("ext2_os2_normalize_path - could not thunk pArgdat - rc = %d", rc);
        }
    }
    else
    {
        printk("ext2_os2_normalize_path - not a path directed call");
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}

/*
 * vfs_link entry point. Implements the link() system call (creates a hard link).
 */
int ext2_os2_link(struct fs32_fsctl_parms *parms)
{
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    char *pData;
    unsigned short *plenDataOut;
    union argdat32 *pArgdat;
    int rc;
    int rc2;
    int len;
    char *pPath;
    char *src;
    char *dst;
    struct super_block *sb;
    struct cdfsi32 *pcdfsi;

    if (parms->iArgType == FSCTL_ARG_CURDIR)
    {
        if ((rc = DevHlp32_VirtToLin(parms->pArgdat, __StackToFlat(&pArgdat))) == NO_ERROR)
        {
            if ((rc = DevHlp32_VirtToLin(pArgdat->cd.pPath, __StackToFlat(&pPath))) == NO_ERROR)
            {
                printk("ext2_os2_link - src is %s", pPath);
                if ((rc = DevHlp32_VirtToLin(pArgdat->cd.pcdfsi, __StackToFlat(&pcdfsi))) == NO_ERROR)
                {
                    sb = getvolume(pcdfsi->cdi_hVPB);
                    if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
                    {
                        if (parms->lenData)
                        {
                            if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                            {
                                if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
                                {
                                    rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY, pData, parms->lenData, (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));
                                    if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                    {
                                        printk("ext2_os2_link - dst is %s", pPath);
                                        rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                                        if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                        {
                                            /*
                                             * Skip drive letter for source
                                             */
                                            src = pPath;
                                            if (src[0] &&
                                                (((src[0] >= 'A') && (src[0] <= 'Z')) ||
                                                 ((src[0] >= 'a') && (src[0] <= 'z'))) &&
                                                src[1] &&
                                                (src[1] == ':'))
                                            {
                                                /*
                                                 * Skip drive letter for target
                                                 */
                                                dst = pData;
                                                if (dst[0] &&
                                                    (((dst[0] >= 'A') && (dst[0] <= 'Z')) ||
                                                     ((dst[0] >= 'a') && (dst[0] <= 'z'))) &&
                                                    dst[1] &&
                                                    (dst[1] == ':'))
                                                {
                                                    if (toupper(src[0]) == toupper(dst[0]))
                                                    {
                                                        /*
                                                         * Do the actual hard link operation
                                                         */
                                                        rc = sys_link(sb->s_mounted, src + 2, dst + 2);
                                                        rc = map_err(rc);
                                                    }
                                                    else
                                                    {
                                                        printk("ext2_os2_link - src not on same file system as dst");
                                                        rc = ERROR_INVALID_PARAMETER;
                                                    }
                                                }
                                                else
                                                {
                                                    printk("ext2_os2_link - dst without drive letter");
                                                    rc = ERROR_INVALID_PARAMETER;
                                                }

                                            }
                                            else
                                            {
                                                printk("ext2_os2_link - src without drive letter");
                                                rc = ERROR_INVALID_PARAMETER;
                                            }
                                            if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                                            {
                                                /*
                                                 * Nothing else
                                                 */
                                            }
                                            else
                                            {
                                                printk("ext2_os2_link - could not unlock plenDataOut - rc = %d", rc2);
                                                rc = rc2;
                                            }
                                        }
                                        else
                                        {
                                            printk("ext2_os2_link - could not lock plenDataOut - rc = %d", rc);
                                        }
                                        if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                                        {
                                            /*
                                             * Nothing else
                                             */
                                        }
                                        else
                                        {
                                            printk("ext2_os2_link - could not unlock pData - rc = %d", rc2);
                                            rc = rc2;
                                        }
                                    }
                                    else
                                    {
                                        printk("ext2_os2_link - could not lock pData - rc = %d", rc);
                                    }
                                }
                                else
                                {
                                    printk("ext2_os2_link - could not thunk plenDataOut - rc = %d", rc);
                                }
                            }
                            else
                            {
                                printk("ext2_os2_link - could not thunk pData - rc = %d", rc);
                            }
                        }
                        else
                        {
                            printk("ext2_os2_link - data buffer too small");
                            rc = ERROR_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        printk("ext2_os2_link - could not retrieve superblock");
                        rc = ERROR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    printk("ext2_os2_link - could not thunk pcdfsi - rc = %d", rc);
                }
            }
            else
            {
                printk("ext2_os2_link - could not thunk pPath - rc = %d", rc);
            }
        }
        else
        {
            printk("ext2_os2_link - could not thunk pArgdat - rc = %d", rc);
        }
    }
    else
    {
        printk("ext2_os2_link - not a path directed call");
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}




/*
 * Linux VFS ioctl entry point.
 */
int ext2_os2_ioctl(struct fs32_fsctl_parms *parms, int ioctl_func, int size, int rw)
{
    char lock_1[12];
    char lock_2[12];
    unsigned long PgCount;
    union argdat32 *pArgdat;
    struct sffsi32 *psffsi;
    union sffsd32 *psffsd;
    char *pData;
    unsigned short *plenDataOut;
    int rc, rc2;
    int lockflags;
    struct file *filp;

    if (parms->iArgType == FSCTL_ARG_FILEINSTANCE)
    {
        if (parms->lenData >= size)
        {
            pArgdat = VDHQueryLin(parms->pArgdat);
            psffsi = VDHQueryLin(pArgdat->sf.psffsi);
            psffsd = VDHQueryLin(pArgdat->sf.psffsd);
            if ((psffsd->f) && (psffsd->f->f_magic == FILE_MAGIC))
            {
                if (psffsd->f->f_inode)
                {
                    if ((rc = DevHlp32_VirtToLin(parms->pData, __StackToFlat(&pData))) == NO_ERROR)
                    {
                        if ((rc = DevHlp32_VirtToLin(parms->plenDataOut, __StackToFlat(&plenDataOut))) == NO_ERROR)
                        {
                            lockflags = (rw ? VMDHL_WRITE | VMDHL_LONG | VMDHL_VERIFY : VMDHL_LONG | VMDHL_VERIFY);
                            rc = DevHlp32_VMLock(lockflags, pData, size, (void *)-1, __StackToFlat(lock_1), __StackToFlat(&PgCount));
                            if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                            {
                                rc = DevHlp32_VMLock(VMDHL_LONG | VMDHL_VERIFY | VMDHL_WRITE, plenDataOut, sizeof(short), (void *)-1, __StackToFlat(lock_2), __StackToFlat(&PgCount));

                                if ((rc == NO_ERROR) || (rc == ERROR_NOBLOCK))
                                {

                                    if (psffsd->f->f_op && psffsd->f->f_op->ioctl)
                                    {
                                        rc = psffsd->f->f_op->ioctl(psffsd->f->f_inode, psffsd->f, ioctl_func, (unsigned long)pData);
                                        rc = map_err(rc);
                                        if (rw)
                                            *plenDataOut = size;
                                    }
                                    else
                                    {
                                        rc = ERROR_NOT_SUPPORTED;
                                    }

                                    if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_2))) == NO_ERROR)
                                    {
                                        /*
                                         * Nothing else
                                         */
                                    }
                                    else
                                    {
                                        rc = rc2;
                                    }
                                }
                                if ((rc2 = DevHlp32_VMUnlock(__StackToFlat(lock_1))) == NO_ERROR)
                                {
                                    /*
                                     * Nothing else
                                     */
                                }
                                else
                                {
                                    rc = rc2;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            rc = ERROR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}

extern void _Optlink fs32_init_CheckRing0Initialize(void);

/*
 * struct fs32_fsctl_parms {
 *     PTR16          plenDataOut;
 *     unsigned short lenData;
 *     PTR16          pData;
 *     PTR16          plenParmOut;
 *     unsigned short lenParm;
 *     PTR16          pParm;
 *     unsigned short func;
 *     unsigned short iArgType;
 *     PTR16          pArgdat;
 * };
 */

int FS32ENTRY fs32_fsctl(struct fs32_fsctl_parms *parms)
{
    int rc;

    // V0.9.12 (2001-05-03) [umoeller]
    fs32_init_CheckRing0Initialize();

    switch (parms->func)
    {
        case FSCTL_FUNC_NEW_INFO:
            rc = fsctl_func_new_info(parms);
            break;
        case IFSDBG_OPEN:
            rc = ifsdbg_open(parms);
            break;
        case IFSDBG_CLOSE:
            rc = ifsdbg_close(parms);
            break;
        case IFSDBG_READ:
            rc = ifsdbg_read(parms);
            break;
        case EXT2_OS2_BDFLUSH:
            rc = ext2_os2_bdflush(parms);
            break;
        case EXT2_OS2_SHRINK_CACHE:
            rc = ext2_os2_shrink_cache(parms);
            break;
        case EXT2_OS2_GETDATA:
            rc = ext2_os2_getdata(parms);
            break;
        case EXT2_OS2_SYNC:
            rc = ext2_os2_sync(parms);
            break;
        case EXT2_OS2_STAT:
            rc = ext2_os2_stat(parms);
            break;
        case EXT2_OS2_FSTAT:
            rc = ext2_os2_fstat(parms);
            break;
        case EXT2_OS2_LINK:
            rc = ext2_os2_link(parms);
            break;
        case EXT2_OS2_NORMALIZE_PATH:
            rc = ext2_os2_normalize_path(parms);
            break;

            /*
             * Linux ext2 fs ioctl
             */
        case EXT2_OS2_IOCTL_EXT2_IOC_GETFLAGS:
            rc = ext2_os2_ioctl(parms, EXT2_IOC_GETFLAGS, sizeof(int), 0);

            break;

        case EXT2_OS2_IOCTL_EXT2_IOC_SETFLAGS:
            rc = ext2_os2_ioctl(parms, EXT2_IOC_SETFLAGS, sizeof(int), 1);

            break;

        case EXT2_OS2_IOCTL_EXT2_IOC_GETVERSION:
            rc = ext2_os2_ioctl(parms, EXT2_IOC_GETVERSION, sizeof(int), 0);

            break;

        case EXT2_OS2_IOCTL_EXT2_IOC_SETVERSION:
            rc = ext2_os2_ioctl(parms, EXT2_IOC_SETVERSION, sizeof(int), 1);

            break;


        case EXT2_OS2_SHOW_BUFFERS:
            show_buffers();
            rc = NO_ERROR;
            break;
        case EXT2_OS2_DUMP_DIRTY_BUFFERS:
            dump_dirty_buffers();
            rc = NO_ERROR;
            break;

        default:
            rc = ERROR_NOT_SUPPORTED;
            break;
    }
    return rc;
}
