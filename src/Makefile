#!/usr/bin/make
#

# paths

prefix ?= ..
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

.PHONY: depend
depend:
	sed -n -e '1,/^# DEPENDS/ p' Makefile > Makefile.n
	echo "" >> Makefile.n
	cc -MM -MG *.c | \
		sed -e 's,\.o,$$(OBJ_EXT),' \
		-e '/^[a-z]/ s,^,$$(OBJDIR)/,' >> Makefile.n
	mv Makefile.n Makefile

# main targets

PROGRAMS = $(BINDIR)/bdj4$(EXE_EXT)
.PHONY: programs
programs: $(MKC_ENV_SHR) $(MKC_REQLIB) $(DEPENDS) $(PROGRAMS)

TESTPROGRAMS = $(BINDIR)/check_all$(EXE_EXT)
.PHONY: testprograms
testprograms: $(MKC_ENV_SHR) $(MKC_REQLIB) $(DEPENDS) $(TESTPROGRAMS)

LIBRARIES = $(SHLIBDIR)/libbdj$(SHLIB_EXT)
.PHONY: libraries
libraries: $(MKC_ENV_SHR) $(MKC__REQLIB) $(DEPENDS) $(LIBRARIES)

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
	@-$(RM) -f $(MKC_ENV_SHR) $(MKC_REQLIB) $(DEPENDS)
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
		-o $@ \
		-r $(MKC_REQLIB) \
		$(BDJMAINOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj \
		$(LIBS)
	patchelf --set-rpath '$$ORIGIN/../bin' $@


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
		-o $@ \
		-r $(MKC_REQLIB) \
		$(BDJOBJS) \
		$(LIBS)

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
		-o $@ \
		-r $(MKC_REQLIB) \
		$(CHECKOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj \
		$(CHECKLIBS) \
		$(LIBS)
	patchelf --set-rpath '$$ORIGIN/../bin' $@

# DEPENDS - Do not remove this line

$(OBJDIR)/bdj4$(OBJ_EXT): bdj4.c bdj4main.h
$(OBJDIR)/bdj4main$(OBJ_EXT): bdj4main.c bdjconfig.h bdj4main.h musicdb.h song.h list.h \
 tagdef.h bdjstring.h sysvars.h log.h
$(OBJDIR)/bdjstring$(OBJ_EXT): bdjstring.c bdjconfig.h bdjstring.h
$(OBJDIR)/check_all$(OBJ_EXT): check_all.c bdjconfig.h check_bdj.h
$(OBJDIR)/check_datafile$(OBJ_EXT): check_datafile.c bdjconfig.h datafile.h list.h \
 check_bdj.h
$(OBJDIR)/check_fileutil$(OBJ_EXT): check_fileutil.c bdjconfig.h fileutil.h check_bdj.h
$(OBJDIR)/check_list$(OBJ_EXT): check_list.c bdjconfig.h list.h bdjstring.h check_bdj.h
$(OBJDIR)/check_lock$(OBJ_EXT): check_lock.c bdjconfig.h lock.h bdjstring.h check_bdj.h
$(OBJDIR)/check_rafile$(OBJ_EXT): check_rafile.c bdjconfig.h rafile.h check_bdj.h
$(OBJDIR)/check_sock$(OBJ_EXT): check_sock.c bdjconfig.h sock.h check_bdj.h tmutil.h
$(OBJDIR)/check_tmutil$(OBJ_EXT): check_tmutil.c bdjconfig.h tmutil.h check_bdj.h
$(OBJDIR)/datafile$(OBJ_EXT): datafile.c bdjconfig.h datafile.h list.h tmutil.h fileutil.h
$(OBJDIR)/fileutil$(OBJ_EXT): fileutil.c bdjconfig.h fileutil.h sysvars.h
$(OBJDIR)/list$(OBJ_EXT): list.c bdjconfig.h list.h
$(OBJDIR)/lock$(OBJ_EXT): lock.c bdjconfig.h portability.h process.h tmutil.h lock.h \
 fileutil.h
$(OBJDIR)/log$(OBJ_EXT): log.c bdjconfig.h log.h tmutil.h fileutil.h
$(OBJDIR)/musicdb$(OBJ_EXT): musicdb.c bdjconfig.h musicdb.h song.h list.h rafile.h \
 bdjstring.h lock.h datafile.h tagdef.h log.h
$(OBJDIR)/process$(OBJ_EXT): process.c bdjconfig.h process.h
$(OBJDIR)/rafile$(OBJ_EXT): rafile.c bdjconfig.h rafile.h lock.h tmutil.h log.h
$(OBJDIR)/sock$(OBJ_EXT): sock.c bdjconfig.h sock.h tmutil.h log.h
$(OBJDIR)/song$(OBJ_EXT): song.c bdjconfig.h song.h list.h bdjstring.h tagdef.h log.h
$(OBJDIR)/sysvars$(OBJ_EXT): sysvars.c bdjconfig.h sysvars.h
$(OBJDIR)/tagdef$(OBJ_EXT): tagdef.c bdjconfig.h tagdef.h bdjstring.h list.h
$(OBJDIR)/tmutil$(OBJ_EXT): tmutil.c bdjconfig.h tmutil.h
