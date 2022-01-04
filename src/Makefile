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

# _MKCONFIG_SHELL = /bin/bash
MKC_PREFIX = bdj
MKC_CONFDIR = mkc_config
MKC_FILES = mkc_files
MKC_OUTPUT = config.h
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

mcheck: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		./bin/bdj4)

mcheckcli: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		./bin/bdj4 --clicomm)

rall:
	$(MAKE) -e libraries
	$(MAKE) -e testprograms
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

PROGRAMS = $(BINDIR)/bdj4$(EXE_EXT) $(BINDIR)/bdj4main$(EXE_EXT) \
	$(BINDIR)/bdj4player$(EXE_EXT) $(BINDIR)/clicomm$(EXE_EXT)
.PHONY: programs
programs: $(MKC_ENV_SHR) $(MKC_REQLIB) $(PROGRAMS)

TESTPROGRAMS = $(BINDIR)/check_all$(EXE_EXT)
.PHONY: testprograms
testprograms: $(MKC_ENV_SHR) $(MKC_REQLIB) $(TESTPROGRAMS)

LIBRARIES = $(SHLIBDIR)/libbdj4common$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4player$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4$(SHLIB_EXT)
.PHONY: libraries
libraries: $(MKC_ENV_SHR) $(MKC__REQLIB) $(LIBRARIES)

# mkconfig targets

$(MKC_ENV_SHR): $(MKC_ENV_SHR_CONF)
	@-$(RM) -f $(MKC_ENV_SHR)
	PKG_CONFIG_PATH=../plocal/lib/pkgconfig CC=$(CC) \
		$(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_SHR_CONF)

