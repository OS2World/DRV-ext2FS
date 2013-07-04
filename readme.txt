
ext2fs Source README
(W) Ulrich M”ller, May 9, 2001


1. LICENSE, COPYRIGHT, DISCLAIMER
=================================

    Copyright (C) 1995-1997 Matthieu Willm and others.
    Copyright (C) 2001 Ulrich M”ller.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as contained in
    the file COPYING in this distribution.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


2. INTRODUCTION
===============

    The Netlabs "ext2fs" project is an attempt to get the existing
    ext2fs port for OS/2 to run on LVM machines as well.

    While ext2-os2 (which was the original name of the ext2fs port
    to OS/2) is very stable on non-LVM machines, it hangs on boot
    if LVM is installed. This applies to all LVM machines, be they
    Warp 4 with LVM added manually, WSeB, eCS, MCP, or whatever.


3. CURRENT STATUS
=================

    Basically, this is still the original ext2fs port. However,
    the following changes were made:

    1)  The build tree was reorganized to have the standard
        include and src directories, and all produces binaries
        are put into a newly created bin/ tree.

    2)  All references to mwdd32.sys were removed. In other
        words, mwdd32.sys is no longer required to run this
        IFS, and all mwdd32 code (such as the DevHlp32's)
        is now linked statically.

    3)  This required a lot of changes to the initialization
        routines (the FS_INIT callout). Since FS_INIT is the
        only export which gets called on ring 3 by the OS/2
        kernel, the old port used mwdd32.sys to implement a
        hack so that the 32-bits implementation ran at ring 0.

        This hack has been removed, which required a rewrite
        of the init code, which is now mostly in assembler.

    Presently, the IFS crashes on FS_MOUNT. I am presently working
    on getting these issues resolved. This means that the IFS
    is not useable; it has only been put onto CVS so that other
    people could take a look at it.


4. DIRECTORIES
==============

    In src/, we have:

    --  devhlp32, the 32-bit device helper code formerly in
        mwdd32.sys.

    --  ext2: the actual file system code, ported from Linux.

    --  fsd32: the 32-bit implementation code for the IFS FS_*
        exports, called from the 16-bit thunks in interface.

    --  fsh32: 32-bit file-system helper code, formerly in
        the mwdd32 source tree.

    --  interface: the 16-bit FS_* exports, in assembler. This
        calls the code in fsd32 via thunks.

    --  misc: well, miscellanous code.

    --  util: utility code which is lacking in the VAC subsystem
        library.

    --  vfs: the Linux VFS layer.

    --  vfsapi: ring-3 library code.


5. HOW TO BUILD
===============

    This should compile and link with VAC 3.08 and ALP from the
    Toolkit 3. The assembler code uses some stuff which apparently
    doesn't work with the newer ALP's (e.g. from TK 4.5); I am
    looking into this.

    I had compiler crashes ("internal compiler error") when
    using VAC 3.6.5, but YYMV.

    In any case, a 16-bit C compiler is NOT needed.

    To build:

    1)  Take a look at the top of the main makefile and set the
        environment variables mentioned there.

        I will later add a configure.cmd to make this easier.

    2)  Run "nmake dep" to build the dependencies (this uses the
        fastdep.exe from tools/).

    3)  Run NMAKE. You should then find the IFS in bin/modules.


Ulrich M”ller.

