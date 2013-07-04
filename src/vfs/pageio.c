//
// $Header: e:\\netlabs.cvs\\ext2fs/src/vfs/pageio.c,v 1.1 2001/05/09 17:48:07 umoeller Exp $
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
#include <os2.h>                // From the "Developer Connection Device Driver Kit" version 2.0

#include <os2\types.h>
#include <os2\StackToFlat.h>
#include <os2\devhlp32.h>
#include <os2\os2proto.h>
#include <os2\errors.h>
#include <os2\volume.h>
#include <os2\log.h>
#include <os2\magic.h>     // Magic numbers
#include <os2\request_list.h>
#include <linux\fs.h>
#include <linux\fs_proto.h>
#include <linux\sched.h>
#include <linux\locks.h>


/*
 * This routine adds one page from a PageCmdList to the end of a request list.
 * It returns the number of RLE added to the list. 0 means the list is full.
 */
static int add_page_to_request(struct PageCmdHeader *h, struct PageCmd *p, struct reqlist *s, struct file *f) {
    int j, k, l, n, nb_extents;
    int nr = f->f_inode->i_sb->s_blocks_per_page;
    blk_t blks[8]; // 4096 / 512
    blk_t sectors_per_block;

    struct _Req_List_Header *pRLH;        // Request list header
    struct _PB_Read_Write   *pRLE;        // Request list entry
    struct _SG_Descriptor   *pSG;         // Scatter-gather descriptor
    struct _PG_Private      *pPG;         // ext2fs specific part of RLE
    struct _Req_List_Header *tmp;

//    printk("\tadd_page_to_request");

    pRLH   = &(s->s_rwlist);
    if (s->s.s_last_RLE) {
        pRLE  = s->s.s_last_RLE;
        pRLE  =  (struct _PB_Read_Write *)((char *)pRLE + pRLE->RqHdr.Length);
    } else {
        /*
         * First RLE in the list
         */
        pRLE   = (struct _PB_Read_Write *)((char *)pRLH + sizeof(struct _Req_List_Header));
    }

    /*
     * Is there enough room left ? (one RLE per block in the page is the worst case)
     */
//    if (sizeof(struct reqlist) - (unsigned long)pRLE < nr * SZ_ONE_REQ) {
//        return 0;
//    }

    if ( (char *)pRLE + nr * SZ_ONE_REQ > (char *)s + sizeof(struct reqlist)) {
        return 0;
    }

    /*
     * Converts SWAPPER.DAT file offsets to disk block numbers
     */
    for (j = 0; j < nr; j++) {
        blks[j] = f->f_inode->i_op->bmap(f->f_inode, (p->FileOffset + j * f->f_inode->i_sb->s_blocksize) / f->f_inode->i_sb->s_blocksize);
        if (!blks[j])
            ext2_os2_panic(0, "swapper - bmap returned 0");
    }

    //
    // Counts the number of extents (an extent is a set of consecutive disk blocks)
    //
    k = 0;
    n = 0;
    while (k < nr) {
        l = k + 1;
        while ((l < nr) && (blks[l] == blks[k] + l - k)) l++;
        if (k < nr) {
            n++;
        }
        k = l;
    }
    nb_extents = n;
//    kernel_printf("\tpageio : nr = %d, extents = %d", nr, n);

    //
    // We now create a request for each extent
    //
    k = 0;    // First blk of extent
    n = 0;    // Number of extents
//    l = 0;  // Last blk of extent
    while (k < nr) {
        l = k + 1;
        while ((l < nr) && (blks[l] == blks[k] + l - k)) {
            l++;
        }
        //
        // Now the current extent is [k, l[
        // n is the current extent number (starting from 0)
        //
        if (k < nr) {
            pSG = (struct _SG_Descriptor *)((char *)pRLE + sizeof(struct _PB_Read_Write));
            pPG = (struct _PG_Private    *)((char *)pSG  + sizeof(struct _SG_Descriptor));

            /*
             * Updates last RLE pointer in swapreq
             */
            s->s.s_last_RLE = pRLE;

            //
            // We initialize the request header
            //
            pRLE->RqHdr.Length         = (unsigned short)((unsigned long)pPG  + sizeof(struct _PG_Private) - (unsigned long)pRLE);        // offset of the next request
            pRLE->RqHdr.Old_Command    = PB_REQ_LIST;                          // always 1Ch
            pRLE->RqHdr.Command_Code   = p->Cmd;                              // command code
            pRLE->RqHdr.Head_Offset    = (ULONG)pRLE - (ULONG)pRLH;            // offset from begin of Req List Header
            pRLE->RqHdr.Req_Control    = RH_NOTIFY_DONE | RH_NOTIFY_ERROR;     // Notify on completion or error
            pRLE->RqHdr.Priority       = (BYTE)(G_pLocInfoSeg->LIS_CurThrdPri >> 8);   // Priority of request
            pRLE->RqHdr.Status         = RH_NOT_QUEUED | RH_NO_ERROR;          // status bitfield
            pRLE->RqHdr.Error_Code     = 0;                                    // I24 error code
            pRLE->RqHdr.Notify_Address = (void *)stub_pager_RLE_completed;               // 16:16 address of notification routine
            pRLE->RqHdr.Hint_Pointer   = 0xFFFFFFFF;                           // 16:16 pointer to req packet in list

            //
            // We initialize the command specific fields
            //
            sectors_per_block    = f->f_inode->i_sb->s_blocksize / 512;
            pRLE->Start_Block    = blks[k] * sectors_per_block;     // disk block number
            pRLE->Block_Count    = sectors_per_block * (l - k);               // number of blocks to transfer
            pRLE->Blocks_Xferred = 0;                                         // number of blocks transferred
            pRLE->RW_Flags       = 0;                                         // command specific control flags
            pRLE->SG_Desc_Count  = 1;                               // number of SG descriptors

            //
            // We initialize the scatter-gather descriptor
            //
            pSG->BufferSize = (l - k) * f->f_inode->i_sb->s_blocksize;  // size of the buffer in bytes
            pSG->BufferPtr  = (void *)(p->Addr + k * f->f_inode->i_sb->s_blocksize);                       // physical address of buffer

            //
            // We initialize the bh field and req_list field
            //
            pPG->reqlist      = s;
            pPG->magic        = PG_PRIVATE_MAGIC;
            pPG->list         = h;
            pPG->cmd[0]       = p;
//            pPG->nr_pages     = 1;


            //
            // Next request list entry
            //
            pRLE = (struct _PB_Read_Write *)((char *)pRLE + pRLE->RqHdr.Length);
#if 0
            /*
             * Updates last RLE pointer in swapreq
             */
            s->s.s_last_RLE    = pRLE;
#endif
            //
            // Next extent
            //
            n++;
        }
        k = l;
    }
    pRLH->Count += nb_extents;

    return nb_extents;
}