# remove -lvlc from the required libraries, as it is only need for the player.
$(MKC_REQLIB):  $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB) $(MKC_OUTPUT)
	sed 's/-lvlc//' $(MKC_REQLIB) > $(MKC_REQLIB).n
	mv -f $(MKC_REQLIB).n $(MKC_REQLIB)

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
	@-$(MAKE) eclean
	@-$(RM) -f $(OBJDIR)/*

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w ww *.orig $(MKC_CONFDIR)/*.orig
	@-$(RM) -f $(MKC_FILES)/mkc_compile.log $(MKC_CONFDIR)/*~
	@-$(RM) -f ../tmp/* ../conv/*~ ./locale/*~ ../locale/*~ ../*~
	@-$(RM) -f ../install/*~ ../pkg/*~ build/*~ utils/*~
	@-$(RM) -f si-in.txt si-sort.txt

# bdj executables

LAUNCHEROBJS = \
	$(OBJDIR)/bdj4$(OBJ_EXT) \
	$(OBJDIR)/bdjstring$(OBJ_EXT) \
	$(OBJDIR)/fileop$(OBJ_EXT) \
	$(OBJDIR)/pathutil$(OBJ_EXT) \
	$(OBJDIR)/sysvars$(OBJ_EXT) \
	$(OBJDIR)/strlcat$(OBJ_EXT) \
	$(OBJDIR)/strlcpy$(OBJ_EXT)
RESOURCEFILES =

$(BINDIR)/bdj4$(EXE_EXT): $(LAUNCHEROBJS)
	@. ./build/prebuild.sh $@ ; \
	$(MAKE) -e rbdj4

# do not use the required libs for the bdj4 launcher
rbdj4:
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-o $(BINDIR)/bdj4$(EXE_EXT) \
		$(LAUNCHEROBJS) \
		$(RESOURCEFILES)
	./build/postbuild.sh $(BINDIR)/bdj4$(EXE_EXT)

BDJMAINOBJS = $(OBJDIR)/bdj4main$(OBJ_EXT)

$(BINDIR)/bdj4main$(EXE_EXT): $(MKC_REQLIB) $(BDJMAINOBJS) \
		$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-o $@ \
		$(BDJMAINOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4 -l bdj4common
	./build/postbuild.sh $@

PLAYEROBJS = $(OBJDIR)/bdj4player$(OBJ_EXT) \
	$(OBJDIR)/vlci$(OBJ_EXT)

$(BINDIR)/bdj4player$(EXE_EXT): $(PLAYEROBJS) \
		$(SHLIBDIR)/libbdj4player$(SHLIB_EXT) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-o $@ \
		$(PLAYEROBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l vlc -l bdj4player -l bdj4common
	./build/postbuild.sh $@

CLICOMMOBJS = \
	$(OBJDIR)/clicomm$(OBJ_EXT)

$(BINDIR)/clicomm$(EXE_EXT): $(CLICOMMOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-o $@ \
		$(CLICOMMOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

# bdj libraries

LIBCOMMONOBJS = \
	$(OBJDIR)/bdjmsg$(OBJ_EXT) \
	$(OBJDIR)/bdjopt$(OBJ_EXT) \
	$(OBJDIR)/bdjstring$(OBJ_EXT) \
	$(OBJDIR)/bdjvars$(OBJ_EXT) \
	$(OBJDIR)/datafile$(OBJ_EXT) \
	$(OBJDIR)/datautil$(OBJ_EXT) \
	$(OBJDIR)/filedata$(OBJ_EXT) \
	$(OBJDIR)/fileutil$(OBJ_EXT) \
	$(OBJDIR)/fileop$(OBJ_EXT) \
	$(OBJDIR)/list$(OBJ_EXT) \
	$(OBJDIR)/lock$(OBJ_EXT) \
	$(OBJDIR)/log$(OBJ_EXT) \
	$(OBJDIR)/pathutil$(OBJ_EXT) \
	$(OBJDIR)/process$(OBJ_EXT) \
	$(OBJDIR)/sock$(OBJ_EXT) \
	$(OBJDIR)/sockh$(OBJ_EXT) \
	$(OBJDIR)/strlcat$(OBJ_EXT) \
	$(OBJDIR)/strlcpy$(OBJ_EXT) \
	$(OBJDIR)/sysvars$(OBJ_EXT) \
	$(OBJDIR)/tmutil$(OBJ_EXT)

$(SHLIBDIR)/libbdj4common$(SHLIB_EXT): $(LIBCOMMONOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-o $@ \
		$(LIBCOMMONOBJS)

LIBBDJOBJS = \
	$(OBJDIR)/autosel$(OBJ_EXT) \
	$(OBJDIR)/bdj4init$(OBJ_EXT) \
	$(OBJDIR)/bdjvarsdf$(OBJ_EXT) \
	$(OBJDIR)/bdjvarsdfload$(OBJ_EXT) \
	$(OBJDIR)/dance$(OBJ_EXT) \
	$(OBJDIR)/dnctypes$(OBJ_EXT) \
	$(OBJDIR)/genre$(OBJ_EXT) \
	$(OBJDIR)/level$(OBJ_EXT) \
	$(OBJDIR)/musicdb$(OBJ_EXT) \
	$(OBJDIR)/playlist$(OBJ_EXT) \
	$(OBJDIR)/queue$(OBJ_EXT) \
	$(OBJDIR)/rafile$(OBJ_EXT) \
	$(OBJDIR)/rating$(OBJ_EXT) \
	$(OBJDIR)/sequence$(OBJ_EXT) \
	$(OBJDIR)/song$(OBJ_EXT) \
	$(OBJDIR)/songlist$(OBJ_EXT) \
	$(OBJDIR)/sortopt$(OBJ_EXT) \
	$(OBJDIR)/tagdef$(OBJ_EXT) \
	$(OBJDIR)/volalsa$(OBJ_EXT) \
	$(OBJDIR)/volmac$(OBJ_EXT) \
	$(OBJDIR)/volpa$(OBJ_EXT) \
	$(OBJDIR)/volume$(OBJ_EXT) \
	$(OBJDIR)/volwin$(OBJ_EXT)

$(SHLIBDIR)/libbdj4$(SHLIB_EXT): $(LIBBDJOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-o $@ \
		$(LIBBDJOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

LIBPLAYOBJS = \
	$(OBJDIR)/volume$(OBJ_EXT) \
	$(OBJDIR)/volalsa$(OBJ_EXT) \
	$(OBJDIR)/volmac$(OBJ_EXT) \
	$(OBJDIR)/volpa$(OBJ_EXT) \
	$(OBJDIR)/volwin$(OBJ_EXT)

$(SHLIBDIR)/libbdj4player$(SHLIB_EXT): $(MKC_REQLIB) $(LIBPLAYOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-o $@ \
		$(LIBPLAYOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

# test routines

CHECKOBJS = $(OBJDIR)/check_all$(OBJ_EXT) \
	$(OBJDIR)/check_filedata$(OBJ_EXT) \
	$(OBJDIR)/check_fileop$(OBJ_EXT) \
	$(OBJDIR)/check_list$(OBJ_EXT) \
	$(OBJDIR)/check_lock$(OBJ_EXT) \
	$(OBJDIR)/check_datafile$(OBJ_EXT) \
	$(OBJDIR)/check_process$(OBJ_EXT) \
	$(OBJDIR)/check_queue$(OBJ_EXT) \
	$(OBJDIR)/check_rafile$(OBJ_EXT) \
	$(OBJDIR)/check_sock$(OBJ_EXT) \
	$(OBJDIR)/check_tmutil$(OBJ_EXT) \
	$(OBJDIR)/check_pathutil$(OBJ_EXT) \
	$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)

$(BINDIR)/check_all$(EXE_EXT): $(CHECKOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-o $@ \
		$(CHECKOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4 -l bdj4common
	./build/postbuild.sh $@

# some platform/compiler specific targets

# volmac.m require clang on windows
# and it won't affect the other platforms
$(OBJDIR)/volmac$(OBJ_EXT): volmac.m volume.h $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) CC=clang \
		$(MKC_DIR)/mkc.sh \
		-compile $(MKC_ECHO) \
		-o $@ -shared $<

$(OBJDIR)/volwin$(OBJ_EXT): volwin.cpp volume.h $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) CC=$(CXX) $(MKC_DIR)/mkc.sh \
		-compile $(MKC_ECHO) \
		-o $@ -shared $<

# DEPENDS - Do not remove this line

$(OBJDIR)/autosel$(OBJ_EXT): autosel.c config.h autosel.h datafile.h list.h
$(OBJDIR)/bdj4$(OBJ_EXT): bdj4.c config.h sysvars.h portability.h bdjstring.h fileop.h \
 pathutil.h
$(OBJDIR)/bdj4init$(OBJ_EXT): bdj4init.c config.h bdj4init.h musicdb.h song.h list.h \
 tagdef.h bdjstring.h sysvars.h portability.h bdjvars.h bdjvarsdfload.h \
 log.h fileutil.h tmutil.h fileop.h datautil.h bdjopt.h sequence.h \
 datafile.h songlist.h
$(OBJDIR)/bdj4main$(OBJ_EXT): bdj4main.c bdj4init.h sockh.h sock.h config.h bdjvars.h \
 bdjmsg.h log.h fileutil.h process.h bdjstring.h sysvars.h portability.h
$(OBJDIR)/bdj4player$(OBJ_EXT): bdj4player.c config.h bdjmsg.h bdjstring.h bdjvars.h \
 fileop.h list.h log.h fileutil.h sock.h sockh.h sysvars.h portability.h \
 vlci.h pathutil.h
$(OBJDIR)/bdjmsg$(OBJ_EXT): bdjmsg.c config.h bdjmsg.h bdjstring.h
$(OBJDIR)/bdjopt$(OBJ_EXT): bdjopt.c config.h bdjopt.h list.h datafile.h datautil.h \
 portability.h
$(OBJDIR)/bdjstring$(OBJ_EXT): bdjstring.c config.h bdjstring.h
$(OBJDIR)/bdjvars$(OBJ_EXT): bdjvars.c config.h bdjvars.h sysvars.h portability.h
$(OBJDIR)/bdjvarsdf$(OBJ_EXT): bdjvarsdf.c config.h bdjvarsdf.h datafile.h list.h
$(OBJDIR)/bdjvarsdfload$(OBJ_EXT): bdjvarsdfload.c config.h bdjvarsdfload.h bdjvarsdf.h \
 datafile.h list.h dance.h autosel.h dnctypes.h genre.h level.h rating.h \
 sortopt.h portability.h datautil.h
$(OBJDIR)/check_all$(OBJ_EXT): check_all.c config.h check_bdj.h sysvars.h portability.h \
 tagdef.h bdjstring.h list.h log.h fileutil.h
$(OBJDIR)/check_datafile$(OBJ_EXT): check_datafile.c config.h fileop.h datafile.h list.h \
 log.h fileutil.h check_bdj.h
$(OBJDIR)/check_filedata$(OBJ_EXT): check_filedata.c config.h filedata.h check_bdj.h \
 portability.h
$(OBJDIR)/check_fileop$(OBJ_EXT): check_fileop.c config.h fileop.h check_bdj.h \
 portability.h
$(OBJDIR)/check_list$(OBJ_EXT): check_list.c config.h list.h bdjstring.h check_bdj.h
$(OBJDIR)/check_lock$(OBJ_EXT): check_lock.c config.h lock.h bdjstring.h check_bdj.h \
 sysvars.h portability.h
$(OBJDIR)/check_pathutil$(OBJ_EXT): check_pathutil.c config.h pathutil.h check_bdj.h \
 portability.h
$(OBJDIR)/check_process$(OBJ_EXT): check_process.c config.h process.h check_bdj.h sysvars.h \
 portability.h
$(OBJDIR)/check_rafile$(OBJ_EXT): check_rafile.c config.h rafile.h check_bdj.h
$(OBJDIR)/check_sock$(OBJ_EXT): check_sock.c config.h sock.h check_bdj.h tmutil.h log.h \
 fileutil.h
$(OBJDIR)/check_tmutil$(OBJ_EXT): check_tmutil.c config.h tmutil.h check_bdj.h
$(OBJDIR)/clicomm$(OBJ_EXT): clicomm.c config.h sockh.h sock.h bdjmsg.h sysvars.h \
 portability.h bdjvars.h log.h fileutil.h tmutil.h
$(OBJDIR)/dance$(OBJ_EXT): dance.c config.h dance.h datafile.h list.h fileop.h bdjvarsdf.h \
 dnctypes.h
$(OBJDIR)/datafile$(OBJ_EXT): datafile.c config.h datafile.h list.h tmutil.h fileop.h \
 bdjstring.h log.h fileutil.h portability.h filedata.h
$(OBJDIR)/datautil$(OBJ_EXT): datautil.c config.h datautil.h sysvars.h portability.h
$(OBJDIR)/dnctypes$(OBJ_EXT): dnctypes.c config.h dnctypes.h datafile.h list.h fileop.h \
 bdjvarsdf.h
$(OBJDIR)/filedata$(OBJ_EXT): filedata.c config.h filedata.h
$(OBJDIR)/fileop$(OBJ_EXT): fileop.c config.h fileop.h bdjstring.h portability.h pathutil.h \
 sysvars.h
$(OBJDIR)/fileutil$(OBJ_EXT): fileutil.c config.h fileutil.h bdjstring.h portability.h
$(OBJDIR)/genre$(OBJ_EXT): genre.c config.h genre.h datafile.h list.h fileop.h
$(OBJDIR)/level$(OBJ_EXT): level.c config.h level.h datafile.h list.h fileop.h bdjvarsdf.h
$(OBJDIR)/list$(OBJ_EXT): list.c config.h list.h log.h fileutil.h
$(OBJDIR)/lock$(OBJ_EXT): lock.c config.h sysvars.h portability.h process.h tmutil.h lock.h \
 fileop.h
$(OBJDIR)/log$(OBJ_EXT): log.c config.h log.h fileutil.h tmutil.h datautil.h portability.h
$(OBJDIR)/musicdb$(OBJ_EXT): musicdb.c config.h musicdb.h song.h list.h rafile.h \
 bdjstring.h lock.h datafile.h tagdef.h log.h fileutil.h
$(OBJDIR)/pathutil$(OBJ_EXT): pathutil.c config.h bdjstring.h pathutil.h portability.h
$(OBJDIR)/playlist$(OBJ_EXT): playlist.c config.h playlist.h datafile.h list.h fileop.h \
 pathutil.h portability.h
$(OBJDIR)/process$(OBJ_EXT): process.c config.h process.h log.h fileutil.h
$(OBJDIR)/rafile$(OBJ_EXT): rafile.c config.h rafile.h lock.h tmutil.h log.h fileutil.h
$(OBJDIR)/rating$(OBJ_EXT): rating.c config.h rating.h datafile.h list.h fileop.h \
 bdjvarsdf.h
$(OBJDIR)/sequence$(OBJ_EXT): sequence.c config.h sequence.h datafile.h list.h fileop.h
$(OBJDIR)/sock$(OBJ_EXT): sock.c config.h sock.h tmutil.h log.h fileutil.h
$(OBJDIR)/sockh$(OBJ_EXT): sockh.c config.h sockh.h sock.h tmutil.h log.h fileutil.h \
 bdjmsg.h
$(OBJDIR)/song$(OBJ_EXT): song.c config.h song.h list.h bdjstring.h tagdef.h log.h \
 fileutil.h datafile.h level.h rating.h
$(OBJDIR)/songlist$(OBJ_EXT): songlist.c config.h songlist.h datafile.h list.h fileop.h
$(OBJDIR)/sortopt$(OBJ_EXT): sortopt.c config.h sortopt.h datafile.h list.h fileop.h
$(OBJDIR)/strlcat$(OBJ_EXT): strlcat.c config.h bdjstring.h
$(OBJDIR)/strlcpy$(OBJ_EXT): strlcpy.c config.h bdjstring.h
$(OBJDIR)/sysvars$(OBJ_EXT): sysvars.c config.h sysvars.h portability.h bdjstring.h \
 pathutil.h
$(OBJDIR)/tagdef$(OBJ_EXT): tagdef.c config.h tagdef.h bdjstring.h list.h
$(OBJDIR)/tmutil$(OBJ_EXT): tmutil.c config.h tmutil.h bdjstring.h
$(OBJDIR)/vlci$(OBJ_EXT): vlci.c config.h vlci.h list.h bdjstring.h log.h fileutil.h
$(OBJDIR)/volalsa$(OBJ_EXT): volalsa.c config.h
$(OBJDIR)/volpa$(OBJ_EXT): volpa.c config.h volume.h
$(OBJDIR)/volume$(OBJ_EXT): volume.c config.h volume.h
