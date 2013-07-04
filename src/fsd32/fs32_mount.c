
/*
 *@@sourcefile fs32_mount.c:
 *      32-bit C handler called from 16-bit entry point
 *      in src\interface\fs_thunks.asm.
 *
 *      See fs32_mount() below.
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
#include <linux\ext2_proto.h>
#include <linux\stat.h>
#include <os2\vfsapi.h>
#include <os2\ifsdbg.h>

#define ERROR_VOLUME_NOT_MOUNTED 0xEE00     // IFS specific

/* ******************************************************************
 *
 *   Global variables (DGROUP)
 *
 ********************************************************************/

extern char Use_Extended_Interface;
extern struct file_system_type ext2_fs_type;

#if 0
extern struct file_system_type dasd_fs_type;
#endif

extern int drive_options[];

/* ******************************************************************
 *
 *   Code (CODE32 segment)
 *
 ********************************************************************/

/*
 *@@ do_mount:
 *
 *
 */

struct super_block* do_mount(struct vpfsi32 *pvpfsi,
                             union vpfsd32 *pvpfsd,
                             unsigned short hVPB,
                             struct file_system_type *type)
{
    struct super_block *tmp;
    struct super_block *sb;
    int rc;

    sb = get_empty_super();
    pvpfsd->u.sb = sb;

    if ((sb) && (sb->s_magic_internal == SUPER_MAGIC))
    {
        sb->sector_size = pvpfsi->vpi_bsize;
        sb->nb_sectors = pvpfsi->vpi_totsec;
        sb->s_drive = pvpfsi->vpi_drive;
        sb->s_unit = pvpfsi->vpi_unit;
        sb->s_blocksize = BLOCK_SIZE;
        sb->sectors_per_block = BLOCK_SIZE / sb->sector_size;
        sb->s_dev = hVPB;
        sb->s_status = VOL_STATUS_MOUNT_IN_PROGRESS;
        if (!Read_Write)
            sb->s_flags = MS_RDONLY;

        /*
         * Strategy 2 I/O support processing
         */
        if (Use_Extended_Interface)
        {
            struct DriverCaps32 *pDCS;
            struct VolChars32 *pVCS;

            if (pvpfsi->vpi_pDCS.seg)
            {
                if ((rc = DevHlp32_VirtToLin(pvpfsi->vpi_pDCS,
                                             __StackToFlat(&pDCS)))
                            == NO_ERROR)
                {
                    if (pvpfsi->vpi_pVCS.seg)
                    {
                        if ((rc = DevHlp32_VirtToLin(pvpfsi->vpi_pVCS,
                                                     __StackToFlat(&pVCS)))
                                           == NO_ERROR)
                        {

                            if ((pDCS) && (pDCS->Strategy2.seg))
                            {
                                kernel_printf("\tStrategy 2 entry point found");
                                if (pVCS)
                                {
                                    kernel_printf("\tVolume descriptor        : 0x%04X", pVCS->VolDescriptor);  /* see equates below                    */
                                    kernel_printf("\tAverage seek time        : %d", pVCS->AvgSeekTime);    /* milliseconds, if unknown, FFFFh      */
                                    kernel_printf("\tAverage latency time     : %d", pVCS->AvgLatency);     /* milliseconds, if unknown, FFFFh      */
                                    kernel_printf("\tBlocks on smallest track : %d", pVCS->TrackMinBlocks);     /* blocks on smallest track             */
                                    kernel_printf("\tBlocks on largest track  : %d", pVCS->TrackMaxBlocks);     /* blocks on largest track              */
                                    kernel_printf("\tMax scatter-gather list  : %d", pVCS->MaxSGList);  /* Adapter scatter/gather list limit    */
                                }
                                if ((pVCS) && (pVCS->VolDescriptor & VC_REMOVABLE_MEDIA))
                                {
                                    printk("\tMedia is removable ...");
                                    if (drive_options[pvpfsi->vpi_drive])
                                    {
                                        kernel_printf("\t... using strat 2 I/Os");
                                        sb->s_strat2.seg = pDCS->Strategy2.seg;
                                        sb->s_strat2.ofs = pDCS->Strategy2.ofs;
                                    }
                                    else
                                    {
                                        kernel_printf("\t... not supported for strat 2 yet !");
                                    }
                                }
                                else
                                {
                                    kernel_printf("\tMedia is NOT removable, using strat 2 I/Os");
                                    sb->s_strat2.seg = pDCS->Strategy2.seg;
                                    sb->s_strat2.ofs = pDCS->Strategy2.ofs;
                                }
                            }
                            else
                            {
                                kernel_printf("\tNO strategy 2 entry point found, using standard FSH_DOVOLIO instead");
                            }
                        }
                        else
                        {
                            kernel_printf("\tError while thunking pvpfsi->vpi_pVCS - rc = %d", rc);
                        }
                    }
                }
                else
                {
                    kernel_printf("\tError while thunking pvpfsi->vpi_pDCS - rc = %d", rc);
                }
            }
        }
        else
        {
            kernel_printf("\tUsing standard I/Os as requested on the IFS command line.");
        }

        if ((type) && (type->read_super))
        {
            tmp = type->read_super(sb, 0, 0);
            if (tmp)
            {
                sb->s_blocks_per_page = (unsigned char)(4096 / sb->s_blocksize);
                sb->s_status = VOL_STATUS_MOUNTED;
                sb->sectors_per_block = sb->s_blocksize / sb->sector_size;
                /*
                 * nothing else to do ...
                 */
            }
            else
            {
                put_super(sb);
                sb = 0;
            }
        }
        else
        {
            sb = 0;
        }

    }
    else
    {
        sb = 0;
    }

