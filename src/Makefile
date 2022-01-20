#!/usr/bin/make
#

MAKEFLAGS += --no-print-directory

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
MKC_OUTPUT_COMMON = configc.h
MKC_OUTPUT_ALSA = configalsa.h
MKC_OUTPUT_PA = configpa.h
MKC_OUTPUT_VOLWIN = configvolwin.h
MKC_OUTPUT_VLC = configvlc.h
MKC_OUTPUT_TEST = configt.h
MKC_ENV_SHR = $(MKC_PREFIX)-env.shared
MKC_ENV_TEST = $(MKC_PREFIX)-env.test

MKC_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX).mkc
MKC_CONF_COMMON = $(MKC_CONFDIR)/$(MKC_PREFIX)common.mkc
MKC_CONF_ALSA = $(MKC_CONFDIR)/$(MKC_PREFIX)alsa.mkc
MKC_CONF_PA = $(MKC_CONFDIR)/$(MKC_PREFIX)pa.mkc
MKC_CONF_VOLWIN = $(MKC_CONFDIR)/$(MKC_PREFIX)volwin.mkc
MKC_CONF_VLC = $(MKC_CONFDIR)/$(MKC_PREFIX)vlc.mkc
MKC_CONF_TEST = $(MKC_CONFDIR)/$(MKC_PREFIX)test.mkc
MKC_ENV_SHR_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env-shared.mkc
MKC_ENV_TEST_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env-test.mkc

OBJ_EXT = .o
EXE_EXT =

# mkconfig variables

MKC_DIR = ../mkconfig
MKCONFIG_TYPE = sh
MKC_REQLIB = $(MKC_PREFIX)-reqlibs.main
MKC_REQLIB_COMMON = $(MKC_PREFIX)-reqlibs.common
MKC_REQLIB_PLAYER = $(MKC_PREFIX)-reqlibs.player
MKC_REQLIB_ALSA = $(MKC_PREFIX)-reqlibs.alsa
MKC_REQLIB_PA = $(MKC_PREFIX)-reqlibs.pa
MKC_REQLIB_VOLWIN = $(MKC_PREFIX)-reqlibs.volwin
MKC_REQLIB_VLC = $(MKC_PREFIX)-reqlibs.vlc
MKC_REQLIB_TEST = $(MKC_PREFIX)-reqlibs.test
MKC_ECHO =
#MKC_ECHO = -e

CFLAGS_DEBUG = -O0 -g

#

.PHONY: all rall
all: $(MKC_ENV_SHR) $(MKC_ENV_TEST)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall

rall:
	$(MAKE) -e libraries
	$(MAKE) -e programs
	. ./$(MKC_ENV_TEST); $(MAKE) -e testprograms

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
	cc -MM -MG -I ../packages/mongoose *.c *.cpp | \
		sed -e 's,\.o,$$(OBJ_EXT),' \
		-e '/^[a-z]/ s,^,$$(OBJDIR)/,' >> Makefile.n
	mv Makefile.n Makefile

# testing
.PHONY: check
check:
	export MALLOC_PERTURB_=1; \
		 ../bin/check_all 2>&1 | tee w

# recommended to change PLAYER and VOLUME in data/bdjconfig.txt to
# be the null libraries.
.PHONY: mcheck mchecksupp mcheckcli
mcheck: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		./bin/bdj4 --main --debug 7)

mchecksupp: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/bdj4 --main --debug 7)

mcheckcli: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/bdj4 --clicomm)

mcheckcheck: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/check_all)

# main targets

PROGRAMS = \
	$(BINDIR)/bdj4$(EXE_EXT) \
	$(BINDIR)/bdj4main$(EXE_EXT) \
	$(BINDIR)/bdj4player$(EXE_EXT) \
	$(BINDIR)/clicomm$(EXE_EXT) \
	$(BINDIR)/mobilemarquee$(EXE_EXT) \
	$(BINDIR)/remotecontrol$(EXE_EXT)
.PHONY: programs
programs: $(PROGRAMS)

TESTPROGRAMS = $(BINDIR)/check_all$(EXE_EXT) $(BINDIR)/chkprocess$(EXE_EXT)
.PHONY: testprograms
testprograms: $(TESTPROGRAMS)

LIBRARIES = \
	$(SHLIBDIR)/libbdj4common$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4player$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
	$(SHLIBDIR)/libplivlc$(SHLIB_EXT) \
	$(SHLIBDIR)/libplinull$(SHLIB_EXT) \
	$(SHLIBDIR)/libvolalsa$(SHLIB_EXT) \
	$(SHLIBDIR)/libvolmac$(SHLIB_EXT) \
	$(SHLIBDIR)/libvolnull$(SHLIB_EXT) \
	$(SHLIBDIR)/libvolpa$(SHLIB_EXT) \
	$(SHLIBDIR)/libvolwin$(SHLIB_EXT)
