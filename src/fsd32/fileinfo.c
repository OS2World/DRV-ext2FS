
/*
 *@@sourcefile fileinfo.c:
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
#include <os2\filefind.h>
#include <os2\fnmatch.h>

#define THISFILE FILE_TEST_C    // obsolete

/* 1 - means OK, 0 means doesn't match */
int filter_with_attrs(struct inode *pino, unsigned short attrs, char *name)
{
    unsigned short mayhave = attrs & 0xFF;
    unsigned short musthave = (attrs >> 8) & 0xFF;
    unsigned short mapped;

    mapped = Linux_To_DOS_Attrs(pino, name);


/*
 * Actual   May have  Must have  Included   a&may&must ~a&~may&~must a&may&~must  ~a&may&~must
 * 1          1          1         y           1           0          0               0
 * 1          1          0         y           0           0          1               0
 * 1          0          1         n           0           0          0               0
 * 1          0          0         n           0           0          0               0
 * 0          1          1         n           0           0          0               0
 * 0          1          0         y           0           0          0               1
 * 0          0          1         n           0           0          0               0
 * 0          0          0         y           0           1          0               0
 *
 * => a & may | ~a & ~must
 *
 */

    return !(~(((mapped & mayhave) | ((~mapped) & (~musthave)))));
}

int ino_to_fileinfo(
                       struct inode *pino,
                       char *databuf,
                       UINT32 maxlen,
                       PUINT32 plen,
                       unsigned short level,
                       unsigned short flags,
                       unsigned short attrs,
                       struct dirent *pDir,
                       INT32 position, int TypeOp)
{
    pCommonFileInfo pCommon;
    pFileName pFile;
    PINT32 pPosition;

    *plen = 0;

/***************************************************************************/
/*** First field : long position if FF_GETPOS - nothing if FF_NOPOS      ***/
/***************************************************************************/
    if (TypeOp == TYPEOP_FILEFIND)
    {
        if (flags == FF_GETPOS)
        {
            if (sizeof(INT32) + *plen > maxlen)
            {
                fs_log("ino_to_fileinfo() - Buffer overflow for first field");
                return ERROR_BUFFER_OVERFLOW;
            }
            pPosition = (PINT32) (databuf + *plen);
            *pPosition = position;
            *plen += sizeof(INT32);
        }
    }
/***************************************************************************/

/***************************************************************************/
/*** Second field : common file information to all level/flags           ***/
/***************************************************************************/
    if (sizeof(CommonFileInfo) + *plen > maxlen)
    {
        kernel_printf("ino_to_fileinfo() - Buffer overflow for second field - len = %lu - maxlen = %lu", (UINT32) sizeof(CommonFileInfo) + (UINT32) (*plen), (UINT32) maxlen);
        return ERROR_BUFFER_OVERFLOW;
    }
    pCommon = (pCommonFileInfo) (databuf + *plen);

    date_unix2dos(pino->i_ctime, &(pCommon->timeCreate), &(pCommon->dateCreate));
    date_unix2dos(pino->i_atime, &(pCommon->timeAccess), &(pCommon->dateAccess));
    date_unix2dos(pino->i_mtime, &(pCommon->timeWrite), &(pCommon->dateWrite));
    pCommon->cbEOF = pino->i_size;
    pCommon->cbAlloc = pino->i_blocks;
    pCommon->attr = FILE_NORMAL;

    if ((S_ISLNK(pino->i_mode)) ||
        (S_ISBLK(pino->i_mode)) ||
        (S_ISCHR(pino->i_mode)) ||
        (S_ISFIFO(pino->i_mode)) ||
        (S_ISSOCK(pino->i_mode)))
    {
        pCommon->attr |= FILE_SYSTEM;
/*** UNIXish special files are seen as SYSTEM files ***/
    }                           /* endif */

    if (S_ISDIR(pino->i_mode))
    {
        pCommon->attr |= FILE_DIRECTORY;
    }                           /* endif */

    if ((!(pino->i_mode & S_IWUSR)) &&
        (!(pino->i_mode & S_IWGRP)) &&
        (!(pino->i_mode & S_IWOTH)))
    {
        pCommon->attr |= FILE_READONLY;
    }
    *plen += sizeof(CommonFileInfo);
/***************************************************************************/

/***************************************************************************/
/*** Third field : nothing for level FIL_STANDARD                        ***/
/***************************************************************************/
/***************************************************************************/

/***************************************************************************/
/*** Third field : Size on disk of EA set for FIL_QUERYEASIZE            ***/
/***************************************************************************/
    if (level == FIL_QUERYEASIZE)
    {
        if (sizeof(INT32) + *plen > maxlen)
        {
            fs_log("ino_to_fileinfo() - Buffer overflow for Third field - FIL_QUERYEASIZE");
            return ERROR_BUFFER_OVERFLOW;
        }
        *((PINT32) (databuf + *plen)) = 0;
/*** No EAS in ext2 ***/
        *plen += sizeof(INT32);
    }
/***************************************************************************/

/***************************************************************************/
/*** Third field : FEAList for level QUERYEASFROMLIST                    ***/
/***************************************************************************/
    if (level == FIL_QUERYEASFROMLIST)
    {
        if (sizeof(INT32) + *plen > maxlen)
        {
            fs_log("ino_to_fileinfo() - Buffer overflow for Third field - FIL_QUERYEASFROMLIST");
            return ERROR_BUFFER_OVERFLOW;
        }
        *((PINT32) (databuf + *plen)) = 4;  /*** No EAS in ext2 ***//* this is the length field of FEAList */
        *plen += sizeof(INT32);
    }
/***************************************************************************/

/***************************************************************************/
/*** Fourth field : name                                                 ***/
/***************************************************************************/
    if (TypeOp == TYPEOP_FILEFIND)
    {
        if (*plen + sizeof(FileName) + pDir->d_reclen > maxlen)
        {
            fs_log("ino_to_fileinfo() - Buffer overflow for fourth field");
            return ERROR_BUFFER_OVERFLOW;
        }
        pFile = (pFileName) (databuf + *plen);
        pFile->cbName = (unsigned char)pDir->d_reclen;  /* name length WITHOUT '\0' */
        strcpy(pFile->szName, pDir->d_name);    /* name WITH '\0'           */
        *plen += sizeof(FileName) + pDir->d_reclen;     /* sizeof(FileName) + strlen(Dir.d_name) */
    }
/***************************************************************************/

    return NO_ERROR;
}