    return sb;
}

/*
 *@@ do_remount_sb:
 *      alters the mount flags of a mounted file system. Only the mount point
 *      is used as a reference - file system type and the device are ignored.
 *      FS-specific mount options can't be altered by remounting.
 */

int do_remount_sb(struct super_block *sb,
                  int flags,
                  char *data)
{
    int retval;

#ifndef OS2
    if (!(flags & MS_RDONLY) && sb->s_dev && is_read_only(sb->s_dev))
        return -EACCES;
    /*flags |= MS_RDONLY; */
    /* If we are remounting RDONLY, make sure there are no rw files open */
    if ((flags & MS_RDONLY) && !(sb->s_flags & MS_RDONLY))
        if (!fs_may_remount_ro(sb->s_dev))
            return -EBUSY;
#endif
    if (sb->s_op && sb->s_op->remount_fs)
    {
        retval = sb->s_op->remount_fs(sb,
                                      __StackToFlat(&flags),
                                      data);
        if (retval)
            return retval;
    }
    sb->s_flags = (sb->s_flags & ~MS_RMT_MASK) |
        (flags & MS_RMT_MASK);
    return 0;
}

/*
 *@@ do_unmount:
 *
 */

int do_unmount(struct super_block *sb)
{

    if (!fs_may_umount(sb->s_dev, sb->s_mounted))
    {
        kernel_printf("\tdo_unmount WARNING !! device busy");
        return ERROR_DEVICE_IN_USE;
    }
    invalidate_files(sb, 1);
    sync_buffers(sb->s_dev, 1);
    iput(sb->s_mounted);
    if (sb->s_mounted->i_count)
        printk("do_unmount WARNING !!! sb->s_mounted->i_count = %d", sb->s_mounted->i_count);
    sync_inodes(sb->s_dev);
    sync_buffers(sb->s_dev, 1);
    if (!sb->s_is_swapper_device)
    {
        invalidate_inodes(sb->s_dev);
        sync_buffers(sb->s_dev, 1);
    }
    if ((sb->s_op) && (sb->s_op->put_super))
    {
        sb->s_op->put_super(sb);
        sync_buffers(sb->s_dev, 1);
        sync_buffers(0, 1);
    }
    if (!sb->s_is_swapper_device)
    {
        invalidate_buffers(sb->s_dev);
    }
    put_super(sb);

    sync_buffers(0, 1);
    sync_buffers(0, 1);

    return NO_ERROR;
}