void do_pageio(struct PageCmdHeader *pPageCmdHeader, struct file *f) {
    struct reqlist *s;
    int left, cnt;
    int full;

    struct _Req_List_Header *pRLH;        // Request list header
    struct _PB_Read_Write   *pRLE;        // Request list entry
    struct _SG_Descriptor   *pSG;         // Scatter-gather descriptor
    struct _PG_Private      *pPG;        // ext2fs specific part of RLE
    struct _Req_List_Header *tmp;
    int                      tmpnr;

    left = pPageCmdHeader->OpCount;
    cnt  = pPageCmdHeader->OpCount;

    while(left) {
        s = get_reqlist();

        pRLH            = &(s->s_rwlist);
        s->s.s_last_RLE = 0;

        /*
         * Request List Header initialization.
         */
        pRLH->Count           = 0;                                    // number of requests in Req List
        pRLH->Notify_Address  = (void *)stub_pager_RLH_completed;             // 16:16 address of notification routine
        pRLH->Request_Control = RLH_Notify_Done |
                                RLH_Notify_Err;                       // Notify on completion or on error
        pRLH->Block_Dev_Unit  = f->f_inode->i_sb->s_unit;             // logical unit number of volume (pvpfsi->vpi_unit)
        full = 0;
        while (left && !full) {
            tmpnr = add_page_to_request(pPageCmdHeader, &(pPageCmdHeader->PageCmdList[cnt - left]), s, f);
            if (tmpnr)
                left--;
            else
                full = 1;
        }
        if (pRLH->Count == 1) {
            pRLH->Request_Control |= RLH_Single_Req;
        }
        pRLE               = s->s.s_last_RLE;
        pRLE->RqHdr.Length = RH_LAST_REQ;

        //
        // We now call the device driver's extended strategy routine
        //
//        printk("\tsending %d extents to driver");
        send_RLH_to_driver(s->s.s_self_virt, f->f_inode->i_sb->s_strat2);

        //
        // Waits for completion
        //
        sleep_on(s);
        put_reqlist(s);

    }
}


