#Zon's generic makefile
#target all:
#	finds every .cpp file in the source directory tree.
#	if corresponding dependency files exist in the build dir tree, includes them.
#	if rebuild required:
#		compiles .o files and puts them in corresponding dirs in build dir tree
#		compiles dependency files and puts them in corresponding dirs in build dir tree
#		links the .exe
#
#target clean:
#	removes all the files created during building.
#	Does not delete directories.
#
#target veryclean
#	removes the entire build directory
#
#target test:
#	prints random debug information.

# build options:
#	completed binary goes here.
    BINARYDIR = ./
#	make searches for the sources here.
    SOURCEDIR = source/
#	temporary build files go here.
    BUILDDIR = build/
#	header files are found here.
    INCLUDEDIR = include/
#	libraries are found here.
  #  
# putting everything in the same directory is also acceptable.


#-lglew32 -lopengl32 -lmingw32

#poopcode follows:	
obj_as_cpp=$(patsubst $(BUILDDIR)%.o,$(SOURCEDIR)%.cpp,$1)
#kinda wish we didn't go back and forth with this
cpp_as_obj=$(patsubst $(SOURCEDIR)%.cpp,$(BUILDDIR)%.o,$1)
#recursive wildcard:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
SOURCE_FILES := $(sort $(strip $(call rwildcard,$(SOURCEDIR),*.cpp)))
DIRS := $(sort $(dir $(SOURCE_FILES)))
BUILD_DIRS := $(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(DIRS))
DIRTARGETS := $(addsuffix .dirstamp,$(BUILD_DIRS))
OBJ_FILES := $(call cpp_as_obj,$(SOURCE_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)
DEP_FILES_ACTUAL := $(call rwildcard,$(BUILDDIR),*.d)
#detecting Windows / Linux
ifneq (,$(findstring Windows,$(OS)))
   STFU=1> NUL 2>&1
   NOERROR=2> NULL
   makedir=mkdir $(patsubst %/,%,$(subst /,\,$1))
   delfile=del $(subst /,\,$1)
   deldir=rmdir /s $(subst /,\,$1)
   stamp=echo directory stamp: this tells "make" that "$1" exists. > $1.dirstamp
# compile options:
   PROGNAME = a.exe
   CFLAGS = -g -fmax-errors=1 -std=c++14 -Wno-narrowing
   CC = g++ $(CFLAGS) $(addprefix -I,$(INCLUDEDIR))
   LINK = g++
   LIBDIR = lib/ bin/
#   LIBS = $(addprefix -L,$(LIBDIR)) -lglew32 -lopengl32 -lmingw32 -lSDL2 -lSDL2main -lfreetype -lBulletDynamics -lBulletCollision -lBulletSoftBody -lLinearMath
   LIBS = $(addprefix -L,$(LIBDIR))
else
  OS := $(shell uname)
ifneq (,$(findstring Linux,$(OS)))
   STFU=1> /dev/null 2>&1
   NOERROR=2> /dev/null
   makedir=mkdir $1
   delfile=rm $1 -f
   deldir=rm $1 -f -r
   stamp=echo directory stamp: this tells "make" that "$1" exists. > $1.dirstamp
# compile options:
   PROGNAME = a
   CFLAGS = -g -fmax-errors=1 -std=c++14 -Wno-narrowing -fpermissive
   CC = g++ $(CFLAGS) $(addprefix -I,$(INCLUDEDIR))
   LINK = g++
#   LIBS = -lSDL2 -lSDL2main -lGL -lGLEW -lfreetype -lBulletDynamics -lBulletCollision -lBulletSoftBody -lLinearMath
else
  $(error $(OS) is not supported yet...)
endif
endif

MODE = NORMAL

.PHONY: all test clean cleansilent veryclean verycleansilent

ifeq ($(MODE),NORMAL)
all: $(DIRTARGETS) $(BINARYDIR)$(PROGNAME)
endif
ifeq ($(MODE),DIRS)
all: $(DIRTARGETS)
endif

test:
	@echo OS:
	@echo $(OS)
	@echo source files:
	@echo $(SOURCE_FILES)
	@echo object files:
	@echo $(OBJ_FILES)
	@echo dependency files:
	@echo $(DEP_FILES)
	@echo dirs:
	@echo $(DIRS)
	@echo build_dirs:
	@echo $(BUILD_DIRS)
	@echo dirtargets:
	@echo $(DIRTARGETS)
	@echo dirmode:
	@echo $(DIRMODE)

#this is probably a really bad way to do it
#but those mkdir and "error ignored" messages
#can go eat a bag of dicks
ifeq ($(MODE),NORMAL)
$(DIRTARGETS):
	@echo Rebuilding directory structure in $(BUILDDIR)
	@$(MAKE) MODE=DIRS $(STFU)
else
$(DIRTARGETS):
	@-$(call makedir,$(dir $@))
	@-$(call stamp,$(dir $@))
endif

clean:
	@$(MAKE) cleansilent $(STFU)

cleansilent:
	@-$(call delfile,$(OBJ_FILES) $(DEP_FILES))

veryclean:
	@echo Really delete $(BUILDDIR) ? (y/n)
	@$(MAKE) verycleansilent $(STFU)

verycleansilent:
	@-$(call deldir,$(BUILDDIR))

	
$(BINARYDIR)$(PROGNAME): $(OBJ_FILES) $(DEP_FILES)
	@echo linking $@
	@$(LINK) $(OBJ_FILES) $(LIBS) -o $(BINARYDIR)$(PROGNAME)
	@echo done
	
.SECONDEXPANSION:$(OBJ_FILES)
#$$(patsubst build/%.o,source/%.cpp,$$(patsubst build/%.d,build/%.o,$$@))
dep_as_obj=$(1:.d=.o)

$(DEP_FILES):
	@echo building $@
	@$(CC) $(call obj_as_cpp,$(call dep_as_obj,$@)) -o $(call dep_as_obj,$@) -c -MMD


$(OBJ_FILES): $$(call obj_as_cpp,$$@)
	@echo building $@
	@$(CC) $< -o $@ -c -MMD
	
include $(DEP_FILES_ACTUAL)