extern int check_ext2fs_magic(struct vpfsi32 *pvpfsi, unsigned short hVPB);

extern void _Optlink fs32_init_CheckRing0Initialize(void);

/*
 *@@ fs32_mount:
 *      32-bits handler called from FS_MOUNT (src\interface\fs_thunks.asm).
 *
 *      From my testing, for local (non-remote) file systems,
 *      FS_MOUNT gets called by the kernel
 *      the first time a drive is accessed. The kernel then
 *      goes thru all FSDs and asks them to probe the volume
 *      for whether they can handle it. If no FSD says "yes,
 *      I will", the kernel tries to treat it as FAT, and if
 *      that fails also, the drive cannot be accessed.
 *
 *      Also from my testing, for each new drive, the FSDs get
 *      called in the CONFIG.SYS order. For example, if
 *      HPFS.IFS is before ext2 in CONFIG.SYS, we never get
 *      requests for HPFS drives here because HPFS.IFS has
 *      already claimed them.
 *
 *      When a volume has been recognized, the relationship
 *      between drive, FSD, volume serial number, and volume
 *      label is remembered. The volume serial number and label
 *      are stored in the volume parameter block (VPB). The
 *      VPB is maintained by OS/2 for open files (I/O based on
 *      file-handles), searches, and buffer references. The
 *      VPB represents the media.
 *
 *      Subsequent requests for a volume that has been removed
 *      require polling the installed FSDs for volume
 *      recognition by calling FS_MOUNT. The volume serial
 *      number and volume label of the VPB returned by the
 *      recognizing FSD and the existing VPB are compared. If
 *      the test fails, OS/2 signals the critical error handler
 *      to prompt the user for the correct volume.
 *
 *      The connection between media and VPB is remembered until
 *      all open files on the volume are closed and search and
 *      cache buffer references are removed. Only volume changes
 *      cause a redetermination of the media at the time of next
 *      access.
 *
 *      The 16-bit FS_MOUNT thunk converts the stack parameters into
 *      a flat structure pointer, which is given to this 32-bit
 *      function then. As a result, we get a pointer to the
 *      following:
 *
 +          struct fs32_mount_parms {
 +              PTR16           pBoot;
 +              unsigned short  hVPB;
 +              PTR16           pvpfsd;
 +              PTR16           pvpfsi;
 +              unsigned short  flag;
 +          };
 *
 *      From IFS.INF:
 *
 *      "flag" indicates operation requested:
 *
 *      --  flag == 0 indicates that the FSD is requested to mount
 *          or accept a volume.
 *
 *          During a mount request, the FSD may examine other sectors on
 *          the media by using FSH_DOVOLIO to perform the I/O. If an
 *          uncertain-media return is detected, the FSD is expected to
 *          clean up and return an UNCERTAIN MEDIA error in order to allow
 *          the volume mount logic to restart on the newly-inserted media.
 *          The FSD must provide the buffer to use for additional I/O.
 *
 *      --  flag == 1 indicates that the FSD is being advised that
 *          the specified volume has been removed.
 *
 *          When the kernel detects that a volume has been removed from
 *          its driver, but there are still outstanding references to the
 *          volume, FS_MOUNT is called with flag == 1 to allow the FSD to
 *          drop clean (or other regenerable) data for the volume. Data
 *          which is dirty and cannot be regenerated should be kept so
 *          that it may  be written to the volume when it is remounted
 *          in the drive.
 *
 *          Since the hardware does not allow for kernel-mediated removal
 *          of media, it is certain that the unmount request is issued
 *          when the volume is not present in any drive.
 *
 *      --  flag == 2 indicates that the FSD is requested to release
 *          all internal storage assigned to that volume as it has
 *          been removed from its driver and the last kernel-managed
 *          reference to that volume has been removed.
 *
 *          The OS/2 kernel manages the VPB through a reference count. All
 *          volume-specific objects are labeled with the appropriate
 *          volume handle and represent references to the VPB. When all
 *          kernel references to a volume disappear, FS_MOUNT is called
 *          with flag == 2, indicating a dismount request.
 *
 *      --  flag == 3 indicates that the FSD is requested to accept
 *          the volume regardless of recognition in preparation for
 *          formatting for use with the FSD.
 *
 *          When a volume is to be formatted for use with an FSD,
 *          the kernel calls the FSD's FS_MOUNT entry point with
 *          flag == 3 to allow the FSD to prepare for the format
 *          operation. The FSD should accept the volume even if it
 *          is not a volume of the type that FSD recognizes, since
 *          the point of format is to change the file system on the
 *          volume. The operation may fail if formatting does not
 *          make sense. (For example, an FSD which supports only
 *          CD-ROM.)
 *
 *      "pvpfsi" is a pointer to the file-system-independent portion
 *      of VPB. If the media contains an OS/2-recognizable boot
 *      sector, then the vpi_vid field contains the 32-bit identifier
 *      for that volume. If the media does not contain such a boot
 *      sector, the FSD must generate a unique label for the media
 *      and place it into the vpi_vid field.
 *
 +      "pvpfsd" is a pointer to the file-system-dependent portion of
 *      VPB. The FSD may store information as necessary into this area.
 *
 *      The contents of the vpfsd data structure are as follows:
 *
 *      --  FLAG = 0: The FSD is expected to issue an FSD_FINDDUPHVPB
 *          to see if a duplicate VPB exists. If one does exist, the
 *          VPB fs dependent area of the new VPB is invalid and the
 *          new VPB will be unmounted after the FSD returns from the
 *          MOUNT. The FSD is expected to update the FS dependent area
 *          of the old duplicate VPB. If no duplicate VPB exists, the
 *          FSD should initialize the FS dependent area.
 *
 *      --  FLAG = 1: VPB FS dependent part is same as when FSD last
 *          modified it.
 *
 *      --  FLAG = 2: VPB FS dependent part is same as when FSD last
 *          modified it.
 *
 *      "hVPB" is the handle to the volume.
 *
 *      "pBoot" is a pointer to sector 0 read from the media. This
 *      pointer is only valid when flag == 0. The buffer the pointer
 *      refers to must not be  modified. The pointer is always valid
 *      and does not need to be verified when flag == 0. If a  read
 *      error occurred, the buffer will contain zeroes.
 *
 *      Remarks:
 *
 *      The FSD examines the volume presented and determine whether
 *      it recognizes the file system. If it does, it returns zero,
 *      after having filled in appropriate parts of the vpfsi and
 *      vpfsd data structures. The vpi_vid and vpi_text fields must
 *      be filled in by the FSD. If the FSD has an OS/2 format boot
 *      sector, it must convert the label from the media into ASCIIZ
 *      form. The vpi_hDev field is filled in by OS/2. If the volume
 *      is unrecognized, the driver returns non-zero.
 *
 *      The vpi_text and vpi_vid must be updated by the FSD each time
 *      these values change.
 *
 *      After media recognition time, the volume parameters may be
 *      examined using the FSH_GETVOLPARM call. The volume parameters
 *      should not be changed after media recognition time.
 */

