#!/usr/bin/make
#

# paths

prefix ?= `pwd`/..
PREFIX ?= $(prefix)
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/data
LOCALEDIR ?= $(PREFIX)/locale
INST_DIR = $(DESTDIR)$(PREFIX)
INST_BINDIR = $(DESTDIR)$(BINDIR)
INST_DATADIR = $(DESTDIR)$(DATADIR)
INST_LOCALEDIR = $(DESTDIR)$(LOCALEDIR)

# mkconfig settings

MKC_PREFIX = bdj
MKC_CONFDIR = mkc_config
MKC_FILES = mkc_files
MKC_OUTPUT = bdjconfig.h
MKC_ENV_SHR = $(MKC_PREFIX)-shared.env

MKC_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX).mkc
MKC_ENV_SHR_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env-shared.mkc

OBJ_EXT = .o
EXE_EXT =

# mkconfig variables

MKC_DIR = ../mkconfig
MKCONFIG_TYPE = sh
MKC_REQLIB = $(MKC_PREFIX).reqlibs
#MKC_ECHO =
MKC_ECHO = -e

CFLAGS_DEBUG = -O0 -g

# 

.PHONY: all rall
all: $(MKC_ENV_SHR) 
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall

rall: $(LIBRARIES) $(TESTPROGRAMS) $(PROGRAMS) 
	$(MAKE) -e libraries
	$(MAKE) -e testprograms
	$(MAKE) -e programs

.PHONY: install rinstall
install:  $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rinstall

rinstall: 
	@$(MAKE) -e rall
	cp -f $(LIBRARIES) $(TESTPROGRAMS) $(PROGRAMS) $(INST_BINDIR)

# main targets

PROGRAMS = bdj4main$(EXE_EXT)
.PHONY: programs
programs: $(MKC_ENV_SHR) $(MKC_REQLIB) $(PROGRAMS)

TESTPROGRAMS = check_all$(EXE_EXT)
.PHONY: testprograms
testprograms: $(MKC_ENV_SHR) $(MKC_REQLIB) $(TESTPROGRAMS)

LIBRARIES = libbdj$(SHLIB_EXT)
.PHONY: libraries
libraries: $(MKC_ENV_SHR) $(MKC__REQLIB) $(LIBRARIES)

# mkconfig targets

$(MKC_ENV_SHR): $(MKC_ENV_SHR_CONF)
	@-$(RM) -f $(MKC_ENV_SHR)
	CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_SHR_CONF)

$(MKC_REQLIB):  $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB) $(MKC_OUTPUT)

$(MKC_OUTPUT):       $(MKC_ENV_SHR) $(MKC_CONF)
	@-$(RM) -f $(MKC_OUTPUT)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh $(MKC_CONF)

# compilation

.c$(OBJ_EXT):
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -compile $(MKC_ECHO) \
		-shared $<

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) clean
	@-$(RM) -f $(MKC_ENV_SHR) $(MKC_REQLIB)
	@-$(RM) -f $(MKC_OUTPUT)
	@-$(RM) -rf $(MKC_FILES)

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(RM) -f *.o *.so *.dll *.dylib *.exe
	@-$(RM) -f ../bin/*.o ../bin/*.so ../bin/*.dll ../bin/*.dylib \
		../bin/*.exe

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w *.orig 
	@-$(RM) -f $(MKC_FILES)/mkc_compile.log $(MKC_CONFDIR)/*~
	@-$(RM) -f ../tmp/* ../conv/*~ ../locale/*~

# bdj executables

BDJMAINOBJS = bdj4main$(OBJ_EXT) \
	libbdj$(SHLIB_EXT)
bdj4main$(EXE_EXT): $(BDJMAINOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o bdj4main$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJMAINOBJS) \
		-L . -rpath $(BINDIR) \
		-l bdj \
		$(LIBS)


# bdj libraries

BDJOBJS = bdjstring$(OBJ_EXT) list$(OBJ_EXT) lock$(OBJ_EXT) \
	musicdb$(OBJ_EXT) parse$(OBJ_EXT) \
	rafile$(OBJ_EXT) song$(OBJ_EXT) sysvars$(OBJ_EXT) tagdef$(OBJ_EXT)
libbdj$(SHLIB_EXT): $(MKC_REQLIB) $(BDJOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -shared $(MKC_ECHO) -lib \
		-o libbdj$(SHLIB_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJOBJS) \
		$(LIBS)

# bdj source

bdj4main$(OBJ_EXT): bdj4main.c musicdb.h tagdef.h $(MKC_OUTPUT)

bdjstring$(OBJ_EXT): bdjstring.c bdjstring.h $(MKC_OUTPUT)

list$(OBJ_EXT): list.c list.h $(MKC_OUTPUT)

lock$(OBJ_EXT): lock.c lock.h $(MKC_OUTPUT)

musicdb$(OBJ_EXT): musicdb.c musicdb.h rafile.h song.h \
	parse.h tagdef.h $(MKC_OUTPUT)

parse$(OBJ_EXT): parse.c parse.h $(MKC_OUTPUT)

rafile$(OBJ_EXT): rafile.c rafile.h lock.h $(MKC_OUTPUT)

song$(OBJ_EXT): song.c song.h $(MKC_OUTPUT)

sysvars$(OBJ_EXT): sysvars.c sysvars.h $(MKC_OUTPUT)

tagdef$(OBJ_EXT): tagdef.c tagdef.h $(MKC_OUTPUT)

# test routines

CHECKOBJS = check_all$(OBJ_EXT) check_list$(OBJ_EXT) \
	check_lock$(OBJ_EXT) check_parse$(OBJ_EXT) check_rafile$(OBJ_EXT) \
	libbdj$(SHLIB_EXT)
CHECKLIBS =
check_all$(EXE_EXT): $(CHECKOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o check_all$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(CHECKOBJS) \
		-L . -rpath $(BINDIR) \
		-l bdj \
		$(CHECKLIBS) \
		$(LIBS)


check_list$(OBJ_EXT): check_list.c list.h check_bdj.h

check_lock$(OBJ_EXT): check_lock.c lock.h check_bdj.h

check_parse$(OBJ_EXT): check_parse.c parse.h check_bdj.h

check_rafile$(OBJ_EXT): check_rafile.c rafile.h check_bdj.h

check_all$(OBJ_EXT): check_all.c check_bdj.h