int FS32CALLBACK pager_request_list_completed(struct  _Req_List_Header *request_list);
//#pragma alloc_text(EXT2_FIXED_CODE, pager_request_list_completed)

//
// Request list completed callback
//
// *** WARNING *** This routine is called in interrupt context
//
int FS32CALLBACK pager_request_list_completed(struct  _Req_List_Header *request_list) {
    struct _Req_List_Header *pRLH;        // Request list header
    struct _PB_Read_Write   *pRLE;        // Request list entry
    struct _SG_Descriptor   *pSG;        // Scatter-gather descriptor
    struct _PG_Private      *pPG;        // ext2fs specific part of RLE

    pRLH = request_list;
    pRLE   = (struct _PB_Read_Write *)((char *)pRLH + sizeof(struct _Req_List_Header));
    pSG    = (struct _SG_Descriptor *)((char *)pRLE + sizeof(struct _PB_Read_Write));
    pPG    = (struct _PG_Private    *)((char *)pSG  + sizeof(struct _SG_Descriptor) * pRLE->SG_Desc_Count);

//    printk("pager_request_list_completed - status = %04X", pRLH->Lst_Status);

#ifdef FS_STRICT_CHECKING
    if (pPG->magic != PG_PRIVATE_MAGIC) {
       ext2_os2_panic(0, "request_list_completed - request list with an invalid magic number");
    }
#endif

    if (pRLH->Lst_Status & RLH_All_Req_Done) {
        /*
         * Outflags is 0 on entry of FS_DOPAGEIO, and is set to PGIO_FO_ERROR only in
         * pager_request_completed
         */
        if (!pPG->list->OutFlags) {
            pPG->list->OutFlags = PGIO_FO_DONE;
        }
        wake_up(pPG->reqlist);
        return NO_ERROR;
    } else {
        return NO_ERROR;
    }
}

int FS32CALLBACK pager_request_completed(struct _PB_Read_Write *request);
//#pragma alloc_text(EXT2_FIXED_CODE, pager_request_completed)
//
// Request completed callback
//
// *** WARNING *** This routine is called in interrupt context
//
int FS32CALLBACK pager_request_completed(struct _PB_Read_Write *request) {
    struct _PB_Read_Write   *pRLE;     // Request list entry
    struct _SG_Descriptor   *pSG;      // Scatter-gather descriptor
    struct _PG_Private      *pPG;      // ext2fs specific part of RLE
    int status;
    int err_status;
    int i;

    pRLE   = request;
    pSG    = (struct _SG_Descriptor *)((char *)pRLE + sizeof(struct _PB_Read_Write));
    pPG    = (struct _PG_Private    *)((char *)pSG  + sizeof(struct _SG_Descriptor) * pRLE->SG_Desc_Count);

#ifdef FS_STRICT_CHECKING
    if (pPG->magic != PG_PRIVATE_MAGIC) {
       ext2_os2_panic(0, "request_completed - request list with an invalid magic number");
    }
#endif
    status         = pRLE->RqHdr.Status & 0x0F;
    err_status     = pRLE->RqHdr.Status & 0xF0;

    pPG->cmd[0]->Error  = err_status;
    pPG->cmd[0]->Status = status;

    switch (err_status) {
        //
        // In this case we should be notified later of the real status of the request.
        //
        case RH_NO_ERROR    :
        case RH_RECOV_ERROR :
            break;
        //
        // In this case there is no more chance ...
        //
        default :
            /*
             * Marks the page list header as error
             */
            pPG->list->OutFlags  = PGIO_FO_ERROR;
            break;
    }
    return NO_ERROR;
}
