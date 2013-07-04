
#
# makefile:
#       makefile for main directory.
#       For use with IBM NMAKE, which comes with the IBM compilers,
#       the Developer's Toolkit, and the DDK.
#
#       Required environment variables:
#
#       CVS_WORK_ROOT:
#           must point to the cvs root, the parent
#           of this directory (e.g. K:\cvs if this dir is
#           k:\cvs\ext2os2).
#
#       COPY_TO:
#           where to copy the output IFS to.
#
#       TKPATH:
#           toolkit base directory (e.g. K:\libs\Toolkit45)
#
#       DDKPATH:
#           ddk base directory (e.g. K:\libs\DDK\base)
#
#       EXT2_DEBUG:
#           optional -- if defined to anything, produce
#           debug code.
#
#       Edit "setup.in" to set up the make process (compilation flags etc.).
#

# Say hello to yourself.
!if [@echo +++++ Entering $(MAKEDIR)]
!endif

# include setup (compiler options etc.)
!include setup.in

# MODULESDIR is used for mapfiles and final module (DLL, EXE) output.
# PROJECT_OUTPUT_DIR has been set by setup.in based on the environment.
MODULESDIR=$(PROJECT_OUTPUT_DIR)\modules
!if [@echo ---^> MODULESDIR is $(MODULESDIR)]
!endif

OUTPUTDIR = $(PROJECT_OUTPUT_DIR)

!ifdef EXT2_DEBUG
!if [@echo ---^> Debug build has been enabled.]
!endif
!else
!if [@echo ---^> Building release code (debugging disabled).]
!endif
!endif

# create output directory
!if [@md $(PROJECT_OUTPUT_DIR) 2> NUL]
!endif
!if [@md $(MODULESDIR) 2> NUL]
!endif

# Define the suffixes for files which NMAKE will work on.
# .SUFFIXES is a reserved NMAKE keyword ("pseudotarget") for
# defining file extensions that NMAKE will recognize in inference
# rules.

.SUFFIXES: .obj .dll .exe .h .rc .res

# some variable strings to pass to sub-nmakes
SUBMAKE_PASS_STRING = "PROJECT_BASE_DIR=$(PROJECT_BASE_DIR)" "PROJECT_INCLUDE=$(PROJECT_INCLUDE)"

# store current directory so we can change back later
CURRENT_DIR = $(MAKEDIR)

# OBJECT FILES TO BE LINKED
# -------------------------

HLPOBJS = \
$(OUTPUTDIR)\devhlp32\DevHlp32_VMLock.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_VMUnLock.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_VirtToLin.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_GetDosVar.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_Security.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_AttachToDD.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_AllocOneGDTSelector.obj\
$(OUTPUTDIR)\devhlp32\DevHlp32_FreeGDTSelector.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_SaveMessage.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_setIRQ.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_UnSetIRQ.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_EOI.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_InternalError.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_VMAlloc.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_VMFree.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_ProcBlock.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_ProcRun.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_LinToPageList.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_PageListToLin.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_yield.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_GetInfoSegs.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_VerifyAccess.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_OpenEventSem.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_CloseEventSem.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_PostEventSem.obj \
$(OUTPUTDIR)\devhlp32\DevHlp32_ResetEventSem.obj \
# $(OUTPUTDIR)\util\sec32_attach_ses.obj \
#$(OUTPUTDIR)\devhlp32\DevHlp32_AttachDD.obj \
# $(OUTPUTDIR)\devhlp32\DevHlp32_AllocGDTSelector.obj \
$(OUTPUTDIR)\fsh32\fsh32_addshare.obj \
$(OUTPUTDIR)\fsh32\fsh32_devioctl.obj \
$(OUTPUTDIR)\fsh32\fsh32_dovolio.obj \
$(OUTPUTDIR)\fsh32\fsh32_findduphvpb.obj \
$(OUTPUTDIR)\fsh32\fsh32_forcenoswap.obj \
$(OUTPUTDIR)\fsh32\fsh32_getvolparm.obj \
$(OUTPUTDIR)\fsh32\fsh32_ioboost.obj \
$(OUTPUTDIR)\fsh32\fsh32_qsysinfo.obj \
$(OUTPUTDIR)\fsh32\fsh32_removeshare.obj \
$(OUTPUTDIR)\fsh32\fsh32_segalloc.obj \
$(OUTPUTDIR)\fsh32\fsh32_setvolume.obj \
$(OUTPUTDIR)\util\atol.obj \
$(OUTPUTDIR)\util\attr.obj \
$(OUTPUTDIR)\util\ctype.obj \
$(OUTPUTDIR)\util\datetime.obj \
$(OUTPUTDIR)\util\fnmatch.obj \
$(OUTPUTDIR)\util\isfat.obj \
$(OUTPUTDIR)\util\pathutil.obj \
$(OUTPUTDIR)\util\sec32_attach_ses.obj \
$(OUTPUTDIR)\util\strnicmp.obj \
$(OUTPUTDIR)\util\strpbrk.obj \
$(OUTPUTDIR)\util\strtok.obj \
$(OUTPUTDIR)\util\strtol.obj \
$(OUTPUTDIR)\util\strtoul.obj \
$(OUTPUTDIR)\util\strupr.obj \
$(OUTPUTDIR)\util\util.obj \
$(OUTPUTDIR)\util\vsprintf.obj \

