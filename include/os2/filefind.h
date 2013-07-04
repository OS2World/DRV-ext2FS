//
// $Header: e:\\netlabs.cvs\\ext2fs/include/os2/filefind.h,v 1.1 2001/05/09 17:43:40 umoeller Exp $
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



#ifndef __filefind_h
#define __filefind_h

#include <os2\types.h>
#include <linux\fs.h>

#pragma pack(1)
typedef struct {
      unsigned short dateCreate;
      unsigned short timeCreate;
      unsigned short dateAccess;
      unsigned short timeAccess;
      unsigned short dateWrite;
      unsigned short timeWrite;
      long           cbEOF;
      long           cbAlloc;
      unsigned short attr;
} CommonFileInfo;


typedef CommonFileInfo *pCommonFileInfo;

typedef struct {
      unsigned char  cbName;      /* name length WITHOUT \0 */
      unsigned char  szName[1];   /* name INCLUDING \0      */
} FileName;

typedef FileName *pFileName;
#pragma pack()


/***************************************/
/*** structure de findfirst/findnext ***/
/*** MUST BE <= 24 BYTES             ***/
/***************************************/
typedef struct {
    unsigned long  last;
    struct file   *p_file;
    char          *pName;                            /* Nom de recherche                    */
//  pchar pNom       = pName + 1 * CCHMAXPATH;       /* R‚pertoire de base de la recherche  */
//  pchar pWork      = pName + 2 * CCHMAXPATH;       /* Nom de travail : fichier lu         */
//  pchar pNomUpper  = pName + 3 * CCHMAXPATH;
//  pchar pWorkUpper = pName + 4 * CCHMAXPATH;
    UINT16 attr;
//    int FS_CLOSEd;
} h_find;

typedef h_find *p_hfind;


#define TYPEOP_FILEFIND 1
#define TYPEOP_FILEINFO 2

int fileinfo_to_ino(char *databuf, struct inode *ino, int level, unsigned short datalen, struct sffsi32 *psffsi);

int ino_to_fileinfo(
                    struct inode * pino,
                    char * databuf,
                    UINT32 maxlen,
                    PUINT32 plen,
                    unsigned short level,
                    unsigned short flags,
                    unsigned short attrs,
                    struct dirent *pDir,
                    INT32 position, int TypeOp);

int myfindnext(
             struct super_block    *p_volume,
             struct file           *p_file,
             unsigned short         attr,
             struct fsfsi32   * pfsfsi,
             struct fsfsd32   * pfsfsd,
             char *                  pData,
             unsigned short         cbData,
             unsigned short * pcMatch,
             unsigned short         level,
             unsigned short         flags,
             UINT32                 index_dir,
             int                    is_findfirst,
             int                    caseRetensive
            );

#endif
