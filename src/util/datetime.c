//
// $Header: e:\\netlabs.cvs\\ext2fs/src/util/datetime.c,v 1.1 2001/05/09 17:49:52 umoeller Exp $
//

// 32 bits OS/2 device driver and IFS support. Provides 32 bits kernel 
// services (DevHelp) and utility functions to 32 bits OS/2 ring 0 code 
// (device drivers and installable file system drivers).
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

//
// date_dos2unix  and date_unix2dos are from the Linux FAT file system
//

/* Linear day numbers of the respective 1sts in non-leap years. */

static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
                  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */


// extern struct timezone sys_tz;
extern long timezone;

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

long date_dos2unix(unsigned short time,unsigned short date)
{
        long month,year,secs;

        month = ((date >> 5) & 15L)-1L;
        year = date >> 9;
        secs = (time & 31L)*2L+60*((time >> 5) & 63L)+(time >> 11)*3600L+86400L*
            ((date & 31L)-1L+day_n[month]+(year/4L)+year*365L-((year & 3L) == 0 &&
            month < 2L ? 1 : 0)+3653L);
                        /* days since 1.1.70 plus 80's leap day */
//        secs += sys_tz.tz_minuteswest*60;
        secs += timezone;
        return secs;
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */

void date_unix2dos(long unix_date,unsigned short *time,
    unsigned short *date)
{
        long day,year,nl_day,month;
        unsigned long ud;
//        unix_date -= sys_tz.tz_minuteswest*60;
        unix_date -= timezone;
#if 0
        *time = (unsigned short)((unix_date % 60)/2+(((unix_date/60) % 60) << 5)+
            (((unix_date/3600) % 24) << 11));
#else
        ud = (unsigned long) unix_date;
        *time = (unsigned short)((ud % 60)/2+(((ud/60) % 60) << 5)+
            (((ud/3600) % 24) << 11));
#endif
        day = unix_date/86400-3652;
        year = day/365;
        if ((year+3)/4+365*year > day) year--;
        day -= (year+3)/4+365*year;
        if (day == 59 && !(year & 3)) {
                nl_day = day;
                month = 2;
        }
        else {
                nl_day = (year & 3) || day <= 59 ? day : day-1;
                for (month = 0; month < 12; month++)
                        if (day_n[month] > nl_day) break;
        }
        *date = (unsigned short)(nl_day-day_n[month-1]+1+(month << 5)+(year << 9));
}