IFSOBJS = \
$(OUTPUTDIR)\interface\fs_init.obj \
$(OUTPUTDIR)\interface\fs_thunks.obj \
$(OUTPUTDIR)\fsd32\fs32_allocatepagespace.obj \
$(OUTPUTDIR)\fsd32\fs32_chdir.obj \
$(OUTPUTDIR)\fsd32\fs32_chgfileptr.obj \
$(OUTPUTDIR)\fsd32\fs32_close.obj \
$(OUTPUTDIR)\fsd32\fs32_commit.obj \
$(OUTPUTDIR)\fsd32\fs32_delete.obj \
$(OUTPUTDIR)\fsd32\fs32_dopageio.obj \
$(OUTPUTDIR)\fsd32\fs32_init.obj \
$(OUTPUTDIR)\fsd32\fs32_exit.obj \
$(OUTPUTDIR)\fsd32\fs32_fileattribute.obj \
$(OUTPUTDIR)\fsd32\fs32_fileinfo.obj \
$(OUTPUTDIR)\fsd32\fs32_findclose.obj \
$(OUTPUTDIR)\fsd32\fs32_findfirst.obj \
$(OUTPUTDIR)\fsd32\fs32_findfromname.obj \
$(OUTPUTDIR)\fsd32\fs32_findnext.obj \
$(OUTPUTDIR)\fsd32\fs32_flushbuf.obj \
$(OUTPUTDIR)\fsd32\fs32_fsctl.obj \
$(OUTPUTDIR)\fsd32\fs32_fsinfo.obj \
$(OUTPUTDIR)\fsd32\fs32_ioctl.obj \
$(OUTPUTDIR)\fsd32\fs32_mkdir.obj \
$(OUTPUTDIR)\fsd32\fs32_mount.obj \
$(OUTPUTDIR)\fsd32\fs32_move.obj \
$(OUTPUTDIR)\fsd32\fs32_newsize.obj \
$(OUTPUTDIR)\fsd32\fs32_opencreate.obj \
$(OUTPUTDIR)\fsd32\fs32_openpagefile.obj \
$(OUTPUTDIR)\fsd32\fs32_pathinfo.obj \
$(OUTPUTDIR)\fsd32\fs32_read.obj \
$(OUTPUTDIR)\fsd32\fs32_rmdir.obj \
$(OUTPUTDIR)\fsd32\fs32_shutdown.obj \
$(OUTPUTDIR)\fsd32\fs32_write.obj \
$(OUTPUTDIR)\fsd32\fileinfo.obj \
$(OUTPUTDIR)\vfs\file_table.obj \
$(OUTPUTDIR)\vfs\inode.obj \
$(OUTPUTDIR)\vfs\buffer.obj \
$(OUTPUTDIR)\vfs\read_write.obj \
$(OUTPUTDIR)\vfs\request_list.obj \
$(OUTPUTDIR)\vfs\dcache.obj \
$(OUTPUTDIR)\vfs\misc.obj \
$(OUTPUTDIR)\vfs\ll_rw_block.obj \
$(OUTPUTDIR)\vfs\namei.obj \
$(OUTPUTDIR)\vfs\pageio.obj \
$(OUTPUTDIR)\vfs\super.obj \
$(OUTPUTDIR)\vfs\bitops.obj \
$(OUTPUTDIR)\vfs\minifsd.obj \
$(OUTPUTDIR)\vfs\strategy2.obj \
$(OUTPUTDIR)\ext2\balloc.obj \
$(OUTPUTDIR)\ext2\bitmap.obj \
$(OUTPUTDIR)\ext2\dir.obj \
$(OUTPUTDIR)\ext2\file.obj \
$(OUTPUTDIR)\ext2\fsync.obj \
$(OUTPUTDIR)\ext2\ialloc.obj \
$(OUTPUTDIR)\ext2\inode.obj \
$(OUTPUTDIR)\ext2\ioctl.obj \
$(OUTPUTDIR)\ext2\namei.obj \
$(OUTPUTDIR)\ext2\super.obj \
$(OUTPUTDIR)\ext2\truncate.obj \

