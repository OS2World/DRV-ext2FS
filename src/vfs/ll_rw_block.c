//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/ll_rw_block.c,v 1.1 2001/05/09 17:48:07 umoeller Exp $
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


//
//
// This file contains an "emulation" of Linux ll_rw_block I/O routine, 100%
// specific to OS/2. Its purpose is to emulate Linux block device I/O by
// rerouting the requests to the relevant OS/2 device driver, using either
// FSH_DOVOLIO (strategy 1) or directly through the strategy 2 DD entry point
// (if supported).
//
#ifdef __IBMC__
#pragma strings(readonly)
#endif

#define os2_panic(msg) ext2_os2_panic(0, msg)

#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>
#include <string.h>

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\fsh32.h>
#include <os2\os2proto.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <os2\log.h>
#include <os2\magic.h>
#include <os2\request_list.h>

#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\sched.h>
#include <linux\locks.h>



//
// WARNING !!! Unlike Linux ll_rw_block, here ll_rw_block is SYNCHRONOUS because FSH_DOVOLIO is.
// If we wanted an asynchronous version we could for instance use the extended strategy interface
// if the block device supports it.
//
static void ll_rw_block_standard(int rw, int nr, struct buffer_head **bh, struct super_block *sb) {
    int rc;
    int i;
    int nb_sec;
    int rwmode;

    switch (rw) {
        case READ  :
        case READA :
            rwmode = DVIO_OPREAD;
            break;
        case WRITE  :
        case WRITEA :
            rwmode = DVIO_OPWRITE;
            break;
        default :
            os2_panic("ll_rw_block() - Invalid flag");
    }
    for (i = 0 ; i < nr ; i++) {
        lock_buffer(bh[i]);
        bh[i]->b_uptodate = 0;
        mark_buffer_clean(bh[i]);
        nb_sec = (int)sb->sectors_per_block;
        if ((rc = fsh32_dovolio(
                                rwmode,
//                                DVIO_ALLFAIL | DVIO_ALLABORT | DVIO_ALLRETRY,
                                DVIO_ALLACK,
                                bh[i]->b_dev,
                                bh[i]->b_data,
                                __StackToFlat(&nb_sec),
                                bh[i]->b_blocknr * sb->sectors_per_block
                               )) != NO_ERROR) {
            kernel_printf("FSH_DOVOLIO() - rc = 0x%04X - blk_no = %lu", rc, bh[i]->b_blocknr);
//            if (rc == ERROR_NOT_READY) {
//                fsh32_forcevolume(hVPB, 1);
//            }
        } else {
            bh[i]->b_uptodate = 1;
        }
        unlock_buffer(bh[i]);

    }
}

int FS32CALLBACK request_list_completed(struct  _Req_List_Header *request_list);
//#pragma alloc_text(EXT2_FIXED_CODE, request_list_completed)
//
// Request list completed callback
//
// *** WARNING *** This routine is called in interrupt context
//
int FS32CALLBACK request_list_completed(struct  _Req_List_Header *request_list) {
    struct _Req_List_Header *pRLH;        // Request list header
    struct _PB_Read_Write   *pRLE;        // Request list entry
    struct _SG_Descriptor   *pSG;         // Scatter-gather descriptor
    struct RW_private       *pBH;         // ext2fs specific part of RLE


    pRLH   = request_list;
    pRLE   = (struct _PB_Read_Write *)((unsigned long)pRLH + sizeof(struct _Req_List_Header));
    pSG    = (struct _SG_Descriptor *)((unsigned long)pRLE + sizeof(struct _PB_Read_Write));
    pBH    = (struct  RW_private    *)((unsigned long)pSG  + sizeof(struct _SG_Descriptor) * pRLE->SG_Desc_Count);

    if (pBH->magic != RW_PRIVATE_MAGIC) {
       os2_panic("request_list_completed - request list with an invalid magic number");
    }
    if (pRLH->Lst_Status & RLH_All_Req_Done) {
        put_reqlist(pBH->reqlist);

    }
    return NO_ERROR;
}


