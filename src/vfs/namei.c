//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/namei.c,v 1.1 2001/05/09 17:48:07 umoeller Exp $
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
#include <os2.h>        // From the "Developer Connection Device Driver Kit" version 2.0

#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\DevHlp32.h>
#include <os2\os2proto.h>
#include <os2\errors.h>

#include <linux\errno.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <os2\log.h>


/*
 * lookup() looks up one part of a pathname, using the fs-dependent
 * routines (currently minix_lookup) for it. It also checks for
 * fathers (pseudo-roots, mount-points)
 */
int lookup(struct inode * dir,const char * name, int len,
    struct inode ** result)
{
    struct super_block * sb;
    int perm;

    *result = NULL;
    if (!dir)
        return -ENOENT;
#ifndef OS2
/* check permissions before traversing mount-points */
    perm = permission(dir,MAY_EXEC);
#else
        perm = 0;
#endif
    if (len==2 && name[0] == '.' && name[1] == '.') {
#ifndef OS2
        if (dir == current->fs->root) {
            *result = dir;
            return 0;
        } else if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {
            sb = dir->i_sb;
            iput(dir);
            dir = sb->s_covered;
            if (!dir)
                return -ENOENT;
            dir->i_count++;
        }
#else
        if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {
            *result = dir;
            return 0;
                }
#endif
    }
    if (!dir->i_op || !dir->i_op->lookup) {
        iput(dir);
        return -ENOTDIR;
    }
    if (perm != 0) {
        iput(dir);
        return perm;
    }
    if (!len) {
        *result = dir;
        return 0;
    }
    return dir->i_op->lookup(dir,name,len,result);
}

int follow_link(struct inode * dir, struct inode * inode,
    int flag, int mode, struct inode ** res_inode)
{
    if (!dir || !inode) {
        iput(dir);
        iput(inode);
        *res_inode = NULL;
        return -ENOENT;
    }
    if (!inode->i_op || !inode->i_op->follow_link) {
        iput(dir);
        *res_inode = inode;
        return 0;
    }
    return inode->i_op->follow_link(dir,inode,flag,mode,res_inode);
}

/*
 *  dir_namei()
 *
 * dir_namei() returns the inode of the directory of the
 * specified name, and the name within that directory.
 */
#ifndef OS2
static int dir_namei(const char * pathname, int * namelen, const char ** name,
#else
       int dir_namei(const char * pathname, int * namelen, const char ** name,
#endif
    struct inode * base, struct inode ** res_inode)
{
    char c;
    const char * thisname;
    int len,error;
    struct inode * inode;

    *res_inode = NULL;
#ifndef OS2
    if (!base) {
        base = current->fs->pwd;
        base->i_count++;
    }
    if ((c = *pathname) == '/') {
        iput(base);
        base = current->fs->root;
        pathname++;
        base->i_count++;
    }
#endif
    while (1) {
        thisname = pathname;
#ifndef OS2
        for(len=0;(c = *(pathname++))&&(c != '/');len++)
#else
        for(len=0;(c = *(pathname++))&&(c != '/')&&(c != '\\');len++)
#endif
            /* nothing */ ;
        if (!c)
            break;
        base->i_count++;
#ifndef OS2
        error = lookup(base,thisname,len,&inode);
#else
        error = lookup(base,thisname,len,__StackToFlat(&inode));
#endif
        if (error) {
            iput(base);
            return error;
        }
        error = follow_link(base,inode,0,0,__StackToFlat(&base));
        if (error)
            return error;
    }
    if (!base->i_op || !base->i_op->lookup) {
        iput(base);
        return -ENOTDIR;
    }
    *name = thisname;
    *namelen = len;
    *res_inode = base;
    return 0;
}


static int do_link(struct inode * oldinode, const char * newname, struct inode *base)
{
    struct inode * dir;
    const char * basename;
    int namelen, error;

    base->i_count++;    /* dir_namei uses up base */
    error = dir_namei(newname,__StackToFlat(&namelen),__StackToFlat(&basename),base,__StackToFlat(&dir));
    if (error) {
        iput(oldinode);
        return error;
    }
    if (!namelen) {
        iput(oldinode);
        iput(dir);
        return -EPERM;
    }
    if (IS_RDONLY(dir)) {
        iput(oldinode);
        iput(dir);
        return -EROFS;
    }
    if (dir->i_dev != oldinode->i_dev) {
        iput(dir);
        iput(oldinode);
        return -EXDEV;
    }
#ifndef OS2
    if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
        iput(dir);
        iput(oldinode);
        return error;
    }
#endif
    /*
     * A link to an append-only or immutable file cannot be created
     */
    if (IS_APPEND(oldinode) || IS_IMMUTABLE(oldinode)) {
        iput(dir);
        iput(oldinode);
        return -EPERM;
    }
    if (!dir->i_op || !dir->i_op->link) {
        iput(dir);
        iput(oldinode);
        return -EPERM;
    }
    dir->i_count++;
    down(&dir->i_sem);
    error = dir->i_op->link(oldinode, dir, basename, namelen);
    up(&dir->i_sem);
    iput(dir);
    return error;
}

