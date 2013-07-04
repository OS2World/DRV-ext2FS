//
// $Header: e:\\netlabs.cvs\\ext2fs/include/asm/bitops.h,v 1.1 2001/05/09 17:43:37 umoeller Exp $
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

#ifndef _ASM_GENERIC_BITOPS_H_
#define _ASM_GENERIC_BITOPS_H_

#ifdef OS2
#include <os2\types.h>
#include <os2\StackToFlat.h>
#endif

/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assembly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * Also note, these routines assume that you have 32 bit integers.
 * You will have to change this if you are trying to port Linux to the
 * Alpha architecture or to a Cray.  :-)
 *
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */
INLINE void cli(void) {_disable();}
INLINE void sti(void) {_enable();}

#if 0
INLINE int set_bit(int nr,int * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    cli();
    retval = (mask & *addr) != 0;
    *addr |= mask;
    sti();
    return retval;
}

INLINE int clear_bit(int nr, int * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    cli();
    retval = (mask & *addr) != 0;
    *addr &= ~mask;
    sti();
    return retval;
}

INLINE int test_bit(int nr, int * addr)
{
    int mask;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    return ((mask & *addr) != 0);
}
#else
extern int _Optlink set_bit(
                            int nr,          /* eax */
                            void * addr          /* edx */
                           );
extern int _Optlink test_bit(
                            int nr,          /* eax */
                            void * addr          /* edx */
                           );
extern int _Optlink clear_bit(
                            int nr,          /* eax */
                            void * addr          /* edx */
                           );
extern unsigned long _Optlink ffz(
                                  unsigned long word    /* eax */
                                 );
#endif

/*
 * find the first occurrence of byte 'c', or 1 past the area if none
 */
INLINE void * memscan(void * cs, long c, long n) {
    unsigned char  *x  = (unsigned char *)cs;
    unsigned char   cc = (unsigned char)c;
    long i = 0;

    while ((x[i] != cc) && (i < n)) i++;
    return (x[i] == cc ? x + i : x + n);
}

#if 0
INLINE unsigned long ffz(unsigned long word) {
    unsigned long temp = 0;
    word = ~word;
    while (!test_bit(temp, (int *)&word)) temp++;
    return temp;

}
#endif

INLINE int find_first_zero_bit(void *addr, unsigned size) {
    unsigned long i = 0L, j = 0L;
    unsigned long *x = (unsigned long *)addr;
    size = size / 32L;
    while ((i < size) && (x[i]==~0L)) i++;
    if  (i == size) return (i << 5L);
    while ((j < 32L) && ((x[i] & (1L << j)) != 0L)) j++;
   return ((i << 5L) + j );
}

INLINE int find_next_zero_bit (void * addr, int size, int offset)
{
    unsigned long   *p = ((unsigned long *) addr) + (offset >> 5L);
    long            set = 0L,
                    bit = offset & 31L,
                    res;
    unsigned long   q = (*p) >> bit;

    if (bit) {
            set = find_first_zero_bit(__StackToFlat(&q), 32L);
// V0.9.12 (2001-04-29) [umoeller]            set = find_first_zero_bit(&q, 32L);
            if (set < 32L - bit) {
                return set + offset;
            }
            set = 32L - bit;
            p++;
    }
    /*
     * No zero yet, search remaining full bytes for a zero
     */
    res = find_first_zero_bit (p, size - 32L * (p - (unsigned long *) addr));

    return (offset + set + res);
}


#endif /* _ASM_GENERIC_BITOPS_H */