int FS32CALLBACK request_completed(struct _PB_Read_Write *request);
//#pragma alloc_text(EXT2_FIXED_CODE, request_completed)
//
// Request completed callback
//
// *** WARNING *** This routine is called in interrupt context
//
int FS32CALLBACK request_completed(struct _PB_Read_Write *request) {
    struct _PB_Read_Write   *pRLE;     // Request list entry
    struct _SG_Descriptor   *pSG;      // Scatter-gather descriptor
    struct RW_private       *pBH;      // ext2fs specific part of RLE
    int status;
    int err_status;
    int i;

    pRLE   = (struct _PB_Read_Write *)request;
    pSG    = (struct _SG_Descriptor *)((unsigned long)pRLE + sizeof(struct _PB_Read_Write));
    pBH    = (struct RW_private *)((unsigned long)pSG  + sizeof(struct _SG_Descriptor) * pRLE->SG_Desc_Count);


    if (pBH->magic != RW_PRIVATE_MAGIC) {
       os2_panic("request_completed - request list with an invalid magic number");
    }

    status         = pRLE->RqHdr.Status & 0x0F;
    err_status     = pRLE->RqHdr.Status & 0xF0;

    //
    // First case : the request is finished
    //
    if (status == RH_DONE) {
        switch (err_status) {
            case RH_NO_ERROR :
                break;
            case RH_RECOV_ERROR :
//                os2_panic("request_completed - Recoverable error");
//                kernel_printf("request_completed() - Recoverable error 0x%0X occured for block %lu - status = RH_DONE", pRLE->RqHdr.Error_Code, pBH->bh->b_blocknr);
                break;
            default :
//                kernel_printf("request_completed() - RH_DONE & 0x%0X", err_status);
//                os2_panic("request_completed - Unrecoverable error");
                for (i = 0 ; i < pBH->nb_bh ; i++) {
                    unlock_buffer(pBH->bh[i]);
                }
                return NO_ERROR;
        }
#if 0
        switch (pRLE->RqHdr.Command_Code) {
            case PB_WRITE_X :
                for (i = 0 ; i < pBH->nb_bh ; i++) {
                     if (pBH->bh[i]->b_dirt) {
                         mark_buffer_clean(pBH->bh[i]);
                    }
                }
                break;

                case PB_READ_X :
                    break;
                default :
                      break;
        }
#endif
        for (i = 0 ; i < pBH->nb_bh ; i++) {
            pBH->bh[i]->b_uptodate = 1;
            unlock_buffer(pBH->bh[i]);
        }
        return NO_ERROR;
    }

    //
    // Second case : the request is NOT finished
    //
    switch (err_status) {
        //
        // In this case we should be notified later of the real status of the request.
        //
        case RH_NO_ERROR    :
        case RH_RECOV_ERROR :
            return NO_ERROR;
        //
        // In this case there is no more chance ...
        //
        default :
            for (i = 0 ; i < pBH->nb_bh ; i++) {
                unlock_buffer(pBH->bh[i]);
            }
            return NO_ERROR;
    }
}

extern unsigned long stub_request_completed;
extern unsigned long stub_request__list_completed;


