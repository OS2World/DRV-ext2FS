//
// $Header: e:\\netlabs.cvs\\ext2fs/src/ext2/super.c,v 1.1 2001/05/09 17:53:03 umoeller Exp $
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

/*
 *  linux/fs/ext2/super.c
 *
 *  Copyright (C) 1992, 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                                  Laboratoire MASI - Institut Blaise Pascal
 *                                  Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifdef __IBMC__
    #pragma strings(readonly)
#endif

#ifdef OS2
    #define INCL_DOSERRORS
    #define INCL_NOPMAPI
    #include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0
    #include <stdarg.h>
    #define VA_START(ap, last) ap = ((va_list)__StackToFlat(&last)) + __nextword(last)

    #include <string.h>

    #include <os2\types.h>
    #include <os2\StackToFlat.h>
    #include <linux\fs.h>
    #include <linux\ext2_fs.h>
    #include <linux\fs_proto.h>
    #include <linux\ext2_proto.h>
    #include <os2\os2proto.h>
    #include <linux\errno.h>
    #include <linux\sched.h>
    #include <linux\locks.h>
    #include <os2\log.h>
    #include <os2\fsh32.h>
    #include <os2\ctype.h>
#endif

#ifndef EXT2_DYNAMIC_REV            // defined in linux\ext2fs.h
    #error Blah.
#endif

#ifdef OS2
    #define NORET_TYPE
    #define KERN_CRIT
    #define KERN_WARNING
    #define MAJOR(dev) dev
    #define MINOR(dev) 0
    extern char scratch_buffer[];
    extern int auto_fsck;
    extern char Errors_Panic;
    INLINE unsigned long simple_strtoul(const char *string, char **end_ptr, int radix) {
        return strtoul(string, end_ptr, radix);
    }
    INLINE void kfree_s(void *data, int size) {
        DevHlp32_VMFree(data);
    }
#endif

NORET_TYPE void ext2_panic (struct super_block * sb, const char * function,
                            const char * fmt, ...)
{
#ifndef OS2
        char buf[1024];
#else
        char *buf = scratch_buffer;
#endif
        va_list args;

        if (!(sb->s_flags & MS_RDONLY)) {
                sb->u.ext2_sb.s_mount_state |= EXT2_ERROR_FS;
                sb->u.ext2_sb.s_es->s_state |= EXT2_ERROR_FS;
                mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
                sb->s_dirt = 1;
        }
        VA_START (args, fmt);
        vsprintf (buf, fmt, args);
        va_end (args);
#ifndef OS2
        panic ("EXT2-fs panic (device %d/%d): %s: %s\n",
               MAJOR(sb->s_dev), MINOR(sb->s_dev), function, buf);
#else
        ext2_os2_panic (1, "ext2fs panic (on drive %c:): %s: %s\n",
               sb->s_drive + 'A', function, buf);
#endif
}

void ext2_error (struct super_block * sb, const char * function,
                 const char * fmt, ...)
{
        char __buf[1024];
    char *buf = __StackToFlat(__buf);
        va_list args;

        if (!(sb->s_flags & MS_RDONLY)) {
                sb->u.ext2_sb.s_mount_state |= EXT2_ERROR_FS;
                sb->u.ext2_sb.s_es->s_state |= EXT2_ERROR_FS;
                mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
                sb->s_dirt = 1;
        }
        VA_START (args, fmt);
        vsprintf (buf, fmt, args);
        va_end (args);
        if (test_opt (sb, ERRORS_PANIC) ||
            (sb->u.ext2_sb.s_es->s_errors == EXT2_ERRORS_PANIC &&
             !test_opt (sb, ERRORS_CONT) && !test_opt (sb, ERRORS_RO)))
#ifndef OS2
                panic ("EXT2-fs panic (device %d/%d): %s: %s\n",
                       MAJOR(sb->s_dev), MINOR(sb->s_dev), function, buf);
        printk (KERN_CRIT "EXT2-fs error (device %d/%d): %s: %s\n",
                MAJOR(sb->s_dev), MINOR(sb->s_dev), function, buf);
#else
                ext2_os2_panic (1, "ext2-os2 detected a file system error on drive %c: %s: %s\n",
                       sb->s_drive + 'A', function, buf);
        printk (KERN_CRIT "EXT2-fs error (on drive %c:): %s: %s\n",
                sb->s_drive + 'A', function, buf);
#endif

        if (test_opt (sb, ERRORS_RO) ||
            (sb->u.ext2_sb.s_es->s_errors == EXT2_ERRORS_RO &&
             !test_opt (sb, ERRORS_CONT) && !test_opt (sb, ERRORS_PANIC))) {
                printk ("Remounting filesystem read-only\n");
                sb->s_flags |= MS_RDONLY;
        }
}

void ext2_warning (struct super_block * sb, const char * function,
                   const char * fmt, ...)
{
        char __buf[1024];
    char *buf = __StackToFlat(__buf);
        va_list args;

        VA_START (args, fmt);
        vsprintf (buf, fmt, args);
        va_end (args);
#ifndef OS2
        printk (KERN_WARNING "EXT2-fs warning (device %d/%d): %s: %s\n",
                MAJOR(sb->s_dev), MINOR(sb->s_dev), function, buf);
#else
        printk (KERN_WARNING "EXT2-fs warning (on drive %c:): %s: %s\n",
                sb->s_drive + 'A', function, buf);
#endif
}

#ifndef OS2
static struct super_operations ext2_sops = {
    ext2_read_inode,
    NULL,
    ext2_write_inode,
    ext2_put_inode,
    ext2_put_super,
    ext2_write_super,
    ext2_statfs,
    ext2_remount
};
#else
void ext2_write_super (struct super_block * sb);
int ext2_remount (struct super_block * sb, int * flags, char * data);

static struct super_operations ext2_sops = {
        ext2_read_inode,
        NULL,
        ext2_write_inode,
        ext2_put_inode,
        ext2_put_super,
        ext2_write_super,
        NULL, // ext2_statfs,
        ext2_remount
};
#endif
void ext2_put_super (struct super_block  *sb)
{
        int db_count;
        int i;
#ifdef OS2
        printk("ext2_put_super 0x%04X", sb->s_dev);
#endif
        lock_super (sb);
        if (!(sb->s_flags & MS_RDONLY)) {
                sb->u.ext2_sb.s_es->s_state = sb->u.ext2_sb.s_mount_state;
                mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
        }
#ifndef OS2
        sb->s_dev = 0;
#endif
        db_count = sb->u.ext2_sb.s_db_per_group;
        for (i = 0; i < db_count; i++)
                if (sb->u.ext2_sb.s_group_desc[i])
                        brelse (sb->u.ext2_sb.s_group_desc[i]);
        kfree_s (sb->u.ext2_sb.s_group_desc,
                 db_count * sizeof (struct buffer_head *));
        for (i = 0; i < EXT2_MAX_GROUP_LOADED; i++)
                if (sb->u.ext2_sb.s_inode_bitmap[i])
                        brelse (sb->u.ext2_sb.s_inode_bitmap[i]);
        for (i = 0; i < EXT2_MAX_GROUP_LOADED; i++)
                if (sb->u.ext2_sb.s_block_bitmap[i])
                        brelse (sb->u.ext2_sb.s_block_bitmap[i]);

        brelse (sb->u.ext2_sb.s_sbh);
        unlock_super (sb);
        return;
}

static void ext2_setup_super (struct super_block * sb,
                              struct ext2_super_block * es)
{
#ifdef OS2
        if (Errors_Panic) {
            clear_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_CONT);
            clear_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_RO);
            set_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_PANIC);
        } else {
            clear_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_PANIC);
            clear_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_RO);
            set_opt(sb->u.ext2_sb.s_mount_opt, ERRORS_CONT);
        }
#endif

        if (es->s_rev_level > EXT2_CURRENT_REV) {
                        printk ("EXT2-fs warning: revision level too high, "
                                "forcing read/only mode\n");
                        sb->s_flags |= MS_RDONLY;
        }
        if (!(sb->s_flags & MS_RDONLY)) {
                if (!(sb->u.ext2_sb.s_mount_state & EXT2_VALID_FS))
                        printk ("EXT2-fs warning: mounting unchecked fs, "
                                "running e2fsck is recommended\n");
                else if ((sb->u.ext2_sb.s_mount_state & EXT2_ERROR_FS))
                        printk ("EXT2-fs warning: mounting fs with errors, "
                                "running e2fsck is recommended\n");
                else if (es->s_max_mnt_count >= 0 &&
                         es->s_mnt_count >= (unsigned short) es->s_max_mnt_count)
                        printk ("EXT2-fs warning: maximal mount count reached, "
                                "running e2fsck is recommended\n");
                else if (es->s_checkinterval &&
                        (es->s_lastcheck + es->s_checkinterval <= CURRENT_TIME))
                        printk ("EXT2-fs warning: checktime reached, "
                                "running e2fsck is recommended\n");
                es->s_state &= ~EXT2_VALID_FS;
                if (!es->s_max_mnt_count)
                        es->s_max_mnt_count = EXT2_DFL_MAX_MNT_COUNT;
#ifndef OS2
                es->s_mnt_count++;
#else
                //
                // This is to force Linux to autocheck the ext2fs partition "touched" by OS/2
                // this autocheck is enabled unless -no_auto_fsck is specified on the IFS cmd
                // line
                //
                if (auto_fsck) {
                    kernel_printf("e2fsck will be forced next time Linux will mount this partition");
                      es->s_mnt_count = EXT2_DFL_MAX_MNT_COUNT;
                } else {
                    es->s_mnt_count++;
                }
#endif
                es->s_mtime = CURRENT_TIME;
                mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
                sb->s_dirt = 1;
#ifndef OS2 // For the moment .....
                if (test_opt (sb, DEBUG))
#endif
                        printk ("[EXT II FS %s, %s, bs=%lu, fs=%lu, gc=%lu, "
                                "bpg=%lu, ipg=%lu, mo=%04lx]\n",
                                EXT2FS_VERSION, EXT2FS_DATE, sb->s_blocksize,
                                sb->u.ext2_sb.s_frag_size,
                                sb->u.ext2_sb.s_groups_count,
                                EXT2_BLOCKS_PER_GROUP(sb),
                                EXT2_INODES_PER_GROUP(sb),
                                sb->u.ext2_sb.s_mount_opt);
#ifndef OS2 // For the moment .....
                if (test_opt (sb, CHECK)) {
#endif
                        ext2_check_blocks_bitmap (sb);
                        ext2_check_inodes_bitmap (sb);
#ifndef OS2 // For the moment .....
                }
#endif
        }
}

static int ext2_check_descriptors (struct super_block * sb)
{
    int i;
    int desc_block = 0;
    unsigned long block = sb->u.ext2_sb.s_es->s_first_data_block;
    struct ext2_group_desc * gdp = NULL;

    ext2_debug ("Checking group descriptors");

    for (i = 0; i < sb->u.ext2_sb.s_groups_count; i++)
    {
        if ((i % EXT2_DESC_PER_BLOCK(sb)) == 0)
            gdp = (struct ext2_group_desc *) sb->u.ext2_sb.s_group_desc[desc_block++]->b_data;
        if (gdp->bg_block_bitmap < block ||
            gdp->bg_block_bitmap >= block + EXT2_BLOCKS_PER_GROUP(sb))
        {
            ext2_error (sb, "ext2_check_descriptors",
                    "Block bitmap for group %d"
                    " not in group (block %lu)!",
                    i, (unsigned long) gdp->bg_block_bitmap);
            return 0;
        }
        if (gdp->bg_inode_bitmap < block ||
            gdp->bg_inode_bitmap >= block + EXT2_BLOCKS_PER_GROUP(sb))
        {
            ext2_error (sb, "ext2_check_descriptors",
                    "Inode bitmap for group %d"
                    " not in group (block %lu)!",
                    i, (unsigned long) gdp->bg_inode_bitmap);
            return 0;
        }
        if (gdp->bg_inode_table < block ||
            gdp->bg_inode_table + sb->u.ext2_sb.s_itb_per_group >=
            block + EXT2_BLOCKS_PER_GROUP(sb))
        {
            ext2_error (sb, "ext2_check_descriptors",
                    "Inode table for group %d"
                    " not in group (block %lu)!",
                    i, (unsigned long) gdp->bg_inode_table);
            return 0;
        }
        block += EXT2_BLOCKS_PER_GROUP(sb);
        gdp++;
    }
    return 1;
}

/*
 * This function has been shamelessly adapted from the msdos fs
 */