int fileinfo_to_ino(char *databuf, struct inode *ino, int level, unsigned short datalen, struct sffsi32 *psffsi)
{
    int rc;
    pCommonFileInfo pCommon;
    unsigned short time;
    unsigned short date;

    rc = NO_ERROR;

    /*
     * Only FIL_STANDARD is significant. FIL_QUERYEASIZE is not because one can't SET EAS in an ext2
     * file system.
     */
    if (level == FIL_STANDARD)
    {
        if (datalen >= sizeof(pCommonFileInfo))
        {
            pCommon = (pCommonFileInfo) databuf;


            /*
             * Creation date
             */
            date_unix2dos(ino->i_ctime, __StackToFlat(&time), __StackToFlat(&date));
            if (pCommon->timeCreate)
                time = pCommon->timeCreate;
            if (pCommon->dateCreate)
                date = pCommon->dateCreate;
            ino->i_ctime = date_dos2unix(time, date);
            if (psffsi)
            {
                psffsi->sfi_cdate = date;
                psffsi->sfi_ctime = time;
                if ((pCommon->timeCreate) || (pCommon->dateCreate))
                {
                    /*
                     * Clears the stamp creation date bit if we've just modified the creation date.
                     */
                    psffsi->sfi_tstamp &= ~ST_SCREAT;   // disable kernel time stamp

                    psffsi->sfi_tstamp |= ST_PCREAT;    // propagate new value to other sft

                }
            }

            /*
             * Last write date
             */
            date_unix2dos(ino->i_mtime, __StackToFlat(&time), __StackToFlat(&date));
            if (pCommon->timeWrite)
                time = pCommon->timeWrite;
            if (pCommon->dateWrite)
                date = pCommon->dateWrite;
            ino->i_mtime = date_dos2unix(time, date);
            if (psffsi)
            {
                psffsi->sfi_mdate = date;
                psffsi->sfi_mtime = time;
                if ((pCommon->timeWrite) || (pCommon->dateWrite))
                {
                    /*
                     * Clears the stamp last write date bit if we've just modified the last write date.
                     */
                    psffsi->sfi_tstamp &= ~ST_SWRITE;   // disable kernel time stamp

                    psffsi->sfi_tstamp |= ST_PWRITE;    // propagate new value to other sft

                }
            }

            /*
             * Last access date
             */
            date_unix2dos(ino->i_atime, __StackToFlat(&time), __StackToFlat(&date));
            if (pCommon->timeAccess)
                time = pCommon->timeAccess;
            if (pCommon->dateAccess)
                date = pCommon->dateAccess;
            ino->i_atime = date_dos2unix(time, date);
            if (psffsi)
            {
                psffsi->sfi_adate = date;
                psffsi->sfi_atime = time;
                if ((pCommon->timeAccess) || (pCommon->dateAccess))
                {
                    /*
                     * Clears the stamp last access date bit if we've just modified the last access date.
                     */
                    psffsi->sfi_tstamp &= ~ST_SREAD;    // disable kernel time stamp

                    psffsi->sfi_tstamp |= ST_PREAD;     // propagate new value to other sft

                }
            }

            /*
             * File size - neither the IFS document nor the Control Program Reference
             *             are clear about if we must change the file size when DosSetPathInfo
             *             or DosSetFIleInfo is called. HPFS does NOT change it .... so we don't
             *             change it also.
             */

            /*
             * File attributes
             */
            DOS_To_Linux_Attrs(ino, pCommon->attr);

            /*
             * Marks the inode as dirty
             */
            ino->i_dirt = 1;

        }
        else
        {
            rc = ERROR_BUFFER_OVERFLOW;
        }
    }
    return rc;
}