int do_unlink(struct inode *base, const char * name)
{
    const char * basename;
    int namelen, error;
    struct inode * dir;

        base->i_count ++;
    error = dir_namei(name,__StackToFlat(&namelen),__StackToFlat(&basename),base,__StackToFlat(&dir));
    if (error)
        return error;
    if (!namelen) {
        iput(dir);
        return -EPERM;
    }
    if (IS_RDONLY(dir)) {
        iput(dir);
        return -EROFS;
    }
#ifndef OS2
    if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
        iput(dir);
        return error;
    }
#endif
    /*
     * A file cannot be removed from an append-only directory
     */
    if (IS_APPEND(dir)) {
        iput(dir);
        return -EPERM;
    }
    if (!dir->i_op || !dir->i_op->unlink) {
        iput(dir);
        return -EPERM;
    }
    return dir->i_op->unlink(dir,basename,namelen);
}

int do_rmdir(struct inode *base, const char * name)
{
    const char * basename;
    int namelen, error;
    struct inode * dir;

        base->i_count ++;
    error = dir_namei(name,__StackToFlat(&namelen),__StackToFlat(&basename),base,__StackToFlat(&dir));
    if (error)
        return error;
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }
    if (IS_RDONLY(dir)) {
        iput(dir);
        return -EROFS;
    }
#ifndef OS2
    if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
        iput(dir);
        return error;
    }
#endif
    /*
     * A subdirectory cannot be removed from an append-only directory
     */
    if (IS_APPEND(dir)) {
        iput(dir);
        return -EPERM;
    }
    if (!dir->i_op || !dir->i_op->rmdir) {
        iput(dir);
        return -EPERM;
    }
    return dir->i_op->rmdir(dir,basename,namelen);
}

static int _namei(const char * pathname, struct inode * base,
    int follow_links, struct inode ** res_inode)
{
    const char * basename;
    int namelen,error;
    struct inode * inode;

    *res_inode = NULL;
    base->i_count++;    /* dir_namei uses up base */
    error = dir_namei(pathname,__StackToFlat(&namelen),__StackToFlat(&basename),base, __StackToFlat(&base));
    if (error)
        return error;
    base->i_count++;    /* lookup uses up base */
    error = lookup(base,basename,namelen,__StackToFlat(&inode));
    if (error) {
        iput(base);
        return error;
    }
    if (follow_links) {
        error = follow_link(base,inode,0,0,__StackToFlat(&inode));
        if (error)
            return error;
    } else
        iput(base);
    *res_inode = inode;
    return 0;
}

int do_mkdir(struct inode *base, const char * pathname, int mode)
{
    const char * basename;
    int namelen, error;
    struct inode * dir;

        base->i_count ++;
    error = dir_namei(pathname,__StackToFlat(&namelen),__StackToFlat(&basename),base,__StackToFlat(&dir));
    if (error)
        return error;
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }
    if (IS_RDONLY(dir)) {
        iput(dir);
        return -EROFS;
    }