static int parse_options (char * options, unsigned long * sb_block,
              unsigned short *resuid, unsigned short * resgid,
              unsigned long * mount_options)
{
    char * this_char;
    char * value;

    if (!options)
        return 1;
    for (this_char = strtok (options, ",");
         this_char != NULL;
         this_char = strtok (NULL, ",")) {
        if ((value = strchr (this_char, '=')) != NULL)
            *value++ = 0;
        if (!strcmp (this_char, "bsddf"))
            clear_opt (*mount_options, MINIX_DF);
        else if (!strcmp (this_char, "check")) {
            if (!value || !*value)
                set_opt (*mount_options, CHECK_NORMAL);
            else if (!strcmp (value, "none")) {
                clear_opt (*mount_options, CHECK_NORMAL);
                clear_opt (*mount_options, CHECK_STRICT);
            }
            else if (strcmp (value, "normal"))
                set_opt (*mount_options, CHECK_NORMAL);
            else if (strcmp (value, "strict")) {
                set_opt (*mount_options, CHECK_NORMAL);
                set_opt (*mount_options, CHECK_STRICT);
            }
            else {
                printk ("EXT2-fs: Invalid check option: %s\n",
                    value);
                return 0;
            }
        }
        else if (!strcmp (this_char, "debug"))
            set_opt (*mount_options, DEBUG);
        else if (!strcmp (this_char, "errors")) {
            if (!value || !*value) {
                printk ("EXT2-fs: the errors option requires "
                    "an argument");
                return 0;
            }
            if (!strcmp (value, "continue")) {
                clear_opt (*mount_options, ERRORS_RO);
                clear_opt (*mount_options, ERRORS_PANIC);
                set_opt (*mount_options, ERRORS_CONT);
            }
            else if (!strcmp (value, "remount-ro")) {
                clear_opt (*mount_options, ERRORS_CONT);
                clear_opt (*mount_options, ERRORS_PANIC);
                set_opt (*mount_options, ERRORS_RO);
            }
            else if (!strcmp (value, "panic")) {
                clear_opt (*mount_options, ERRORS_CONT);
                clear_opt (*mount_options, ERRORS_RO);
                set_opt (*mount_options, ERRORS_PANIC);
            }
            else {
                printk ("EXT2-fs: Invalid errors option: %s\n",
                    value);
                return 0;
            }
        }
        else if (!strcmp (this_char, "grpid") ||
             !strcmp (this_char, "bsdgroups"))
            set_opt (*mount_options, GRPID);
        else if (!strcmp (this_char, "minixdf"))
            set_opt (*mount_options, MINIX_DF);
        else if (!strcmp (this_char, "nocheck")) {
            clear_opt (*mount_options, CHECK_NORMAL);
            clear_opt (*mount_options, CHECK_STRICT);
        }
        else if (!strcmp (this_char, "nogrpid") ||
             !strcmp (this_char, "sysvgroups"))
            clear_opt (*mount_options, GRPID);
        else if (!strcmp (this_char, "resgid")) {
            if (!value || !*value) {
                printk ("EXT2-fs: the resgid option requires "
                    "an argument");
                return 0;
            }
            *resgid = simple_strtoul (value, &value, 0);
            if (*value) {
                printk ("EXT2-fs: Invalid resgid option: %s\n",
                    value);
                return 0;
            }
        }
        else if (!strcmp (this_char, "resuid")) {
            if (!value || !*value) {
                printk ("EXT2-fs: the resuid option requires "
                    "an argument");
                return 0;
            }
            *resuid = simple_strtoul (value, &value, 0);
            if (*value) {
                printk ("EXT2-fs: Invalid resuid option: %s\n",
                    value);
                return 0;
            }
        }
        else if (!strcmp (this_char, "sb")) {
            if (!value || !*value) {
                printk ("EXT2-fs: the sb option requires "
                    "an argument");
                return 0;
            }
            *sb_block = simple_strtoul (value, &value, 0);
            if (*value) {
                printk ("EXT2-fs: Invalid sb option: %s\n",
                    value);
                return 0;
            }
        }
        else {
            printk ("EXT2-fs: Unrecognized mount option %s\n", this_char);
            return 0;
        }
    }
    return 1;
}


