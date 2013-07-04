
#
#   SUBTARGET is defined by the main makefile to be one of:
#
#       -- all: this will invoke "nmake all" on the subdirs.
#
#       -- dep: this will invoke "nmake dep" on the subdirs.
#

# Say hello to yourself.
!if [@echo +++++ Entering $(MAKEDIR)]
!endif

# store current directory so we can change back later
CURRENT_DIR = $(MAKEDIR)

SUBDIRS = interface devhlp32 ext2 fsd32 fsh32 misc util vfs vfsapi

all: $(SUBDIRS)
    @echo ----- Leaving $(MAKEDIR)

interface:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

devhlp32:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

ext2:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

fsd32:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

fsh32:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

misc:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

mwdd32:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

util:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

vfs:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

vfsapi:
    @echo $(MAKEDIR)\makefile: Going for subdir src\$@
    @cd $@
    @$(MAKE) -nologo $(SUBTARGET) "MAINMAKERUNNING=YES"
    @cd $(CURRENT_DIR)

