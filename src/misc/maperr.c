//
// $Header: e:\\netlabs.cvs\\ext2fs/src/misc/maperr.c,v 1.1 2001/05/09 17:49:50 umoeller Exp $
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

#include <linux\errno.h>
#include <os2\types.h>
#include <os2\os2proto.h>

int Linux_to_OS2_err[] = {
              NO_ERROR,           //        0  : No error !
              ERROR_ACCESS_DENIED,    // EPERM  1  : Operation not permitted
              ERROR_FILE_NOT_FOUND,   // ENOENT 2  : No such file or directory
              ERROR_INVALID_PROCID,   // ESRCH  3  : No such process
              ERROR_INTERRUPT,    // EINTR  4  : Interrupted system call
              ERROR_READ_FAULT,   // EIO    5  : I/O error
              -1,             // ENXIO  6  : No such device or address
              -1,             // E2BIG  7  : Arg list too long
              ERROR_BAD_EXE_FORMAT,   // ENOEXEC    8  : Exec format error
              ERROR_INVALID_HANDLE,   // EBADF  9  : Bad file number
              ERROR_NO_CHILD_PROCESS, // ECHILD 10 : No child processes
              -1,             // EAGAIN 11 : Try again
              ERROR_NOT_ENOUGH_MEMORY, // ENOMEM    12 : Out of memory
              ERROR_ACCESS_DENIED,    // EACCES 13 : Permission denied
              ERROR_INVALID_ADDRESS,  // EFAULT 14 : Bad address
              -1,             // ENOTBLK    15 : Block device required
              ERROR_DEVICE_IN_USE,    // EBUSY  16 : Device or resource busy
              ERROR_FILE_EXISTS,      // EEXIST 17 : File exists
              -1,             // EXDEV  18 : Cross-device link
              ERROR_BAD_UNIT,     // ENODEV 19 : No such device
              ERROR_PATH_NOT_FOUND,   // ENOTDIR    20 : Not a directory
              ERROR_DIRECTORY,    // EISDIR 21 : Is a directory
              ERROR_INVALID_PARAMETER, // EINVAL    22 : Invalid argument
              ERROR_NO_MORE_FILES,    // ENFILE 23 : File table overflow
              ERROR_NO_MORE_FILES,    // EMFILE 24 : Too many open files
              -1,             // ENOTTY 25 : Not a typewriter
              -1,             // ETXTBSY    26 : Text file busy
              ERROR_SEEK,             // EFBIG  27 : File too large
              ERROR_DISK_FULL,    // ENOSPC 28 : No space left on device
              ERROR_SEEK_ON_DEVICE,   // ESPIPE 29 : Illegal seek
              ERROR_WRITE_PROTECT,    // EROFS  30 : Read-only file system
              -1,             // EMLINK 31 : Too many links
              ERROR_BROKEN_PIPE,      // EPIPE  32 : Broken pipe
              -1,             // EDOM   33 : Math argument out of domain of func
              -1,             // ERANGE 34 : Math result not representable
              -1,             // EDEADLK    35 : Resource deadlock would occur
              ERROR_FILENAME_EXCED_RANGE, // ENAMETOOLONG   36 : File name too long
              -1,             // ENOLCK 37 : No record locks available
              ERROR_NOT_SUPPORTED,    // ENOSYS 38 : Function not implemented
              ERROR_DIR_NOT_EMPTY,    // ENOTEMPTY  39 : Directory not empty
              ERROR_CIRCULARITY_REQUESTED, // ELOOP 40 : Too many symbolic links encountered
              -1              // EWOULDBLOCK EAGAIN Operation would block
};