struct super_block * ext2_read_super (struct super_block * sb, void * data,
                      int silent)
{
    struct buffer_head * bh;
    struct ext2_super_block * es;
    unsigned long sb_block = 1;
    unsigned short resuid = EXT2_DEF_RESUID;
    unsigned short resgid = EXT2_DEF_RESGID;
    unsigned long logic_sb_block = 1;
    int dev = sb->s_dev;
    int db_count;
    int i, j;
#ifdef EXT2FS_PRE_02B_COMPAT
    int fs_converted = 0;
#endif

#ifndef OS2
    set_opt (sb->u.ext2_sb.s_mount_opt, CHECK_NORMAL);
#else
    set_opt (sb->u.ext2_sb.s_mount_opt, CHECK_STRICT);
#endif
    if (!parse_options ((char *) data, &sb_block, &resuid, &resgid,
        &sb->u.ext2_sb.s_mount_opt)) {
        sb->s_dev = 0;
        return NULL;
    }

    lock_super (sb);
    set_blocksize (dev, BLOCK_SIZE);
    if (!(bh = bread (dev, sb_block, BLOCK_SIZE))) {
        sb->s_dev = 0;
        unlock_super (sb);
        printk ("EXT2-fs: unable to read superblock\n");
        return NULL;
    }
    /*
     * Note: s_es must be initialized s_es as soon as possible because
     * some ext2 macro-instructions depend on its value
     */
    es = (struct ext2_super_block *) bh->b_data;
    sb->u.ext2_sb.s_es = es;
    sb->s_magic = es->s_magic;
    if (sb->s_magic != EXT2_SUPER_MAGIC
#ifdef EXT2FS_PRE_02B_COMPAT
       && sb->s_magic != EXT2_PRE_02B_MAGIC
#endif
       ) {
        if (!silent)
            printk ("VFS: Can't find an ext2 filesystem on dev %d/%d.\n",
                MAJOR(dev), MINOR(dev));
        failed_mount:
        sb->s_dev = 0;
        unlock_super (sb);
        if (bh)
                brelse (bh);
        return NULL;
    }
#ifdef EXT2_DYNAMIC_REV
    if (es->s_rev_level > EXT2_GOOD_OLD_REV) {
        if (es->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPP) {
#ifndef OS2
            printk("EXT2-fs: %s: couldn't mount because of "
                   "unsupported optional features.\n",
                   kdevname(dev));
#else
            printk("EXT2-fs (drive %c:) couldn't mount because of "
                   "unsupported optional features.\n",
                   sb->s_drive + 'A');
#endif
            goto failed_mount;
        }
        if (!(sb->s_flags & MS_RDONLY) &&
            (es->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPP)) {
#ifndef OS2
            printk("EXT2-fs: %s: couldn't mount RDWR because of "
                   "unsupported optional features.\n",
                   kdevname(dev));
#else
            printk("EXT2-fs (drive %c:) couldn't mount RDWR because of "
                   "unsupported optional features.\n",
                   sb->s_drive + 'A');
#endif
            goto failed_mount;
        }
    }
#endif
    sb->s_blocksize = EXT2_MIN_BLOCK_SIZE << es->s_log_block_size;
    sb->s_blocksize_bits = EXT2_BLOCK_SIZE_BITS(sb);
    if (sb->s_blocksize != BLOCK_SIZE &&
        (sb->s_blocksize == 1024 || sb->s_blocksize == 2048 ||
         sb->s_blocksize == 4096)) {
        unsigned long offset;

        brelse (bh);
        set_blocksize (dev, sb->s_blocksize);
        logic_sb_block = (sb_block*BLOCK_SIZE) / sb->s_blocksize;
        offset = (sb_block*BLOCK_SIZE) % sb->s_blocksize;
        bh = bread (dev, logic_sb_block, sb->s_blocksize);
        if(!bh)
            return NULL;
        es = (struct ext2_super_block *) (((char *)bh->b_data) + offset);
        sb->u.ext2_sb.s_es = es;
        if (es->s_magic != EXT2_SUPER_MAGIC) {
            sb->s_dev = 0;
            unlock_super (sb);
            brelse (bh);
            printk ("EXT2-fs: Magic mismatch, very weird !\n");
            return NULL;
        }
    }
#ifdef EXT2_DYNAMIC_REV
    if (es->s_rev_level == EXT2_GOOD_OLD_REV) {
        sb->u.ext2_sb.s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
        sb->u.ext2_sb.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    } else {
        sb->u.ext2_sb.s_inode_size = es->s_inode_size;
        sb->u.ext2_sb.s_first_ino = es->s_first_ino;
        if (sb->u.ext2_sb.s_inode_size != EXT2_GOOD_OLD_INODE_SIZE) {
            printk ("EXT2-fs: unsupported inode size: %d\n",
                sb->u.ext2_sb.s_inode_size);
            goto failed_mount;
        }
    }
#endif
    sb->u.ext2_sb.s_frag_size = EXT2_MIN_FRAG_SIZE <<
                   es->s_log_frag_size;
    if (sb->u.ext2_sb.s_frag_size)
        sb->u.ext2_sb.s_frags_per_block = sb->s_blocksize /
                          sb->u.ext2_sb.s_frag_size;
    else
        sb->s_magic = 0;
    sb->u.ext2_sb.s_blocks_per_group = es->s_blocks_per_group;
    sb->u.ext2_sb.s_frags_per_group = es->s_frags_per_group;
    sb->u.ext2_sb.s_inodes_per_group = es->s_inodes_per_group;
    sb->u.ext2_sb.s_inodes_per_block = sb->s_blocksize /
                       sizeof (struct ext2_inode);
    sb->u.ext2_sb.s_itb_per_group = sb->u.ext2_sb.s_inodes_per_group /
                        sb->u.ext2_sb.s_inodes_per_block;
    sb->u.ext2_sb.s_desc_per_block = sb->s_blocksize /
                     sizeof (struct ext2_group_desc);
    sb->u.ext2_sb.s_sbh = bh;
    sb->u.ext2_sb.s_es = es;
    if (resuid != EXT2_DEF_RESUID)
        sb->u.ext2_sb.s_resuid = resuid;
    else
        sb->u.ext2_sb.s_resuid = es->s_def_resuid;
    if (resgid != EXT2_DEF_RESGID)
        sb->u.ext2_sb.s_resgid = resgid;
    else
        sb->u.ext2_sb.s_resgid = es->s_def_resgid;
    sb->u.ext2_sb.s_mount_state = es->s_state;
    sb->u.ext2_sb.s_rename_lock = 0;
#ifndef OS2
    sb->u.ext2_sb.s_rename_wait = NULL;
#else
    sb->u.ext2_sb.s_rename_wait = 0;
#endif
#ifdef EXT2FS_PRE_02B_COMPAT
    if (sb->s_magic == EXT2_PRE_02B_MAGIC) {
        if (es->s_blocks_count > 262144) {
            /*
             * fs > 256 MB can't be converted
             */
            sb->s_dev = 0;
            unlock_super (sb);
            brelse (bh);
            printk ("EXT2-fs: trying to mount a pre-0.2b file"
                "system which cannot be converted\n");
            return NULL;
        }
        printk ("EXT2-fs: mounting a pre 0.2b file system, "
            "will try to convert the structure\n");
        if (!(sb->s_flags & MS_RDONLY)) {
            sb->s_dev = 0;
            unlock_super (sb);
            brelse (bh);
            printk ("EXT2-fs: cannot convert a read-only fs\n");
            return NULL;
        }
        if (!convert_pre_02b_fs (sb, bh)) {
            sb->s_dev = 0;
            unlock_super (sb);
            brelse (bh);
            printk ("EXT2-fs: conversion failed !!!\n");
            return NULL;
        }
        printk ("EXT2-fs: conversion succeeded !!!\n");
        fs_converted = 1;
    }
#endif
    if (sb->s_magic != EXT2_SUPER_MAGIC) {
        sb->s_dev = 0;
        unlock_super (sb);
        brelse (bh);
        if (!silent)
            printk ("VFS: Can't find an ext2 filesystem on dev %d/%d.\n",
                MAJOR(dev), MINOR(dev));
        return NULL;
    }
    if (sb->s_blocksize != bh->b_size) {
        sb->s_dev = 0;
        unlock_super (sb);
        brelse (bh);
        if (!silent)
            printk ("VFS: Unsupported blocksize on dev 0x%04x.\n",
                dev);
        return NULL;
    }

    if (sb->s_blocksize != sb->u.ext2_sb.s_frag_size) {
        sb->s_dev = 0;
        unlock_super (sb);
        brelse (bh);
        printk ("EXT2-fs: fragsize %lu != blocksize %lu (not supported yet)\n",
            sb->u.ext2_sb.s_frag_size, sb->s_blocksize);
        return NULL;
    }

    sb->u.ext2_sb.s_groups_count = (es->s_blocks_count -
                        es->s_first_data_block +
                       EXT2_BLOCKS_PER_GROUP(sb) - 1) /
                       EXT2_BLOCKS_PER_GROUP(sb);
    db_count = (sb->u.ext2_sb.s_groups_count + EXT2_DESC_PER_BLOCK(sb) - 1) /
           EXT2_DESC_PER_BLOCK(sb);
#ifndef OS2
    sb->u.ext2_sb.s_group_desc = kmalloc (db_count * sizeof (struct buffer_head *), GFP_KERNEL);
    if (sb->u.ext2_sb.s_group_desc == NULL) {
#else
    if (DevHlp32_VMAlloc(db_count * sizeof (struct buffer_head *), VMDHA_NOPHYSADDR, VMDHA_SWAP, (void **)(&(sb->u.ext2_sb.s_group_desc))) != NO_ERROR) {
#endif
        sb->s_dev = 0;
        unlock_super (sb);
        brelse (bh);
        printk ("EXT2-fs: not enough memory\n");
        return NULL;
    }
    for (i = 0; i < db_count; i++) {
        sb->u.ext2_sb.s_group_desc[i] = bread (dev, logic_sb_block + i + 1,
                               sb->s_blocksize);
        if (!sb->u.ext2_sb.s_group_desc[i]) {
            sb->s_dev = 0;
            unlock_super (sb);
            for (j = 0; j < i; j++)
                brelse (sb->u.ext2_sb.s_group_desc[j]);
            kfree_s (sb->u.ext2_sb.s_group_desc,
                 db_count * sizeof (struct buffer_head *));
            brelse (bh);
            printk ("EXT2-fs: unable to read group descriptors\n");
            return NULL;
        }
    }
    if (!ext2_check_descriptors (sb)) {
        sb->s_dev = 0;
        unlock_super (sb);
        for (j = 0; j < db_count; j++)
            brelse (sb->u.ext2_sb.s_group_desc[j]);
        kfree_s (sb->u.ext2_sb.s_group_desc,
             db_count * sizeof (struct buffer_head *));
        brelse (bh);
        printk ("EXT2-fs: group descriptors corrupted !\n");
        return NULL;
    }
    for (i = 0; i < EXT2_MAX_GROUP_LOADED; i++) {
        sb->u.ext2_sb.s_inode_bitmap_number[i] = 0;
        sb->u.ext2_sb.s_inode_bitmap[i] = NULL;
        sb->u.ext2_sb.s_block_bitmap_number[i] = 0;
        sb->u.ext2_sb.s_block_bitmap[i] = NULL;
    }
    sb->u.ext2_sb.s_loaded_inode_bitmaps = 0;
    sb->u.ext2_sb.s_loaded_block_bitmaps = 0;
    sb->u.ext2_sb.s_db_per_group = db_count;
    unlock_super (sb);
    /*
     * set up enough so that it can read an inode
     */
    sb->s_dev = dev;
    sb->s_op = &ext2_sops;
    if (!(sb->s_mounted = iget (sb, EXT2_ROOT_INO))) {
        sb->s_dev = 0;
        for (i = 0; i < db_count; i++)
            if (sb->u.ext2_sb.s_group_desc[i])
                brelse (sb->u.ext2_sb.s_group_desc[i]);
        kfree_s (sb->u.ext2_sb.s_group_desc,
             db_count * sizeof (struct buffer_head *));
        brelse (bh);
        printk ("EXT2-fs: get root inode failed\n");
        return NULL;
    }
#ifdef EXT2FS_PRE_02B_COMPAT
    if (fs_converted) {
        for (i = 0; i < db_count; i++)
            mark_buffer_dirty(sb->u.ext2_sb.s_group_desc[i], 1);
        sb->s_dirt = 1;
    }
#endif
    ext2_setup_super (sb, es);
    return sb;
}

static void ext2_commit_super (struct super_block * sb,
                   struct ext2_super_block * es)
{
    es->s_wtime = CURRENT_TIME;
    mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
    sb->s_dirt = 0;
}

/*
 * In the second extended file system, it is not necessary to
 * write the super block since we use a mapping of the
 * disk super block in a buffer.
 *
 * However, this function is still used to set the fs valid
 * flags to 0.  We need to set this flag to 0 since the fs
 * may have been checked while mounted and e2fsck may have
 * set s_state to EXT2_VALID_FS after some corrections.
 */

void ext2_write_super (struct super_block * sb)
{
    struct ext2_super_block * es;

    if (!(sb->s_flags & MS_RDONLY)) {
        es = sb->u.ext2_sb.s_es;

        ext2_debug ("setting valid to 0\n");

        if (es->s_state & EXT2_VALID_FS) {
            es->s_state &= ~EXT2_VALID_FS;
            es->s_mtime = CURRENT_TIME;
        }
        ext2_commit_super (sb, es);
    }
    sb->s_dirt = 0;
}

int ext2_remount (struct super_block * sb, int * flags, char * data)
{
    struct ext2_super_block * es;
    unsigned short resuid = sb->u.ext2_sb.s_resuid;
    unsigned short resgid = sb->u.ext2_sb.s_resgid;
    unsigned long new_mount_opt;
    unsigned long tmp;

    /*
     * Allow the "check" option to be passed as a remount option.
     */
    set_opt (sb->u.ext2_sb.s_mount_opt, CHECK_NORMAL);
    if (!parse_options (data, &tmp, &resuid, &resgid,
                &new_mount_opt))
        return -EINVAL;

    sb->u.ext2_sb.s_mount_opt = new_mount_opt;
    sb->u.ext2_sb.s_resuid = resuid;
    sb->u.ext2_sb.s_resgid = resgid;
    es = sb->u.ext2_sb.s_es;
    if ((*flags & MS_RDONLY) == (sb->s_flags & MS_RDONLY))
        return 0;
    if (*flags & MS_RDONLY) {
        if (es->s_state & EXT2_VALID_FS ||
            !(sb->u.ext2_sb.s_mount_state & EXT2_VALID_FS))
            return 0;
        /*
         * OK, we are remounting a valid rw partition rdonly, so set
         * the rdonly flag and then mark the partition as valid again.
         */
        es->s_state = sb->u.ext2_sb.s_mount_state;
        es->s_mtime = CURRENT_TIME;
        mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
        sb->s_dirt = 1;
        ext2_commit_super (sb, es);
    }
    else {
        /*
         * Mounting a RDONLY partition read-write, so reread and
         * store the current valid flag.  (It may have been changed
         * by e2fsck since we originally mounted the partition.)
         */
        sb->u.ext2_sb.s_mount_state = es->s_state;
        sb->s_flags &= ~MS_RDONLY;
        ext2_setup_super (sb, es);
    }
    return 0;
}

struct file_system_type ext2_fs_type = {
        ext2_read_super, "ext2", 1, NULL
};



#ifdef OS2

#define ERROR_VOLUME_NOT_MOUNTED 0xEE00                // IFS specific

/*
 * This routine reads the superblock and tests if it can be an ext2 file system.
 * It does NOT use buffers from the cache.
 */
int check_ext2fs_magic(struct vpfsi32 *pvpfsi, unsigned short hVPB) {
    int   nb_sec;
    int   rc;
    int   rc2;
    char *buf;
    struct ext2_super_block *es;

    //
    // Allocates a temporary buffer
    //
    if ((rc = DevHlp32_VMAlloc(BLOCK_SIZE, VMDHA_NOPHYSADDR, VMDHA_FIXED | VMDHA_CONTIG, __StackToFlat(&buf))) == NO_ERROR) {
        //
        // Reads disk block 1 (with blocksize = 1024)
        //
        nb_sec = BLOCK_SIZE / pvpfsi->vpi_bsize;
        if ((rc = fsh32_dovolio(
                                DVIO_OPREAD | DVIO_OPRESMEM | DVIO_OPBYPASS | DVIO_OPNCACHE,
                                DVIO_ALLABORT | DVIO_ALLRETRY | DVIO_ALLFAIL,
                                hVPB,
                                buf,
                                __StackToFlat(&nb_sec),
                                nb_sec
                               )) == NO_ERROR) {

            es = (struct ext2_super_block *)buf;
            if (es->s_magic == EXT2_SUPER_MAGIC) {
                kernel_printf("ext2 signature found in superblock (hVPB = 0x%04X)", hVPB);

                /*
                 * The volume serial number is a 32 bits CRC checksum of the UUID field
                 */
                pvpfsi->vpi_vid = updcrc(es->s_uuid, sizeof(es->s_uuid));

                /*
                 * The volume name is truncated to the valid OS/2 volume label length (11 chars)
                 */
            strncpy(pvpfsi->vpi_text, es->s_volume_name, sizeof(pvpfsi->vpi_text));
                pvpfsi->vpi_text[sizeof(pvpfsi->vpi_text) - 1] = '\0';

        rc = NO_ERROR;
            } else {
                kernel_printf("ext2 signature NOT found in superblock (hVPB = 0x%04X)", hVPB);
        rc = ERROR_VOLUME_NOT_MOUNTED;
            }

        } else {
        kernel_printf("check_ext2fs_magic - fsh32_dovolio returned %d", rc);
        }

        if ((rc2 = DevHlp32_VMFree((void *)buf)) == NO_ERROR) {
            /*
             * Nothing else
             */
        } else {
        kernel_printf("check_ext2fs_magic - VMFree returned %d", rc);
            rc = rc2;
        }
    } else {
        kernel_printf("check_ext2fs_magic - VMAlloc returned %d", rc);
    }
    return rc;

}
#endif