.PHONY: libraries
libraries: $(LIBRARIES)

# mkconfig targets

$(MKC_ENV_SHR): $(MKC_ENV_SHR_CONF)
	@-$(RM) -f $(MKC_ENV_SHR)
	PKG_CONFIG_PATH=../plocal/lib/pkgconfig CC=$(CC) \
		$(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_SHR_CONF)

$(MKC_ENV_TEST): $(MKC_ENV_TEST_CONF)
	@-$(RM) -f $(MKC_ENV_TEST)
	PKG_CONFIG_PATH=../plocal/lib/pkgconfig CC=$(CC) \
		$(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_TEST_CONF)

$(MKC_REQLIB):  $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB) $(MKC_OUTPUT)

$(MKC_REQLIB_COMMON):  $(MKC_OUTPUT_COMMON)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_COMMON) $(MKC_OUTPUT_COMMON)

$(MKC_REQLIB_ALSA):  $(MKC_OUTPUT_ALSA)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_ALSA) $(MKC_OUTPUT_ALSA)

$(MKC_REQLIB_PA):  $(MKC_OUTPUT_PA)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_PA) $(MKC_OUTPUT_PA)

$(MKC_REQLIB_VOLWIN):  $(MKC_OUTPUT_VOLWIN)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_VOLWIN) $(MKC_OUTPUT_VOLWIN)

$(MKC_REQLIB_VLC):  $(MKC_OUTPUT_VLC)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_VLC) $(MKC_OUTPUT_VLC)

$(MKC_REQLIB_TEST):  $(MKC_OUTPUT_TEST)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB_TEST) $(MKC_OUTPUT_TEST)

$(MKC_OUTPUT):       $(MKC_ENV_SHR) $(MKC_CONF)
	@-$(RM) -f $(MKC_OUTPUT)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF)

$(MKC_OUTPUT_COMMON):       $(MKC_ENV_SHR) $(MKC_CONF_COMMON)
	@-$(RM) -f $(MKC_OUTPUT_COMMON)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_COMMON)

$(MKC_OUTPUT_ALSA):       $(MKC_ENV_SHR) $(MKC_CONF_ALSA)
	@-$(RM) -f $(MKC_OUTPUT_ALSA)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_ALSA)

$(MKC_OUTPUT_PA):       $(MKC_ENV_SHR) $(MKC_CONF_PA)
	@-$(RM) -f $(MKC_OUTPUT_PA)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_PA)

$(MKC_OUTPUT_VOLWIN):       $(MKC_ENV_SHR) $(MKC_CONF_VOLWIN)
	@-$(RM) -f $(MKC_OUTPUT_VOLWIN)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_VOLWIN)

$(MKC_OUTPUT_VLC):       $(MKC_ENV_SHR) $(MKC_CONF_VLC)
	@-$(RM) -f $(MKC_OUTPUT_VLC)
	. ./$(MKC_ENV_SHR);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_VLC)

$(MKC_OUTPUT_TEST):       $(MKC_ENV_TEST) $(MKC_CONF_TEST)
	@-$(RM) -f $(MKC_OUTPUT_TEST)
	. ./$(MKC_ENV_TEST);$(_MKCONFIG_SHELL) \
		$(MKC_DIR)/mkconfig.sh $(MKC_CONF_TEST)

# compilation

$(OBJDIR)/%.o: %.c
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -compile $(MKC_ECHO) \
		-o $@ -shared -DSHLIB_EXT=\\\"$(SHLIB_EXT)\\\" $<

# cleaning