static void ll_rw_block_strat2(int rw, int nr, struct buffer_head **bh, struct super_block *sb) {
    int k, l, n;
    int nb_extents, nr_sg;
    int nr_sg_tot = 0;
    int i;    unsigned char command;
    struct reqlist *reqlist;
    unsigned long sectors_per_block;
    struct buffer_head *__bh2[32];
    struct buffer_head **bh2 = __StackToFlat(__bh2);
    struct _Req_List_Header *tmp;

    struct _Req_List_Header *pRLH;        // Request list header
    struct _PB_Read_Write   *pRLE;        // Request list entry
    struct _SG_Descriptor   *pSG;        // Scatter-gather descriptor
    struct RW_private       *pBH;        // ext2fs specific part of RLE


    switch (rw) {
        case READ  :
        case READA :
            command = PB_READ_X;
            break;
        case WRITE  :
        case WRITEA :
            command = PB_WRITE_X;
            break;
        default :
            ext2_os2_panic(0, "ll_rw_block_strat2() - Invalid flag");
    }

    //
    // - We suppose that all buffer_head structures passed to ll_rw_block are for the same drive ...
    // - We also suppose that the bh array will result in a request list < 64 Kb
    // - We also suppose that there are less than 32 items in the bh array

    //
    // Let's sort the bh array by ascending disk block order (dumb bubble sort ... I will change
    // this later !)
    //
    memcpy(bh2, bh, sizeof(struct buffer_head *) * nr);
    {
         int found = 1;
         int j     = 0;
            while (found) {
                found = 0;
                for (i = j ; i < nr - 1 ; i++) {
                    if (bh2[i]->b_blocknr > bh2[i + 1]->b_blocknr) {
                        struct buffer_head *tmp;
                        tmp = bh2[i];
                        bh2[i] = bh2[i + 1];
                        bh2[i + 1] = tmp;
                        found = 1;
                    }
            j++;
                }
            }
    }

    //
    // Counts the number of extents (an extent is a set of consecutive disk blocks)
    //
    k = 0;
    n = 0;
    while (k < nr) {
        l = k + 1;
        while ((l < nr) && (bh2[l]->b_blocknr == bh2[k]->b_blocknr + l - k)) l++;
        if (k < nr) {
            n++;
        }
        k = l;
    }
    nb_extents = n;

    /*
     * Allocates a request list.
     */
    reqlist = get_reqlist();

    tmp = &(reqlist->s_rwlist);
    pRLH   = tmp;
    pRLE   = (struct _PB_Read_Write *)((char *)pRLH + sizeof(struct _Req_List_Header));


    //
    // We initialize the request list header
    //
    pRLH->Count           = nb_extents;                     // number of requests in Req List
    pRLH->Notify_Address  = (void *)stub_request_list_completed;    // 16:16 address of notification routine
    if (nb_extents == 1) {
        pRLH->Request_Control = RLH_Notify_Done |
                                RLH_Notify_Err  |
                                RLH_Single_Req;             // Notify on completion or on error
    } else {
        pRLH->Request_Control = RLH_Notify_Done |
                                RLH_Notify_Err;             // Notify on completion or on error
    }
    pRLH->Block_Dev_Unit  = sb->s_unit;                     // logical unit number of volume (pvpfsi->vpi_unit)


    //
    // We now create a request for each extent
    //
    k = 0;    // First blk of extent
    n = 0;    // Number of extents
//    l = 0;  // Last blk of extent
    while (k < nr) {
        l = k + 1;
        while ((l < nr) && (bh2[l]->b_blocknr == bh2[k]->b_blocknr + l - k)) {
            l++;
        }
        //
        // Now the current extent is [k, l[
        // n is the current extent number (starting from 0)
        //
        if (k < nr) {
            pSG = (struct _SG_Descriptor *)((char *)pRLE + sizeof(struct _PB_Read_Write));
#if 0
            pBH = (struct RW_private *)((char *)pSG  + sizeof(struct _SG_Descriptor) * (l - k));
#endif
            //
            // We lock the buffers of the current extent
            //
            for (i = k ; i < l ; i++) {
                lock_buffer(bh2[i]);
                bh2[i]->b_uptodate = 0;
        mark_buffer_clean(bh2[i]);
            }

            //
            // We initialize the request header
            //
#if 0
            pRLE->RqHdr.Length         = (n == nb_extents - 1 ? RH_LAST_REQ : (USHORT)pBH  + (USHORT)sizeof(struct RW_private) + (USHORT)(l - k - 1) * (USHORT)sizeof(struct buffer_head *) - (USHORT)pRLE);        // offset of the next request
#endif
            pRLE->RqHdr.Old_Command    = PB_REQ_LIST;                          // always 1Ch
            pRLE->RqHdr.Command_Code   = command;                              // command code
            pRLE->RqHdr.Head_Offset    = (ULONG)pRLE - (ULONG)pRLH;            // offset from begin of Req List Header
            pRLE->RqHdr.Req_Control    = RH_NOTIFY_DONE | RH_NOTIFY_ERROR;     // Notify on completion or error
            pRLE->RqHdr.Priority       = (BYTE)(G_pLocInfoSeg->LIS_CurThrdPri >> 8);   // Priority of request
            pRLE->RqHdr.Status         = RH_NOT_QUEUED | RH_NO_ERROR;          // status bitfield
            pRLE->RqHdr.Error_Code     = 0;                                    // I24 error code
            pRLE->RqHdr.Notify_Address = (void *)stub_request_completed;               // 16:16 address of notification routine
            pRLE->RqHdr.Hint_Pointer   = 0xFFFFFFFF;                           // 16:16 pointer to req packet in list

            //
            // We initialize the command specific fields
            //
            sectors_per_block    = bh2[k]->b_size / 512;
            pRLE->Start_Block    = bh2[k]->b_blocknr * sectors_per_block;     // disk block number
            pRLE->Block_Count    = sectors_per_block * (l - k);               // number of blocks to transfer
            pRLE->Blocks_Xferred = 0;                                         // number of blocks transferred
            pRLE->RW_Flags       = 0;                                         // command specific control flags
#if 0
            pRLE->SG_Desc_Count  = l - k;                                     // number of SG descriptors
            //
            // We initialize the scatter-gather descriptors
            //
            for (i = k ; i < l ; i++) {
                pSG[i - k].BufferSize = bh2[i]->b_size;                       // size of the buffer in bytes
                pSG[i - k].BufferPtr  = (PVOID)(bh2[i]->b_physaddr);          // physical address of buffer
            }
#else
            //
            // We initialize the scatter-gather descriptors
            //
        nr_sg = 0;
        pSG[nr_sg].BufferSize = bh2[k]->b_size;
        pSG[nr_sg].BufferPtr  = (PVOID)(bh2[k]->b_physaddr);
            for (i = k + 1 ; i < l ; i++) {
        if (bh2[i]->b_physaddr == (ULONG)(pSG[nr_sg].BufferPtr) + pSG[nr_sg].BufferSize) {
                    pSG[nr_sg].BufferSize += bh2[i]->b_size;
    //      printk("Contiguous buffer %d", bh2[i]->b_blocknr);
        } else {
            nr_sg ++;
                    pSG[nr_sg].BufferSize = bh2[i]->b_size;                       // size of the buffer in bytes
                    pSG[nr_sg].BufferPtr  = (PVOID)(bh2[i]->b_physaddr);          // physical address of buffer
                }
            }
        pRLE->SG_Desc_Count  = nr_sg + 1;                                     // number of SG descriptors
            nr_sg_tot += nr_sg + 1;
            pBH = (struct RW_private *)((char *)pSG  + sizeof(struct _SG_Descriptor) * (nr_sg + 1));
            pRLE->RqHdr.Length         = (n == nb_extents - 1 ? RH_LAST_REQ : (USHORT)pBH  + (USHORT)sizeof(struct RW_private) + (USHORT)(l - k - 1) * (USHORT)sizeof(struct buffer_head *) - (USHORT)pRLE);        // offset of the next request
#endif
            //
            // We initialize the bh field and req_list field
            //
            pBH->nb_bh        = l - k;
            pBH->reqlist      = reqlist;
            pBH->magic        = RW_PRIVATE_MAGIC;
            for (i = k ; i < l ; i++) {
                pBH->bh[i - k] = bh2[i];
            }


            //
            // Next request list entry
            //
            pRLE   = (struct _PB_Read_Write *)((char *)pRLE    + pRLE->RqHdr.Length);

            //
            // Next extent
            //
            n++;
        }
        k = l;
    }
/*
    kernel_printf("ll_rw_block : nr = %d, extents = %d, sg = %d", nr, nb_extents, nr_sg_tot);
*/
    //
    // We now call the device driver's extended strategy routine
    //
    send_RLH_to_driver(reqlist->s.s_self_virt, sb->s_strat2);


}