MISCOBJS = \
$(OUTPUTDIR)\misc\misc_data.obj \
$(OUTPUTDIR)\misc\maperr.obj \
$(OUTPUTDIR)\misc\panic.obj \
$(OUTPUTDIR)\misc\files.obj \
$(OUTPUTDIR)\misc\case.obj \
$(OUTPUTDIR)\misc\trace.obj \
$(OUTPUTDIR)\misc\volume.obj \
$(OUTPUTDIR)\misc\banner.obj \
$(OUTPUTDIR)\misc\log.obj

STARTOBJ = \
$(OUTPUTDIR)\interface\start.obj

ENDOBJ = \
$(OUTPUTDIR)\interface\end.lib
# note that end.LIB is used here instead of end.obj...
# os2bird found out how to fix the stupid linker problems.

# PSEUDOTARGETS
# -------------

# If you add a subdirectory to SRC\, add a target to
# "SUBDIRS" also to have automatic recompiles.

all: compile link
    @echo ----- Leaving $(MAKEDIR)

compile:
    @cd src
    $(MAKE) -nologo "SUBTARGET=all"
    @cd $(CURRENT_DIR)
    @echo ----- Leaving $(MAKEDIR)

dep:
    @cd src
    $(MAKE) -nologo "SUBTARGET=dep"
    @cd $(CURRENT_DIR)
    @echo ----- Leaving $(MAKEDIR)

LINK_OBJS = $(IFSOBJS) $(MISCOBJS) $(HLPOBJS)


# LINKER PSEUDOTARGETS
# --------------------

link: \
$(MODULESDIR)\ext2-os2.ifs


$(MODULESDIR)\ext2-os2.ifs: ext2-os2.def $(STARTOBJ) $(LINK_OBJS) $(ENDOBJ) makefile
# /exepack:2
        $(LINK) /nod /NOE /nopackc /nopackd /align:2 /OUT:$(@R).dll @<<
$(STARTOBJ) \
$(LINK_OBJS) \
os2386.lib \
$(DDKPATH)\lib\os2386p.lib \
lib\FSHELPER.LIB \
vdh.lib \
CPPON30.lib \
$(ENDOBJ) \
ext2-os2.def
/MAP:$(@R).map \
<<KEEP
        @-del $(@R).ifs
        ren $(@R).dll $(@B).ifs
        mapsym -l $(@:.ifs=.map)
        copy $(@B).sym $(@R).sym
        del $(@B).sym
!ifdef COPY_TO
        copy $(@R).ifs $(COPY_TO)
        copy $(@R).sym $(COPY_TO)
!endif


# TRANSFER
# --------

transfer: all
    sendfile p75 $(MODULESDIR)\ext2-os2.ifs C:\ext2-os2.ifs
    sendfile p75 $(MODULESDIR)\ext2-os2.sym C:\ext2-os2.sym

copyb: all
    zip -9X $(TEMP)\ext2.zip $(MODULESDIR)\ext2-os2.ifs $(MODULESDIR)\ext2-os2.sym
    cmd.exe /c copy $(TEMP)\ext2.zip "B:\"