.PHONY: dataclean
dataclean:
	@-test -d ../data && $(RM) -rf ../data/*

.PHONY: distclean
distclean:
	@-$(MAKE) clean
	@-$(RM) -f $(MKC_ENV_SHR) $(MKC_ENV_TEST)
	@-$(RM) -f $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(MKC_REQLIB_VLC) $(MKC_REQLIB_TEST)
	@-$(RM) -f $(MKC_REQLIB_PA) $(MKC_REQLIB_VOLWIN) $(MKC_REQLIB_ALSA)
	@-$(RM) -f $(MKC_OUTPUT) $(MKC_OUTPUT_COMMON) $(MKC_VLCOUTPUT)
	@-$(RM) -f $(MKC_OUTPUT_ALSA) $(MKC_OUTPUT_PA) $(MKC_OUTPUT_TEST)
	@-$(RM) -f $(MKC_OUTPUT_VOLWIN)
	@-$(RM) -rf $(MKC_FILES)
	@-test -d ../test.dir/data && $(RM) -rf ../test.dir/data/*
	@-$(RM) -f ../install/manifest.txt ../install/src-manifest.txt

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(MAKE) eclean
	@-$(RM) -f $(OBJDIR)/* launcher.res

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w ww *.orig $(MKC_CONFDIR)/*.orig
	@-$(RM) -f $(MKC_FILES)/mkc_compile.log $(MKC_CONFDIR)/*~
	@-$(RM) -f ../tmp/* ../conv/*~ ./locale/*~ ../locale/*~ ../*~
	@-$(RM) -f ../install/*~ ../pkg/*~ build/*~ utils/*~
	@-$(RM) -f ../templates/*~ ../vlc-log.txt vlc-log.txt
	@-$(RM) -f si-in.txt si-sort.txt
	@-$(RM) -f ../web/*~ ../http/*~

# bdj executables

LAUNCHEROBJS = \
	$(OBJDIR)/bdj4$(OBJ_EXT) \
	$(OBJDIR)/bdjstring$(OBJ_EXT) \
	$(OBJDIR)/datautil$(OBJ_EXT) \
	$(OBJDIR)/filedata$(OBJ_EXT) \
	$(OBJDIR)/fileop$(OBJ_EXT) \
	$(OBJDIR)/pathutil$(OBJ_EXT) \
	$(OBJDIR)/sysvars$(OBJ_EXT) \
	$(OBJDIR)/strlcat$(OBJ_EXT) \
	$(OBJDIR)/strlcpy$(OBJ_EXT)
RESOURCEFILES =

# the launcher requires  -lintl
$(BINDIR)/bdj4$(EXE_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(LAUNCHEROBJS)
	@. ./build/prebuild.sh $@ ; \
	$(MAKE) -e rbdj4

rbdj4:
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $(BINDIR)/bdj4$(EXE_EXT) \
		$(LAUNCHEROBJS) \
		$(RESOURCEFILES)
	./build/postbuild.sh $(BINDIR)/bdj4$(EXE_EXT)

BDJMAINOBJS = $(OBJDIR)/bdj4main$(OBJ_EXT)

$(BINDIR)/bdj4main$(EXE_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(BDJMAINOBJS) \
		$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(BDJMAINOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4 -l bdj4common
	./build/postbuild.sh $@

PLAYEROBJS = $(OBJDIR)/bdj4player$(OBJ_EXT)

$(BINDIR)/bdj4player$(EXE_EXT): \
		$(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(PLAYEROBJS) \
		$(SHLIBDIR)/libbdj4player$(SHLIB_EXT) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(PLAYEROBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4player -l bdj4common
	./build/postbuild.sh $@

CLICOMMOBJS = \
	$(OBJDIR)/clicomm$(OBJ_EXT)

$(BINDIR)/clicomm$(EXE_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(CLICOMMOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(CLICOMMOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

MMOBJS = \
	$(OBJDIR)/mobilemarquee$(OBJ_EXT) \
	$(OBJDIR)/mongoose$(OBJ_EXT) \
	$(OBJDIR)/websrv$(OBJ_EXT)

$(BINDIR)/mobilemarquee$(EXE_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) \
		$(MMOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(MMOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

RCOBJS = \
	$(OBJDIR)/remotecontrol$(OBJ_EXT) \
	$(OBJDIR)/mongoose$(OBJ_EXT) \
	$(OBJDIR)/websrv$(OBJ_EXT)

$(BINDIR)/remotecontrol$(EXE_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) \
		$(RCOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(RCOBJS) \
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
	$(OBJDIR)/filemanip$(OBJ_EXT) \
	$(OBJDIR)/fileop$(OBJ_EXT) \
	$(OBJDIR)/fileutil$(OBJ_EXT) \
	$(OBJDIR)/ilist$(OBJ_EXT) \
	$(OBJDIR)/list$(OBJ_EXT) \
	$(OBJDIR)/lock$(OBJ_EXT) \
	$(OBJDIR)/log$(OBJ_EXT) \
	$(OBJDIR)/nlist$(OBJ_EXT) \
	$(OBJDIR)/pathutil$(OBJ_EXT) \
	$(OBJDIR)/process$(OBJ_EXT) \
	$(OBJDIR)/queue$(OBJ_EXT) \
	$(OBJDIR)/slist$(OBJ_EXT) \
	$(OBJDIR)/sock$(OBJ_EXT) \
	$(OBJDIR)/sockh$(OBJ_EXT) \
	$(OBJDIR)/strlcat$(OBJ_EXT) \
	$(OBJDIR)/strlcpy$(OBJ_EXT) \
	$(OBJDIR)/sysvars$(OBJ_EXT) \
	$(OBJDIR)/tmutil$(OBJ_EXT)

$(SHLIBDIR)/libbdj4common$(SHLIB_EXT): \
		$(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(LIBCOMMONOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(LIBCOMMONOBJS)

LIBBDJOBJS = \
	$(OBJDIR)/autosel$(OBJ_EXT) \
	$(OBJDIR)/bdj4init$(OBJ_EXT) \
	$(OBJDIR)/bdjvarsdf$(OBJ_EXT) \
	$(OBJDIR)/bdjvarsdfload$(OBJ_EXT) \
	$(OBJDIR)/dance$(OBJ_EXT) \
	$(OBJDIR)/dancesel$(OBJ_EXT) \
	$(OBJDIR)/dnctypes$(OBJ_EXT) \
	$(OBJDIR)/dylib$(OBJ_EXT) \
	$(OBJDIR)/genre$(OBJ_EXT) \
	$(OBJDIR)/level$(OBJ_EXT) \
	$(OBJDIR)/musicdb$(OBJ_EXT) \
	$(OBJDIR)/musicq$(OBJ_EXT) \
	$(OBJDIR)/playlist$(OBJ_EXT) \
	$(OBJDIR)/portability$(OBJ_EXT) \
	$(OBJDIR)/rafile$(OBJ_EXT) \
	$(OBJDIR)/rating$(OBJ_EXT) \
	$(OBJDIR)/sequence$(OBJ_EXT) \
	$(OBJDIR)/song$(OBJ_EXT) \
	$(OBJDIR)/songlist$(OBJ_EXT) \
	$(OBJDIR)/songsel$(OBJ_EXT) \
	$(OBJDIR)/sortopt$(OBJ_EXT) \
	$(OBJDIR)/status$(OBJ_EXT) \
	$(OBJDIR)/tagdef$(OBJ_EXT) \
	$(OBJDIR)/webclient$(OBJ_EXT)

$(SHLIBDIR)/libbdj4$(SHLIB_EXT): $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(LIBBDJOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(LIBBDJOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

LIBPLAYOBJS = \
	$(OBJDIR)/pli$(OBJ_EXT) \
	$(OBJDIR)/volume$(OBJ_EXT)

$(SHLIBDIR)/libbdj4player$(SHLIB_EXT): \
		$(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(LIBPLAYOBJS) \
		$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(LIBPLAYOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4 -l bdj4common
	./build/postbuild.sh $@

LIBPLIVLCOBJS = \
	$(OBJDIR)/plivlc$(OBJ_EXT) \
	$(OBJDIR)/vlci$(OBJ_EXT)

$(SHLIBDIR)/libplivlc$(SHLIB_EXT): \
		$(MKC_REQLIB) $(MKC_REQLIB_VLC) $(MKC_REQLIB_COMMON) $(LIBPLIVLCOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_VLC) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(LIBPLIVLCOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

LIBPLINULLOBJS = \
	$(OBJDIR)/plinull$(OBJ_EXT) \

$(SHLIBDIR)/libplinull$(SHLIB_EXT): \
		$(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(LIBPLINULLOBJS) \
		$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(LIBPLINULLOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4common
	./build/postbuild.sh $@

LIBVOLALSAOBJS = \
	$(OBJDIR)/volalsa$(OBJ_EXT)

$(SHLIBDIR)/libvolalsa$(SHLIB_EXT): \
		$(MKC_REQLIB_ALSA) $(LIBVOLALSAOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB_ALSA) \
		-o $@ \
		$(LIBVOLALSAOBJS)
	./build/postbuild.sh $@

LIBVOLMACOBJS = \
	$(OBJDIR)/volmac$(OBJ_EXT)

$(SHLIBDIR)/libvolmac$(SHLIB_EXT): \
		$(LIBVOLMACOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-o $@ \
		$(LIBVOLMACOBJS)
	./build/postbuild.sh $@

LIBVOLNULLOBJS = \
	$(OBJDIR)/volnull$(OBJ_EXT)

$(SHLIBDIR)/libvolnull$(SHLIB_EXT): \
		$(LIBVOLNULLOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-o $@ \
		$(LIBVOLNULLOBJS)
	./build/postbuild.sh $@

LIBVOLPAOBJS = \
	$(OBJDIR)/volpa$(OBJ_EXT)

$(SHLIBDIR)/libvolpa$(SHLIB_EXT): \
		$(MKC_REQLIB_PA) $(LIBVOLPAOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB_PA) \
		-o $@ \
		$(LIBVOLPAOBJS)
	./build/postbuild.sh $@

LIBVOLWINOBJS = \
	$(OBJDIR)/volwin$(OBJ_EXT)

$(SHLIBDIR)/libvolwin$(SHLIB_EXT): \
		$(MKC_REQLIB_VOLWIN) $(LIBVOLWINOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -shared $(MKC_ECHO) -lib \
		-r $(MKC_REQLIB_VOLWIN) \
		-o $@ \
		$(LIBVOLWINOBJS)
	./build/postbuild.sh $@

# test routines

CHECKOBJS = $(OBJDIR)/check_all$(OBJ_EXT) \
	$(OBJDIR)/check_datafile$(OBJ_EXT) \
	$(OBJDIR)/check_filedata$(OBJ_EXT) \
	$(OBJDIR)/check_filemanip$(OBJ_EXT) \
	$(OBJDIR)/check_fileop$(OBJ_EXT) \
	$(OBJDIR)/check_ilist$(OBJ_EXT) \
	$(OBJDIR)/check_lock$(OBJ_EXT) \
	$(OBJDIR)/check_nlist$(OBJ_EXT) \
	$(OBJDIR)/check_process$(OBJ_EXT) \
	$(OBJDIR)/check_queue$(OBJ_EXT) \
	$(OBJDIR)/check_rafile$(OBJ_EXT) \
	$(OBJDIR)/check_slist$(OBJ_EXT) \
	$(OBJDIR)/check_sock$(OBJ_EXT) \
	$(OBJDIR)/check_tmutil$(OBJ_EXT) \
	$(OBJDIR)/check_pathutil$(OBJ_EXT) \
	$(SHLIBDIR)/libbdj4$(SHLIB_EXT) \
	$(SHLIBDIR)/libbdj4common$(SHLIB_EXT)

$(BINDIR)/check_all$(EXE_EXT): \
		$(MKC_REQLIB_TEST) $(MKC_REQLIB) $(MKC_REQLIB_COMMON) $(CHECKOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-r $(MKC_REQLIB_TEST) \
		-r $(MKC_REQLIB) \
		-r $(MKC_REQLIB_COMMON) \
		-o $@ \
		$(CHECKOBJS) \
		-L $(SHLIBDIR) -rpath $(BINDIR) \
		-l bdj4 -l bdj4common
	./build/postbuild.sh $@

$(BINDIR)/chkprocess$(EXE_EXT): chkprocess.c
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec $(MKC_ECHO) \
		-o $@ \
		chkprocess.c

# some platform/compiler specific targets

$(OBJDIR)/mongoose$(OBJ_EXT): ../packages/mongoose/mongoose.c $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-compile $(MKC_ECHO) \
		-o $@ -shared \
		-Wno-declaration-after-statement \
		-DMG_ENABLE_IPV6=1 \
		-DMG_ENABLE_LOG=1 \
		-DMG_ENABLE_SSI=0 \
		-DMG_ENABLE_DIRLIST=0 \
		-DMG_ENABLE_OPENSSL=1 \
		-DMG_ENABLE_MBEDTLS=0 \
		$<

# volmac.m require clang on windows
# and it won't affect the other platforms
$(OBJDIR)/volmac$(OBJ_EXT): volmac.m volume.h $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) CC=clang \
		$(MKC_DIR)/mkc.sh \
		-compile $(MKC_ECHO) \
		-o $@ -shared -Wno-unknown-warning-option $<

$(OBJDIR)/volwin$(OBJ_EXT): volwin.cpp volume.h $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) CC=$(CXX) $(MKC_DIR)/mkc.sh \
		-compile $(MKC_ECHO) \
		-o $@ -shared $<

# DEPENDS - Do not remove this line

$(OBJDIR)/autosel$(OBJ_EXT): autosel.c config.h autosel.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h log.h fileutil.h
$(OBJDIR)/bdj4$(OBJ_EXT): bdj4.c config.h bdjstring.h datautil.h fileop.h pathutil.h \
 portability.h sysvars.h
$(OBJDIR)/bdj4init$(OBJ_EXT): bdj4init.c config.h bdj4init.h musicdb.h song.h nlist.h \
 list.h tagdef.h bdjstring.h sysvars.h portability.h bdjvars.h \
 bdjvarsdfload.h log.h fileutil.h tmutil.h fileop.h datautil.h bdjopt.h \
 lock.h
$(OBJDIR)/bdj4main$(OBJ_EXT): bdj4main.c config.h bdj4.h sock.h bdj4init.h bdjmsg.h \
 bdjopt.h bdjstring.h bdjvars.h bdjvarsdf.h datafile.h list.h nlist.h \
 slist.h ilist.h dance.h datautil.h filedata.h lock.h log.h fileutil.h \
 musicq.h queue.h song.h tagdef.h playlist.h musicdb.h songlist.h \
 sequence.h songsel.h portability.h process.h sockh.h sysvars.h tmutil.h \
 webclient.h
$(OBJDIR)/bdj4player$(OBJ_EXT): bdj4player.c config.h bdj4.h sock.h bdjmsg.h bdjopt.h \
 bdjstring.h bdjvars.h datautil.h fileop.h filemanip.h slist.h list.h \
 lock.h log.h fileutil.h pathutil.h pli.h dylib.h portability.h tmutil.h \
 process.h queue.h sockh.h sysvars.h volume.h
$(OBJDIR)/bdjmsg$(OBJ_EXT): bdjmsg.c config.h bdjmsg.h bdjstring.h
$(OBJDIR)/bdjopt$(OBJ_EXT): bdjopt.c config.h bdjopt.h datafile.h list.h nlist.h slist.h \
 ilist.h datautil.h filemanip.h fileop.h portability.h sysvars.h
$(OBJDIR)/bdjstring$(OBJ_EXT): bdjstring.c config.h bdjstring.h
$(OBJDIR)/bdjvars$(OBJ_EXT): bdjvars.c config.h bdjvars.h sysvars.h portability.h \
 bdjstring.h bdjopt.h
$(OBJDIR)/bdjvarsdf$(OBJ_EXT): bdjvarsdf.c config.h bdjvarsdf.h datafile.h list.h nlist.h \
 slist.h ilist.h
$(OBJDIR)/bdjvarsdfload$(OBJ_EXT): bdjvarsdfload.c config.h autosel.h datafile.h list.h \
 nlist.h slist.h ilist.h bdjvarsdf.h bdjvarsdfload.h dance.h datautil.h \
 dnctypes.h genre.h level.h portability.h rating.h sortopt.h status.h
$(OBJDIR)/check_all$(OBJ_EXT): check_all.c config.h configt.h check_bdj.h sysvars.h \
 portability.h tagdef.h bdjstring.h log.h fileutil.h
$(OBJDIR)/check_datafile$(OBJ_EXT): check_datafile.c config.h configt.h fileop.h datafile.h \
 list.h nlist.h slist.h ilist.h log.h fileutil.h check_bdj.h
$(OBJDIR)/check_filedata$(OBJ_EXT): check_filedata.c config.h configt.h filedata.h \
 check_bdj.h portability.h
$(OBJDIR)/check_filemanip$(OBJ_EXT): check_filemanip.c config.h configt.h filemanip.h \
 slist.h list.h fileop.h check_bdj.h portability.h
$(OBJDIR)/check_fileop$(OBJ_EXT): check_fileop.c config.h configt.h fileop.h check_bdj.h \
 portability.h
$(OBJDIR)/check_ilist$(OBJ_EXT): check_ilist.c config.h configt.h ilist.h list.h slist.h \
 bdjstring.h check_bdj.h log.h fileutil.h
$(OBJDIR)/check_list$(OBJ_EXT): check_list.c config.h configt.h nlist.h list.h slist.h \
 ilist.h bdjstring.h check_bdj.h
$(OBJDIR)/check_lock$(OBJ_EXT): check_lock.c config.h configt.h lock.h datautil.h \
 bdjstring.h check_bdj.h sysvars.h portability.h
$(OBJDIR)/check_nlist$(OBJ_EXT): check_nlist.c config.h configt.h nlist.h list.h \
 bdjstring.h check_bdj.h log.h fileutil.h
$(OBJDIR)/check_pathutil$(OBJ_EXT): check_pathutil.c config.h configt.h pathutil.h \
 check_bdj.h portability.h
$(OBJDIR)/check_process$(OBJ_EXT): check_process.c config.h configt.h check_bdj.h \
 datautil.h portability.h process.h sysvars.h
$(OBJDIR)/check_queue$(OBJ_EXT): check_queue.c config.h configt.h queue.h check_bdj.h \
 portability.h log.h fileutil.h
$(OBJDIR)/check_rafile$(OBJ_EXT): check_rafile.c config.h configt.h rafile.h check_bdj.h
$(OBJDIR)/check_slist$(OBJ_EXT): check_slist.c config.h configt.h slist.h list.h \
 bdjstring.h check_bdj.h log.h fileutil.h
$(OBJDIR)/check_sock$(OBJ_EXT): check_sock.c config.h configt.h configc.h sock.h \
 check_bdj.h tmutil.h log.h fileutil.h
$(OBJDIR)/check_tmutil$(OBJ_EXT): check_tmutil.c config.h configt.h tmutil.h check_bdj.h
$(OBJDIR)/chkprocess$(OBJ_EXT): chkprocess.c
$(OBJDIR)/clicomm$(OBJ_EXT): clicomm.c config.h sockh.h sock.h bdjmsg.h sysvars.h \
 portability.h bdjvars.h log.h fileutil.h tmutil.h
$(OBJDIR)/dance$(OBJ_EXT): dance.c config.h bdjstring.h bdjvarsdf.h datafile.h list.h \
 nlist.h slist.h ilist.h dance.h dnctypes.h fileop.h log.h fileutil.h
$(OBJDIR)/dancesel$(OBJ_EXT): dancesel.c config.h bdj4.h sock.h autosel.h datafile.h list.h \
 nlist.h slist.h ilist.h bdjvarsdf.h dance.h dancesel.h playlist.h \
 musicdb.h song.h songlist.h sequence.h songsel.h queue.h
$(OBJDIR)/datafile$(OBJ_EXT): datafile.c config.h bdjstring.h datafile.h list.h nlist.h \
 slist.h ilist.h filedata.h filemanip.h fileop.h log.h fileutil.h \
 portability.h tmutil.h
$(OBJDIR)/datautil$(OBJ_EXT): datautil.c config.h datautil.h sysvars.h portability.h
$(OBJDIR)/dnctypes$(OBJ_EXT): dnctypes.c config.h dnctypes.h datafile.h list.h nlist.h \
 slist.h ilist.h fileop.h bdjvarsdf.h log.h fileutil.h
$(OBJDIR)/dylib$(OBJ_EXT): dylib.c config.h dylib.h pathutil.h portability.h
$(OBJDIR)/filedata$(OBJ_EXT): filedata.c config.h filedata.h
$(OBJDIR)/filemanip$(OBJ_EXT): filemanip.c config.h bdjstring.h filemanip.h slist.h list.h \
 fileop.h pathutil.h portability.h sysvars.h
$(OBJDIR)/fileop$(OBJ_EXT): fileop.c config.h fileop.h
$(OBJDIR)/fileutil$(OBJ_EXT): fileutil.c config.h fileutil.h bdjstring.h portability.h
$(OBJDIR)/genre$(OBJ_EXT): genre.c config.h bdjvarsdf.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h genre.h log.h fileutil.h
$(OBJDIR)/ilist$(OBJ_EXT): ilist.c config.h bdjstring.h ilist.h list.h slist.h log.h \
 fileutil.h nlist.h
$(OBJDIR)/level$(OBJ_EXT): level.c config.h bdjvarsdf.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h level.h log.h fileutil.h
$(OBJDIR)/list$(OBJ_EXT): list.c config.h list.h log.h fileutil.h bdjstring.h tmutil.h
$(OBJDIR)/lock$(OBJ_EXT): lock.c config.h sysvars.h portability.h process.h tmutil.h lock.h \
 datautil.h fileop.h
$(OBJDIR)/log$(OBJ_EXT): log.c config.h log.h fileutil.h tmutil.h datautil.h bdjstring.h \
 portability.h
$(OBJDIR)/mobilemarquee$(OBJ_EXT): mobilemarquee.c config.h ../packages/mongoose/mongoose.h \
 bdj4.h sock.h bdjmsg.h bdjopt.h bdjvars.h lock.h datautil.h log.h \
 fileutil.h process.h sockh.h sysvars.h portability.h tmutil.h websrv.h
$(OBJDIR)/musicdb$(OBJ_EXT): musicdb.c config.h bdjstring.h bdjvarsdf.h datafile.h list.h \
 nlist.h slist.h ilist.h dance.h lock.h datautil.h log.h fileutil.h \
 musicdb.h song.h rafile.h tagdef.h
$(OBJDIR)/musicq$(OBJ_EXT): musicq.c config.h bdjvarsdf.h datafile.h list.h nlist.h slist.h \
 ilist.h dance.h musicq.h queue.h song.h tagdef.h bdjstring.h
$(OBJDIR)/nlist$(OBJ_EXT): nlist.c config.h list.h log.h fileutil.h nlist.h bdjstring.h
$(OBJDIR)/pathutil$(OBJ_EXT): pathutil.c config.h bdjstring.h pathutil.h portability.h
$(OBJDIR)/playlist$(OBJ_EXT): playlist.c config.h bdjvarsdf.h datafile.h list.h nlist.h \
 slist.h ilist.h dance.h datautil.h filemanip.h fileop.h level.h log.h \
 fileutil.h musicdb.h song.h pathutil.h playlist.h songlist.h sequence.h \
 songsel.h queue.h portability.h rating.h status.h tagdef.h bdjstring.h
$(OBJDIR)/pli$(OBJ_EXT): pli.c config.h bdjopt.h datautil.h dylib.h pli.h portability.h \
 tmutil.h sysvars.h
$(OBJDIR)/plinull$(OBJ_EXT): plinull.c config.h pli.h dylib.h portability.h tmutil.h
$(OBJDIR)/plivlc$(OBJ_EXT): plivlc.c config.h configvlc.h pli.h dylib.h portability.h \
 tmutil.h vlci.h slist.h list.h
$(OBJDIR)/portability$(OBJ_EXT): portability.c config.h portability.h
$(OBJDIR)/process$(OBJ_EXT): process.c config.h process.h log.h fileutil.h
$(OBJDIR)/queue$(OBJ_EXT): queue.c config.h queue.h
$(OBJDIR)/rafile$(OBJ_EXT): rafile.c config.h rafile.h lock.h datautil.h tmutil.h log.h \
 fileutil.h
$(OBJDIR)/rating$(OBJ_EXT): rating.c config.h bdjvarsdf.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h log.h fileutil.h rating.h
$(OBJDIR)/remotecontrol$(OBJ_EXT): remotecontrol.c config.h ../packages/mongoose/mongoose.h \
 bdj4.h sock.h bdjmsg.h bdjopt.h bdjvars.h lock.h datautil.h log.h \
 fileutil.h process.h sockh.h sysvars.h portability.h tmutil.h websrv.h
$(OBJDIR)/sequence$(OBJ_EXT): sequence.c config.h dance.h datafile.h list.h nlist.h slist.h \
 ilist.h datautil.h fileop.h log.h fileutil.h portability.h sequence.h
$(OBJDIR)/slist$(OBJ_EXT): slist.c config.h list.h log.h fileutil.h slist.h bdjstring.h
$(OBJDIR)/sock$(OBJ_EXT): sock.c config.h sock.h tmutil.h log.h fileutil.h
$(OBJDIR)/sockh$(OBJ_EXT): sockh.c config.h sockh.h sock.h bdjmsg.h tmutil.h log.h \
 fileutil.h
$(OBJDIR)/song$(OBJ_EXT): song.c config.h bdjopt.h bdjstring.h dance.h datafile.h list.h \
 nlist.h slist.h ilist.h fileop.h genre.h level.h log.h fileutil.h \
 portability.h rating.h song.h status.h tagdef.h
$(OBJDIR)/songlist$(OBJ_EXT): songlist.c config.h songlist.h datafile.h list.h nlist.h \
 slist.h ilist.h fileop.h log.h fileutil.h
$(OBJDIR)/songsel$(OBJ_EXT): songsel.c config.h autosel.h datafile.h list.h nlist.h slist.h \
 ilist.h bdjvarsdf.h level.h log.h fileutil.h musicdb.h song.h \
 portability.h queue.h rating.h songsel.h tagdef.h bdjstring.h
$(OBJDIR)/sortopt$(OBJ_EXT): sortopt.c config.h sortopt.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h log.h fileutil.h
$(OBJDIR)/status$(OBJ_EXT): status.c config.h bdjvarsdf.h datafile.h list.h nlist.h slist.h \
 ilist.h fileop.h log.h fileutil.h status.h
$(OBJDIR)/strlcat$(OBJ_EXT): strlcat.c config.h bdjstring.h
$(OBJDIR)/strlcpy$(OBJ_EXT): strlcpy.c config.h bdjstring.h
$(OBJDIR)/sysvars$(OBJ_EXT): sysvars.c config.h sysvars.h portability.h bdjstring.h \
 filedata.h fileop.h pathutil.h
$(OBJDIR)/tagdef$(OBJ_EXT): tagdef.c config.h tagdef.h bdjstring.h
$(OBJDIR)/tmutil$(OBJ_EXT): tmutil.c config.h tmutil.h bdjstring.h
$(OBJDIR)/vlci$(OBJ_EXT): vlci.c config.h configvlc.h vlci.h slist.h list.h bdjstring.h
$(OBJDIR)/volalsa$(OBJ_EXT): volalsa.c config.h configalsa.h volume.h dylib.h portability.h
$(OBJDIR)/volnull$(OBJ_EXT): volnull.c config.h volume.h dylib.h portability.h
$(OBJDIR)/volpa$(OBJ_EXT): volpa.c config.h configpa.h volume.h dylib.h portability.h \
 bdjstring.h tmutil.h
$(OBJDIR)/volume$(OBJ_EXT): volume.c config.h bdjopt.h datautil.h dylib.h portability.h \
 sysvars.h volume.h
$(OBJDIR)/webclient$(OBJ_EXT): webclient.c config.h log.h fileutil.h sysvars.h \
 portability.h webclient.h
$(OBJDIR)/websrv$(OBJ_EXT): websrv.c config.h ../packages/mongoose/mongoose.h log.h \
 fileutil.h websrv.h
$(OBJDIR)/volwin$(OBJ_EXT): volwin.cpp config.h configvolwin.h
