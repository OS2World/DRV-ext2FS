#
# makefile:
#       makefile for src/classes directory.
#       For use with IBM NMAKE, which comes with the IBM compilers,
#       the Developer's Toolkit, and the DDK.
#
#       All the makefiles have been restructured with V0.9.0.
#
#       Called from:    main makefile
#
#       Input:          ./*.c
#
#       Output:         ../bin/*.obj
#
#       Edit "setup.in" to set up the make process.
#

# Say hello to yourself.
!if [@echo +++++ Entering $(MAKEDIR)]
!endif

# include setup (compiler options etc.)
!include ..\..\setup.in

# COMPONENT is the name of this subdirectory.
COMPONENT = fsd32

# FIXED MACROS
# ------------
#
# You probably need not change the following.
#

# Define the suffixes for files which NMAKE will work on.
# .SUFFIXES is a reserved NMAKE keyword ("pseudotarget") for
# defining file extensions that NMAKE will recognize in inference
# rules.

.SUFFIXES: .c .obj .dll .h .ih .rc .res

# OUTPUTDIR specifies the directory where all the output .OBJ
# files will be created in. $(XWP_OUTPUT_ROOT) is set by
# setup.in to point to \bin from the XWorkplace sources root,
# but this can be modified by setting an external environment
# variable. This approach has the advantage that
# 1) all build files are in a common dir tree and the entire
#    tree can be removed completely;
# 2) the build tree can be on a different physical drive for
#    speed.
OUTPUTDIR = $(EXT2_OUTPUT_ROOT)\$(COMPONENT)
!if [@echo       OUTPUTDIR is $(OUTPUTDIR)]
!endif

# create output directories
!if [@md $(EXT2_OUTPUT_ROOT) 2> NUL]
!endif
!if [@md $(OUTPUTDIR) 2> NUL]
!endif

# The OBJS macro contains all the .OBJ files which need to be
# created from the files in this directory.
# If you add a new source, add the corresponding .OBJ file here.
OBJS = \
#$(OUTPUTDIR)\fs_attribute.obj \
#$(OUTPUTDIR)\fs_name.obj \
$(OUTPUTDIR)\fileinfo.obj \
$(OUTPUTDIR)\fs32_allocatepagespace.obj \
$(OUTPUTDIR)\fs32_chdir.obj \
$(OUTPUTDIR)\fs32_chgfileptr.obj \
$(OUTPUTDIR)\fs32_close.obj \
$(OUTPUTDIR)\fs32_commit.obj \
$(OUTPUTDIR)\fs32_delete.obj \
$(OUTPUTDIR)\fs32_dopageio.obj \
$(OUTPUTDIR)\fs32_exit.obj \
$(OUTPUTDIR)\fs32_fileattribute.obj \
$(OUTPUTDIR)\fs32_fileinfo.obj \
$(OUTPUTDIR)\fs32_findclose.obj \
$(OUTPUTDIR)\fs32_findfirst.obj \
$(OUTPUTDIR)\fs32_findfromname.obj \
$(OUTPUTDIR)\fs32_findnext.obj \
$(OUTPUTDIR)\fs32_flushbuf.obj \
$(OUTPUTDIR)\fs32_fsctl.obj \
$(OUTPUTDIR)\fs32_fsinfo.obj \
$(OUTPUTDIR)\fs32_init.obj \
$(OUTPUTDIR)\fs32_ioctl.obj \
$(OUTPUTDIR)\fs32_mkdir.obj \
$(OUTPUTDIR)\fs32_mount.obj \
$(OUTPUTDIR)\fs32_move.obj \
$(OUTPUTDIR)\fs32_newsize.obj \
$(OUTPUTDIR)\fs32_opencreate.obj \
$(OUTPUTDIR)\fs32_openpagefile.obj \
$(OUTPUTDIR)\fs32_pathinfo.obj \
$(OUTPUTDIR)\fs32_read.obj \
$(OUTPUTDIR)\fs32_rmdir.obj \
$(OUTPUTDIR)\fs32_shutdown.obj \
$(OUTPUTDIR)\fs32_write.obj \

# The main target:
# If we're called from the main makefile, MAINMAKERUNNING is defined,
# and we'll set $(OBJS) as our targets (which will go on).
# Otherwise, we call the main makefile, which will again call ourselves later.
all:   \
!ifndef MAINMAKERUNNING
# we're not being called from main makefile: start main makefile
    callmainmake
    @echo ----- Leaving $(MAKEDIR)
!else
    $(OBJS)
#    $(OUTPUTDIR)\$(COMPONENT).lib
    @echo ----- Leaving $(MAKEDIR)
!endif

callmainmake:
    @echo $(MAKEDIR)\makefile: Recursing to main makefile.
    @cd ..\..
    @nmake
    @echo $(MAKEDIR)\makefile: Returned from main makefile. Done.

# The "dep" target: run fastdep on the sources.
dep:
    $(PROJECT_BASE_DIR)\tools\fastdep $(DEP_INCLUDE) *.c

# Defines needed for compiling stuff in here (asm and c)
# we use the shared defines from setup.in here
# that's the original makefile.ext2-os2
ADEFINES = $(SHARED_A_DEFINES)
CDEFINES = $(SHARED_C_DEFINES)

# main target is the lib file in our $(OUTPUTDIR) subdir,
# which forces the objects to be built.
# Note that the two special objects (start.asm and end.asm)
# are NOT linked into the lib.
$(OUTPUTDIR)\$(COMPONENT).lib: $(OBJS)
    !$(ILIB) /noextdictionary /nobrowse /q $@ -+$?;
# /noignorecase

# Now define inference rules for what to do with certain file
# types, based on their file extension.
# The syntax we need here is ".fromext.toext".
# So whenever NMAKE encounters a .toext file, it
# executes what we specify here.
# The ugly {} brackets are some awkward syntax for specifying
# files in other directories.

# Special macros used here: $(@B) is the current target w/out ext.

# -- assemble ASM files to .OBJ files
.asm{$(OUTPUTDIR)}.obj:
        @echo $(MAKEDIR)\makefile: Assembling $(@B).asm
        $(AS) $(AINCLUDES) $(ADEFINES) $< -Fo:$@ -Fl:$(@:.obj=.lst)

# -- compile C files to .OBJ files

.c.{$(OUTPUTDIR)}.obj:
        @echo $(MAKEDIR)\makefile: Compiling $(@B).c
!ifndef PRECH
# precompiled headers disabled:
        $(CC_RING0) $(CDEFINES) -I$(INCLUDE) /fa$(OUTPUTDIR)\$(@B).asm /Fo$(OUTPUTDIR)\$(@B).obj $(@B).c
!else
        $(CC_RING0) $(CDEFINES) /fi"$(PRECH)\$(@B).pch" /si"$(PRECH)\$(@B).pch" -I$(INCLUDE) /fa$(OUTPUTDIR)\$(@B).lst /Fo$(OUTPUTDIR)\$(@B).obj $(@B).c
!endif

# The dependencies (which sources and include files the objs depend on)
# have been automatically generated into the .depend file by "nmake dep".
# We can therefore simply include .depend.

!include .depend