void ll_rw_block(int rw, int nr, struct buffer_head **bh) {
    struct super_block *sb;
    int                 i;

    if (nr == 0) {
        kernel_printf("ll_rw_block - Nothing to do");
        return;
    }

    switch (rw) {
        case READ  :
        case READA :
            break;

        case WRITE  :
        case WRITEA :
            if (!Read_Write) {
                for (i = 0; i < nr; i++) {
                    mark_buffer_clean(bh[i]);
                }
                return;
            }
            break;

        default :
            os2_panic("ll_rw_block() - Invalid flag");
    }



    for (i = 0 ; i < nr ; i++) {
        if (bh[i]->b_dev == 0) {
            os2_panic("ll_rw_block - NULL device");
        }
        if (bh[i]->b_dev == 0xffff) {
            os2_panic("ll_rw_block - device is 0xFFFF");
        }
        if (bh[0]->b_dev != bh[i]->b_dev) {
            os2_panic("ll_rw_block - Multi-device request NOT allowed");
        }
    }
    if ((sb = getvolume(bh[0]->b_dev)) == 0) {
        os2_panic("ll_rw_block : can't retrieve superblock");
    }

    if (sb->s_strat2.seg) {
        ll_rw_block_strat2(rw, nr, bh, sb);
//        kernel_printf("u [%d] f [%d] m [%lu] r [%lu] a [%lu]", nusedreqlist, nfreereqlist, mallocated, reallocated, asis);
    } else {
        if ((sb->s_status == VOL_STATUS_MOUNTED) ||
            (sb->s_status == VOL_STATUS_MOUNT_IN_PROGRESS)) {
            ll_rw_block_standard(rw, nr, bh, sb);
        }
    }
}

