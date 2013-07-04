
 By Matthieu Willm:

 The purpose of the VFSAPI library is to provide an extension of
 the standard OS/2 standard file system oriented calls to access
 Linux VFS specific features like I-nodes, UNIX file modes, file
 ownership and so on. For the moment the library is quite limited,
 but with the time I'll include some more calls !

 To be able to do what they are designed for, these routines talk
 directly to the ext2-os2.ifs file system driver, through the
 DosFsCtl call. DosFsCtl (almost) directly calls the IFS's FS_FSCTL
 entry point. Most of the work is done in kernel mode inside
 ext2-os2.ifs. DosFsCtl is the IFS equivalent of DosDevIOCtl: an
 extended interface to the IFS.

 The library is compiled using IBM Visualage C++ 3.0, but it should
 also compile without problem with emx/gcc. The precompiled
 vfsapi.dll should work with emx/gcc (and other compilers) without
 recompilation.

 The entry points follow the standard OS/2 32 bits API linkage
 convention (_System for IBM Visualage C++, _syscall for Borland C++).

 Note:  The funtion names, as well as data structure names, have been
        prefixed with "vfs_" in order to prevent name collisions with
        existing C/C++ runtime libraries.