int FS32ENTRY fs32_mount(struct fs32_mount_parms *parms)
{
    struct vpfsi32 *pvpfsi;
    union vpfsd32 *pvpfsd;
    char *pBoot;
    int rc;
    int i;
    struct super_block *sb;
    struct super_block *old_sb;
    unsigned short oldhVPB;

    // _interrupt(3);      // V0.9.12 (2001-05-03) [umoeller]

    // V0.9.12 (2001-05-03) [umoeller]
    fs32_init_CheckRing0Initialize();

    // thunk parameters
    pvpfsi = VDHQueryLin(parms->pvpfsi);
    pvpfsd = VDHQueryLin(parms->pvpfsd);

    switch (parms->flag)
    {
        case MOUNT_MOUNT:
            kernel_printf("FS_MOUNT flg = MOUNT_MOUNT hVPB = 0x%04X", parms->hVPB);

            if ((rc = DevHlp32_VirtToLin(parms->pBoot,
                                         __StackToFlat(&pBoot)))
                        == NO_ERROR)
            {
                /*
                 * We zero out pvpfsd for safety purposes !
                 */
                memset(pvpfsd, 0, sizeof(union vpfsd32));

                if ((rc = check_ext2fs_magic(pvpfsi, parms->hVPB)) == NO_ERROR)
                {
#if 0
                    /*
                     * The volume serial number is a CRC checksum of the boot sector
                     */
                    pvpfsi->vpi_vid = updcrc((unsigned char *)pBoot, pvpfsi->vpi_bsize);

                    /*
                     * The volume label is dummy for the moment ("ext2fs_<drive>")
                     */
                    sprintf(pvpfsi->vpi_text, "EXT2FS_%c", pvpfsi->vpi_drive + 'A');

#endif

                    /*
                     * Is there another instance of the drive ?
                     *     - Yes : update internal volume table and return silently
                     *     - No  : continue the mount process
                     */
                    if ((rc = fsh32_findduphvpb(parms->hVPB,
                                                __StackToFlat(&oldhVPB)))
                                     == NO_ERROR)
                    {
                        kernel_printf(" \tFSH_FINDDUPHVPB(0x%0X) - Found dup hVPB 0x%0X", (int)parms->hVPB, (int)oldhVPB);

                        old_sb = getvolume(oldhVPB);

                        if ((old_sb) && (old_sb->s_magic_internal == SUPER_MAGIC))
                        {
                            old_sb->s_status = VOL_STATUS_MOUNTED;
                            rc = NO_ERROR;
                        }
                        else
                        {
                            kernel_printf("Cannot find old superblock !");
                            rc = ERROR_INVALID_PARAMETER;
                        }


                    }
                    else
                    {
                        kernel_printf(" \tFSH_FINDDUPHVPB - NO dup hVPB");
                        /*
                         * This is the first instance of the drive
                         */

                        sb = do_mount(pvpfsi, pvpfsd, parms->hVPB, &ext2_fs_type);

                        if (sb)
                        {
#if 0
                            /*
                             * Volume name (if any)
                             */
                            if (*(sb->u.ext2_sb.s_es->s_volume_name))
                            {
                                strncpy(pvpfsi->vpi_text, sb->u.ext2_sb.s_es->s_volume_name, sizeof(pvpfsi->vpi_text));
                            }
#endif

                            kernel_printf("Volume characteristics :");
                            kernel_printf("========================");
                            kernel_printf("\t volume id    : 0x%08X", pvpfsi->vpi_vid);
                            kernel_printf("\t hDEV         : 0x%08X", pvpfsi->vpi_hDEV);
                            kernel_printf("\t sector size  : %u", pvpfsi->vpi_bsize);
                            kernel_printf("\t sector/track : %u", pvpfsi->vpi_trksec);
                            kernel_printf("\t heads        : %u", pvpfsi->vpi_nhead);
                            kernel_printf("\t tot sectors  : %lu", pvpfsi->vpi_totsec);
                            kernel_printf("\t drive (0=A)  : %d", (int)(pvpfsi->vpi_drive));
                            kernel_printf("\t unit code    : %d", (int)(pvpfsi->vpi_unit));
                            kernel_printf("\t volume label : %s", pvpfsi->vpi_text);
                            kernel_printf("\t hVPB         : 0x%04X", parms->hVPB);

                            sb->s_status = VOL_STATUS_MOUNTED;
                            pvpfsd->u.sb = sb;
                            rc = NO_ERROR;
                        }
                        else
                        {
                            rc = ERROR_VOLUME_NOT_MOUNTED;
                        }
                    }


                }
                else
                {
                    kernel_printf("\tbasic superblock validation failed for volume 0x%04X", (int)parms->hVPB);
                    rc = ERROR_VOLUME_NOT_MOUNTED;
                }
            }
            else
            {
                kernel_printf("FS_MOUNT - couldn't thunk pBoot");
            }
            break;


        case MOUNT_VOL_REMOVED:
            kernel_printf("FS_MOUNT flg = MOUNT_VOL_REMOVED hVPB = 0x%04X", parms->hVPB);
            if ((pvpfsd->u.sb) && (pvpfsd->u.sb->s_magic_internal == SUPER_MAGIC))
            {
                pvpfsd->u.sb->s_status = VOL_STATUS_REMOVED;
                rc = NO_ERROR;
            }
            else
            {
                rc = ERROR_INVALID_PARAMETER;
            }
            break;

        case MOUNT_RELEASE:
            kernel_printf("FS_MOUNT flg = MOUNT_RELEASE hVPB = 0x%04X", parms->hVPB);
            if ((pvpfsd->u.sb) && (pvpfsd->u.sb->s_magic_internal == SUPER_MAGIC))
            {
                kernel_printf("\tvalid superblock present in VPB");

                switch (pvpfsd->u.sb->s_status)
                {
                    case VOL_STATUS_REMOVED:
                        kernel_printf("\tmedia is NOT present");
                        /*
                         * This is a "hard" unmount !!
                         */
                        invalidate_files(pvpfsd->u.sb, 0);
                        hard_invalidate_inodes(pvpfsd->u.sb->s_dev);
                        if ((pvpfsd->u.sb->s_op) && (pvpfsd->u.sb->s_op->put_super))
                            pvpfsd->u.sb->s_op->put_super(pvpfsd->u.sb);
                        invalidate_buffers(pvpfsd->u.sb->s_dev);
                        put_super(pvpfsd->u.sb);
                        memset(pvpfsd, 0, sizeof(union vpfsd32));

                        rc = NO_ERROR;
                        break;

                    case VOL_STATUS_MOUNTED:
                        kernel_printf("\tmedia is present");
                        rc = do_unmount(pvpfsd->u.sb);
                        if (rc == NO_ERROR)
                        {
                            memset(pvpfsd, 0, sizeof(union vpfsd32));
                        }
                        break;

                    default:
                        kernel_printf("FS_MOUNT - unexpected volume status");
                        rc = ERROR_INVALID_PARAMETER;
                        break;
                }
            }
            else
            {
                kernel_printf("\tNO superblock present in VPB (probably unmounting a dup VPB)");
                rc = NO_ERROR;
            }
            break;


        case MOUNT_ACCEPT:
            kernel_printf("FS_MOUNT flg = MOUNT_ACCEPT hVPB = 0x%04X", parms->hVPB);
#if 0
            sb = do_mount(pvpfsi, pvpfsd, parms->hVPB, &dasd_fs_type);
            if (sb)
            {
                kernel_printf("Volume characteristics :");
                kernel_printf("========================");
                kernel_printf("\t volume id    : 0x%08X", pvpfsi->vpi_vid);
                kernel_printf("\t hDEV         : 0x%08X", pvpfsi->vpi_hDEV);
                kernel_printf("\t sector size  : %u", pvpfsi->vpi_bsize);
                kernel_printf("\t sector/track : %u", pvpfsi->vpi_trksec);
                kernel_printf("\t heads        : %u", pvpfsi->vpi_nhead);
                kernel_printf("\t tot sectors  : %lu", pvpfsi->vpi_totsec);
                kernel_printf("\t drive (0=A)  : %d", (int)(pvpfsi->vpi_drive));
                kernel_printf("\t unit code    : %d", (int)(pvpfsi->vpi_unit));
                kernel_printf("\t volume label : %s", pvpfsi->vpi_text);
                kernel_printf("\t hVPB         : 0x%08X", parms->hVPB);

                sb->s_status = VOL_STATUS_MOUNTED;
                pvpfsd->u.sb = sb;
                rc = NO_ERROR;
            }
            else
            {
                rc = ERROR_VOLUME_NOT_MOUNTED;
            }
#else
            rc = ERROR_NOT_SUPPORTED;
#endif
            break;

        default:
            kernel_printf("FS_MOUNT() invalid flag %d", parms->flag);
            rc = ERROR_INVALID_PARAMETER;
            break;
    }

    return rc;
}