int myfindnext(
                  struct super_block *p_volume,
                  struct file *p_file,
                  unsigned short attr,
                  struct fsfsi32 *pfsfsi,
                  struct fsfsd32 *pfsfsd,
                  char *pData,
                  unsigned short cbData,
                  unsigned short *pcMatch,
                  unsigned short level,
                  unsigned short flags,
                  UINT32 index_dir,
                  int is_findfirst,
                  int caseRetensive
)
{
    char *left;
    char *right;
    UINT32 cur_size;
    int rc;
    char *pName;
    UINT32 len;
    struct inode *ino;
    UINT16 nb_found;
    struct dirent Dir;
    int fn_flag;

    pName = ((p_hfind) pfsfsd)->pName;

    nb_found = 0;
    if (level == FIL_QUERYEASFROMLIST)
    {
        cur_size = sizeof(EAOP);
    }
    else
    {
        cur_size = 0;
    }

    while (nb_found < *pcMatch)
    {
/***********************************************************/
/*** If we are at the end of the parent dir : stop       ***/
/***********************************************************/
        if (VFS_readdir(p_file, __StackToFlat(&Dir)) != NO_ERROR)
        {
            ((p_hfind) pfsfsd)->last = index_dir;
            *pcMatch = nb_found;

            if (nb_found > 0)
            {
#ifdef FS_TRACE
                kernel_printf("myfindnext()  - EOD - found %d entries", nb_found);
#endif
                return NO_ERROR;
            }
            else
            {

#if 1
                if (is_findfirst)
                {

/************************************************************************/
/*** Lib‚ration du buffer stockant les noms de recherche              ***/
/************************************************************************/
                    if ((rc = DevHlp32_VMFree(((p_hfind) pfsfsd)->pName)) != NO_ERROR)
                    {
                        fs_err(FUNC_FS_FINDFIRST, FUNC_G_free, rc, THISFILE, __LINE__);
                        return rc;
                    }           /* end if */
/************************************************************************/

                    if ((rc = vfs_close(p_file)) != NO_ERROR)
                    {
                        fs_err(FUNC_FS_FINDFIRST, FUNC_CLOSE, rc, THISFILE, __LINE__);
                        return rc;
                    }
//                    ((p_hfind)pfsfsd)->FS_CLOSEd = SEARCH_ALREADY_CLOSED;
                }
#else
                FS_FINDCLOSE(pfsfsi, pfsfsd);
                ((p_hfind) pfsfsd)->FS_CLOSEd = SEARCH_ALREADY_CLOSED;
#endif
                return ERROR_NO_MORE_FILES;
            }
        }                       /* end if */
/***********************************************************/

/***********************************************************/
/*** If we found an entry, see if it matches             ***/
/***********************************************************/
        if (caseRetensive)
        {
            fn_flag = _FNM_OS2 | _FNM_IGNORECASE;
        }
        else
        {
            fn_flag = _FNM_OS2;
        }
        if (fnmatch(pName, __StackToFlat(Dir.d_name), fn_flag) == 0)
        {
            if (p_file->f_inode->i_ino != Dir.d_ino)
            {
                if ((ino = iget(p_volume, Dir.d_ino)) == NULL)
                {
                    fs_err(FUNC_FS_FINDFIRST, FUNC_GET_VINODE, rc, THISFILE, __LINE__);
                    return ERROR_NO_MORE_FILES;
                }               /* endif */
            }
            else
            {
                ino = p_file->f_inode;
            }
            if (filter_with_attrs(ino, attr | FILE_READONLY, __StackToFlat(Dir.d_name)))
            {
                if ((rc = ino_to_fileinfo(
                                             ino,
                                             pData + cur_size,
                                             cbData - cur_size,
                                             __StackToFlat(&len),
                                             level,
                                             flags,
                                             attr,
                                             __StackToFlat(&Dir),
                                     index_dir, TYPEOP_FILEFIND)) != NO_ERROR)
                {
                    *pcMatch = nb_found;
                    ((p_hfind) pfsfsd)->last = index_dir;
                    fs_log("====> FS_FINDFIRST : erreur ino_to_fileinfo()");
                    if (p_file->f_inode->i_ino != Dir.d_ino)
                    {
                        iput(ino);
                    }
                    return rc;
                }
                nb_found++;
                cur_size += len;
            }
            if (p_file->f_inode->i_ino != Dir.d_ino)
            {
                iput(ino);
            }
        }                       /* end if */
/***********************************************************/

        index_dir++;
    }                           /* end while */

    *pcMatch = nb_found;
    ((p_hfind) pfsfsd)->last = index_dir;
#ifdef FS_TRACE
    kernel_printf("myfindnext() - found %d entries", nb_found);
#endif

    return NO_ERROR;

}