#ifndef OS2
    if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
        iput(dir);
        return error;
    }
#endif
    if (!dir->i_op || !dir->i_op->mkdir) {
        iput(dir);
        return -EPERM;
    }
    dir->i_count++;
    down(&dir->i_sem);
#ifndef OS2
    error = dir->i_op->mkdir(dir, basename, namelen, mode & 0777 & ~current->fs->umask);
#else
    error = dir->i_op->mkdir(dir, basename, namelen, mode & 0777);
#endif
    up(&dir->i_sem);
    iput(dir);
    return error;
}

/*
 *  namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 */
#ifndef OS2
int namei(const char * pathname, struct inode ** res_inode)
{
    int error;
    char * tmp;

    error = getname(pathname,&tmp);
    if (!error) {

        error = _namei(tmp,NULL,1,res_inode);
        putname(tmp);
    }
    return error;
}
#else
int namei(struct inode *base, const char * pathname, struct inode ** res_inode)
{
    return _namei(pathname,base,1,res_inode);
}
#endif

#ifndef OS2
asmlinkage int sys_link(const char * oldname, const char * newname)
#else
asmlinkage int sys_link(struct inode *base, const char * oldname, const char * newname)
#endif
{
    int error;
    char * to;
    struct inode * oldinode;

    error = namei(base, oldname, __StackToFlat(&oldinode));
    if (error)
        return error;
#ifndef OS2
    error = getname(newname,&to);
    if (error) {
        iput(oldinode);
        return error;
    }
    error = do_link(oldinode,to);
    putname(to);
#else
    error = do_link(oldinode,newname,base);
#endif
    return error;
}

#ifndef OS2
static int do_rename(const char * oldname, const char * newname)
#else
int do_rename(struct inode *srcbase, struct inode *dstbase, const char * oldname, const char * newname)
#endif
{
    struct inode * old_dir, * new_dir;
    const char * old_base, * new_base;
    int old_len, new_len, error;

    error = dir_namei(oldname,__StackToFlat(&old_len),__StackToFlat(&old_base),srcbase,__StackToFlat(&old_dir));
    if (error)
        return error;
#ifndef OS2
    if ((error = permission(old_dir,MAY_WRITE | MAY_EXEC)) != 0) {
        iput(old_dir);
        return error;
    }
#endif
    if (!old_len || (old_base[0] == '.' &&
        (old_len == 1 || (old_base[1] == '.' &&
         old_len == 2)))) {
        iput(old_dir);
        return -EPERM;
    }
    error = dir_namei(newname,__StackToFlat(&new_len),__StackToFlat(&new_base),dstbase,__StackToFlat(&new_dir));
    if (error) {
        iput(old_dir);
        return error;
    }
#ifndef OS2
    if ((error = permission(new_dir,MAY_WRITE | MAY_EXEC)) != 0){
        iput(old_dir);
        iput(new_dir);
        return error;
    }
#endif
    if (!new_len || (new_base[0] == '.' &&
        (new_len == 1 || (new_base[1] == '.' &&
         new_len == 2)))) {
        iput(old_dir);
        iput(new_dir);
        return -EPERM;
    }
    if (new_dir->i_dev != old_dir->i_dev) {
        iput(old_dir);
        iput(new_dir);
        return -EXDEV;
    }
    if (IS_RDONLY(new_dir) || IS_RDONLY(old_dir)) {
        iput(old_dir);
        iput(new_dir);
        return -EROFS;
    }
    /*
     * A file cannot be removed from an append-only directory
     */
    if (IS_APPEND(old_dir)) {
        iput(old_dir);
        iput(new_dir);
        return -EPERM;
    }
    if (!old_dir->i_op || !old_dir->i_op->rename) {
        iput(old_dir);
        iput(new_dir);
        return -EPERM;
    }
    new_dir->i_count++;
    down(&new_dir->i_sem);
    error = old_dir->i_op->rename(old_dir, old_base, old_len,
        new_dir, new_base, new_len);
    up(&new_dir->i_sem);
    iput(new_dir);
    return error;
}
