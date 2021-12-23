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

OBJDIR = objects
SHLIBDIR = $(BINDIR)

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
	PKG_CONFIG_PATH=../plocal/lib/pkgconfig $(MAKE) -e testprograms
	$(MAKE) -e programs

.PHONY: install rinstall
install:  $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rinstall

rinstall:
	@$(MAKE) -e rall
	(cd locale;./install.sh)

# main targets

PROGRAMS = $(BINDIR)/bdj4$(EXE_EXT)
.PHONY: programs
programs: $(MKC_ENV_SHR) $(MKC_REQLIB) $(PROGRAMS)

TESTPROGRAMS = $(BINDIR)/check_all$(EXE_EXT)
.PHONY: testprograms
testprograms: $(MKC_ENV_SHR) $(MKC_REQLIB) $(TESTPROGRAMS)

LIBRARIES = $(SHLIBDIR)/libbdj$(SHLIB_EXT)
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

$(OBJDIR)/%.o: %.c
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -compile $(MKC_ECHO) \
		-o $@ -shared $<

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
	@-$(RM) -f $(OBJDIR)/*
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w *.orig
	@-$(RM) -f $(MKC_FILES)/mkc_compile.log $(MKC_CONFDIR)/*~
	@-$(RM) -f ../tmp/* ../conv/*~ ../locale/*~

# bdj executables

BDJMAINOBJS = $(OBJDIR)/bdj4$(OBJ_EXT) \
	$(SHLIBDIR)/libbdj$(SHLIB_EXT)

$(BINDIR)/bdj4$(EXE_EXT): $(BDJMAINOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o $(BINDIR)/bdj4$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJMAINOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj \
		$(LIBS)


# bdj libraries

BDJOBJS = $(OBJDIR)/bdj4main$(OBJ_EXT) \
	$(OBJDIR)/bdjstring$(OBJ_EXT) \
	$(OBJDIR)/datafile$(OBJ_EXT) \
	$(OBJDIR)/fileutil$(OBJ_EXT) \
	$(OBJDIR)/list$(OBJ_EXT) \
	$(OBJDIR)/lock$(OBJ_EXT) \
	$(OBJDIR)/log$(OBJ_EXT) \
	$(OBJDIR)/musicdb$(OBJ_EXT) \
	$(OBJDIR)/process$(OBJ_EXT) \
	$(OBJDIR)/rafile$(OBJ_EXT) \
	$(OBJDIR)/sock$(OBJ_EXT) \
	$(OBJDIR)/song$(OBJ_EXT) \
	$(OBJDIR)/sysvars$(OBJ_EXT) \
	$(OBJDIR)/tagdef$(OBJ_EXT) \
	$(OBJDIR)/tmutil$(OBJ_EXT)

$(SHLIBDIR)/libbdj$(SHLIB_EXT): $(MKC_REQLIB) $(BDJOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -shared $(MKC_ECHO) -lib \
		-o $(SHLIBDIR)/libbdj$(SHLIB_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJOBJS) \
		$(LIBS)

# bdj source

$(OBJDIR)/bdj4$(OBJ_EXT): bdj4.c bdj4main.h $(MKC_OUTPUT)

$(OBJDIR)/bdj4main$(OBJ_EXT): bdj4main.c bdj4main.h musicdb.h \
	tagdef.h bdjstring.h \
	sysvars.h log.h $(MKC_OUTPUT)

$(OBJDIR)/bdjstring$(OBJ_EXT): bdjstring.c bdjstring.h $(MKC_OUTPUT)

$(OBJDIR)/datafile$(OBJ_EXT): datafile.c datafile.h tmutil.h \
	list.h fileutil.h $(MKC_OUTPUT)

$(OBJDIR)/fileutil$(OBJ_EXT): fileutil.c fileutil.h sysvars.h $(MKC_OUTPUT)

$(OBJDIR)/list$(OBJ_EXT): list.c list.h $(MKC_OUTPUT)

$(OBJDIR)/lock$(OBJ_EXT): lock.c lock.h portability.h process.h \
	tmutil.h fileutil.h $(MKC_OUTPUT)

$(OBJDIR)/log$(OBJ_EXT): log.c log.h tmutil.h fileutil.h $(MKC_OUTPUT)

$(OBJDIR)/musicdb$(OBJ_EXT): musicdb.c musicdb.h rafile.h song.h \
	list.h bdjstring.h lock.h datafile.h tagdef.h log.h $(MKC_OUTPUT)

$(OBJDIR)/process$(OBJ_EXT): process.c process.h $(MKC_OUTPUT)

$(OBJDIR)/rafile$(OBJ_EXT): rafile.c rafile.h lock.h tmutil.h \
	log.h $(MKC_OUTPUT)

$(OBJDIR)/sock$(OBJ_EXT): sock.c sock.h tmutil.h log.h $(MKC_OUTPUT)

$(OBJDIR)/song$(OBJ_EXT): song.c song.h list.h bdjstring.h $(MKC_OUTPUT)

$(OBJDIR)/sysvars$(OBJ_EXT): sysvars.c sysvars.h $(MKC_OUTPUT)

$(OBJDIR)/tagdef$(OBJ_EXT): tagdef.c tagdef.h bdjstring.h $(MKC_OUTPUT)

$(OBJDIR)/tmutil$(OBJ_EXT): tmutil.c tmutil.h $(MKC_OUTPUT)

# headers

datafile.h: list.h
musicdb.h: song.h
portability.h: $(MKC_OUTPUT)
sock.h: bdjconfig.h
song.h: list.h
tagdef.h: bdjstring.h list.h

# test routines

CHECKOBJS = $(OBJDIR)/check_all$(OBJ_EXT) \
	$(OBJDIR)/check_fileutil$(OBJ_EXT) \
	$(OBJDIR)/check_list$(OBJ_EXT) \
	$(OBJDIR)/check_lock$(OBJ_EXT) \
	$(OBJDIR)/check_datafile$(OBJ_EXT) \
	$(OBJDIR)/check_rafile$(OBJ_EXT) \
	$(OBJDIR)/check_sock$(OBJ_EXT) \
	$(OBJDIR)/check_tmutil$(OBJ_EXT) \
	$(SHLIBDIR)/libbdj$(SHLIB_EXT)

CHECKLIBS =
$(BINDIR)/check_all$(EXE_EXT): $(CHECKOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o $(BINDIR)/check_all$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(CHECKOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj \
		$(CHECKLIBS) \
		$(LIBS)


$(OBJDIR)/check_all$(OBJ_EXT): check_all.c check_bdj.h

$(OBJDIR)/check_fileutil$(OBJ_EXT): check_fileutil.c fileutil.h check_bdj.h

$(OBJDIR)/check_list$(OBJ_EXT): check_list.c list.h check_bdj.h

$(OBJDIR)/check_lock$(OBJ_EXT): check_lock.c lock.h check_bdj.h

$(OBJDIR)/check_datafile$(OBJ_EXT): check_datafile.c datafile.h check_bdj.h

$(OBJDIR)/check_rafile$(OBJ_EXT): check_rafile.c rafile.h check_bdj.h

$(OBJDIR)/check_sock$(OBJ_EXT): check_sock.c sock.h tmutil.h check_bdj.h

$(OBJDIR)/check_tmutil$(OBJ_EXT): check_tmutil.c tmutil.h check_bdj.h