#if 0
#define ENOMSG      42  /* No message of desired type */
#define EIDRM       43  /* Identifier removed */
#define ECHRNG      44  /* Channel number out of range */
#define EL2NSYNC    45  /* Level 2 not synchronized */
#define EL3HLT      46  /* Level 3 halted */
#define EL3RST      47  /* Level 3 reset */
#define ELNRNG      48  /* Link number out of range */
#define EUNATCH     49  /* Protocol driver not attached */
#define ENOCSI      50  /* No CSI structure available */
#define EL2HLT      51  /* Level 2 halted */
#define EBADE       52  /* Invalid exchange */
#define EBADR       53  /* Invalid request descriptor */
#define EXFULL      54  /* Exchange full */
#define ENOANO      55  /* No anode */
#define EBADRQC     56  /* Invalid request code */
#define EBADSLT     57  /* Invalid slot */
#define EDEADLOCK   58  /* File locking deadlock error */
#define EBFONT      59  /* Bad font file format */
#define ENOSTR      60  /* Device not a stream */
#define ENODATA     61  /* No data available */
#define ETIME       62  /* Timer expired */
#define ENOSR       63  /* Out of streams resources */
#define ENONET      64  /* Machine is not on the network */
#define ENOPKG      65  /* Package not installed */
#define EREMOTE     66  /* Object is remote */
#define ENOLINK     67  /* Link has been severed */
#define EADV        68  /* Advertise error */
#define ESRMNT      69  /* Srmount error */
#define ECOMM       70  /* Communication error on send */
#define EPROTO      71  /* Protocol error */
#define EMULTIHOP   72  /* Multihop attempted */
#define EDOTDOT     73  /* RFS speificerror */
#define EBADMSG     74  /* Not a data message */
#define EOVERFLOW   75  /* Value too large for defined data type */
#define ENOTUNIQ    76  /* Name not unique on network */
#define EBADFD      77  /* File descriptor in bad state */
#define EREMCHG     78  /* Remote address changed */
#define ELIBACC     79  /* Can not access a needed shared library */
#define ELIBBAD     80  /* Accessing a corrupted shared library */
#define ELIBSCN     81  /* .lib section in a.out corrupted */
#define ELIBMAX     82  /* Attempting to link in too many shared libraries */
#define ELIBEXEC    83  /* Cannot exec a shared library directly */
#define EILSEQ      84  /* Illegal byte sequence */
#define ERESTART    85  /* Interrupted system call should be restarted */
#define ESTRPIPE    86  /* Streams pipe error */
#define EUSERS      87  /* Too many users */
#define ENOTSOCK    88  /* Socket operation on non-socket */
#define EDESTADDRREQ    89  /* Destination address required */
#define EMSGSIZE    90  /* Message too long */
#define EPROTOTYPE  91  /* Protocol wrong type for socket */
#define ENOPROTOOPT 92  /* Protocol not available */
#define EPROTONOSUPPORT 93  /* Protocol not supported */
#define ESOCKTNOSUPPORT 94  /* Socket type not supported */
#define EOPNOTSUPP  95  /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT    96  /* Protocol family not supported */
#define EAFNOSUPPORT    97  /* Address family not supported by protocol */
#define EADDRINUSE  98  /* Address already in use */
#define EADDRNOTAVAIL   99  /* Cannot assign requested address */
#define ENETDOWN    100 /* Network is down */
#define ENETUNREACH 101 /* Network is unreachable */
#define ENETRESET   102 /* Network dropped connection because of reset */
#define ECONNABORTED    103 /* Software caused connection abort */
#define ECONNRESET  104 /* Connection reset by peer */
#define ENOBUFS     105 /* No buffer space available */
#define EISCONN     106 /* Transport endpoint is already connected */
#define ENOTCONN    107 /* Transport endpoint is not connected */
#define ESHUTDOWN   108 /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS    109 /* Too many references: cannot splice */
#define ETIMEDOUT   110 /* Connection timed out */
#define ECONNREFUSED    111 /* Connection refused */
#define EHOSTDOWN   112 /* Host is down */
#define EHOSTUNREACH    113 /* No route to host */
#define EALREADY    114 /* Operation already in progress */
#define EINPROGRESS 115 /* Operation now in progress */
#define ESTALE      116 /* Stale NFS file handle */
#define EUCLEAN     117 /* Structure needs cleaning */
#define ENOTNAM     118 /* Not a XENIX named type file */
#define ENAVAIL     119 /* No XENIX semaphores available */
#define EISNAM      120 /* Is a named type file */
#define EREMOTEIO   121 /* Remote I/O error */
#define EDQUOT      122 /* Quota exceeded */
#endif // 0

int map_err(int err) {
    int mapped;

    if (err < 0) {
        err = -err;
    }

    if (err >= sizeof(Linux_to_OS2_err) / sizeof(int)) {
    kernel_printf("map_err() : Linux error %d cannot be mapped to OS/2 error !", err);
        return err;
    }

    if ((mapped = Linux_to_OS2_err[err]) == -1) {
    kernel_printf("map_err() : Linux error %d cannot be mapped to OS/2 error !", err);
    return err;
    }
    return mapped;
}
