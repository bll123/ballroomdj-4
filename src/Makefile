#!/usr/bin/make
#

# paths

BINDIR = ../bin/

# mkconfig settings

MKC_PREFIX = bdj
MKC_CONFDIR = mkc_config
MKC_FILES = mkc_files
MKC_OUTPUT = bdjconfig.h
MKC_ENV = $(MKC_PREFIX).env
MKC_ENV_SHR = $(MKC_PREFIX)-shared.env
MKC_ENV_TEST = $(MKC_PREFIX)-test.env

MKC_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX).mkc
MKC_ENV_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env.mkc
MKC_ENV_SHR_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env-shared.mkc
MKC_ENV_TEST_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env-test.mkc

OBJ_EXT = .o
EXE_EXT =

# mkconfig variables

MKC_DIR = ../mkconfig
MKCONFIG_TYPE = sh
MKC_REQLIB = $(MKC_PREFIX).reqlibs
#MKC_ECHO =
MKC_ECHO = -e

CFLAGS_DEBUG = -O0 -g

.PHONY: all
all: $(MKC_ENV) $(MKC_ENV_SHR) $(MKC_ENV_TEST)
	. ./$(MKC_ENV_SHR); $(MAKE) -e libraries
	. ./$(MKC_ENV_SHR);. ./$(MKC_ENV_TEST); $(MAKE) -e testprograms
	. ./$(MKC_ENV_SHR); $(MAKE) -e programs

# main targets

PROGRAMS = $(BINDIR)bdj4$(EXE_EXT)
.PHONY: programs
programs: $(MKC_REQLIB) $(PROGRAMS)

TESTPROGRAMS = $(BINDIR)check_all$(EXE_EXT)
.PHONY: testprograms
testprograms: $(MKC_REQLIB) $(TESTPROGRAMS)

LIBRARIES = $(BINDIR)libbdj$(SHLIB_EXT)
.PHONY: libraries
libraries: $(MKC__REQLIB) $(LIBRARIES)

# mkconfig targets

$(MKC_ENV):     $(MKC_ENV_CONF)
	@-$(RM) -f $(MKC_ENV)
	CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh $(MKC_ENV_CONF)

$(MKC_ENV_SHR): $(MKC_ENV_SHR_CONF)
	@-$(RM) -f $(MKC_ENV_SHR)
	CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_SHR_CONF)

$(MKC_ENV_TEST): $(MKC_ENV_TEST_CONF)
	@-$(RM) -f $(MKC_ENV_TEST)
	CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh \
		$(MKC_ENV_TEST_CONF)

$(MKC_REQLIB):  $(MKC_OUTPUT)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -reqlib \
		-o $(MKC_REQLIB) $(MKC_OUTPUT)

$(MKC_OUTPUT):       $(MKC_ENV) $(MKC_CONF)
	@-$(RM) -f $(MKC_OUTPUT)
	. ./$(MKC_ENV);$(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh $(MKC_CONF)

# compilation

.c$(OBJ_EXT):
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -compile $(MKC_ECHO) \
		-shared $<

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) clean
	@-$(RM) -f $(MKC_ENV) $(MKC_ENV_SHR) $(MKC_ENV_TEST) $(MKC_REQLIB)
	@-$(RM) -f $(MKC_OUTPUT)
	@-$(RM) -rf $(MKC_FILES)

.PHONY: clean
clean:
	-$(MAKE) tclean
	-$(RM) -f *.o *.so
	-. ./$(MKC_ENV_SHR); \
		$(RM) -f $(PROGRAMS) $(TESTPROGRAMS) $(LIBRARIES)

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w $(MKC_FILES)/mkc_compile.log $(MKC_CONFDIR)/*~
	@-$(RM) -f test_*.dat

# bdj executables

BDJMAINOBJS = bdj4$(OBJ_EXT) \
	$(BINDIR)libbdj$(SHLIB_EXT)
$(BINDIR)bdj4$(EXE_EXT): $(BDJMAINOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o $(BINDIR)bdj4$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJMAINOBJS) \
		-L $(BINDIR) -rpath $(BINDIR) \
		-l bdj \
		$(LIBS)


# bdj libraries

BDJOBJS = bdjstring$(OBJ_EXT) list$(OBJ_EXT) musicdb$(OBJ_EXT) \
	rafile$(OBJ_EXT) song$(OBJ_EXT)
$(BINDIR)libbdj$(SHLIB_EXT): $(MKC_REQLIB) $(BDJOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -shared $(MKC_ECHO) -lib \
		-o $(BINDIR)libbdj$(SHLIB_EXT) \
		-r $(MKC_REQLIB) \
		$(BDJOBJS) \
		$(LIBS)

# bdj source

bdj4$(OBJ_EXT): bdj4.c musicdb.h $(MKC_OUTPUT)

bdjstring$(OBJ_EXT): bdjstring.c bdjstring.h $(MKC_OUTPUT)

list$(OBJ_EXT): list.c list.h $(MKC_OUTPUT)

musicdb$(OBJ_EXT): musicdb.c musicdb.h rafile.h song.h $(MKC_OUTPUT)

rafile$(OBJ_EXT): rafile.c rafile.h $(MKC_OUTPUT)

song$(OBJ_EXT): song.c song.h $(MKC_OUTPUT)

# test routines

CHECKOBJS = check_all$(OBJ_EXT) check_list$(OBJ_EXT) \
	check_rafile$(OBJ_EXT) \
	$(BINDIR)libbdj$(SHLIB_EXT)
CHECKLIBS =
$(BINDIR)check_all$(EXE_EXT): $(CHECKOBJS)
	@$(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -link -exec $(MKC_ECHO) \
		-o $(BINDIR)check_all$(EXE_EXT) \
		-r $(MKC_REQLIB) \
		$(CHECKOBJS) \
		-L $(BINDIR) -rpath $(BINDIR) \
		-l bdj \
		$(CHECKLIBS) \
		$(LIBS)


check_list$(OBJ_EXT): check_list.c list.h

check_rafile$(OBJ_EXT): check_rafile.c rafile.h

check_all$(OBJ_EXT): check_all.c
