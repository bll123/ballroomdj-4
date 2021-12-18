# included by

MKC_FILES = mkc_files
MKC_FILET = $(MKC_FILES)/$(MKC_PREFIX).t
MKC_REQLIB = $(MKC_FILES)/$(MKC_PREFIX).reqlibs
MKC_DEPFILE = $(MKC_FILES)/$(MKC_PREFIX).depends
MKC_ENVFILE = $(MKC_PREFIX).env

MKC_CONFDIR = .
MKC_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX).mkc
MKC_ENV_CONF = $(MKC_CONFDIR)/$(MKC_PREFIX)-env.mkc

# executable
$(MKC_PREFIX)$(EXE_EXT):	$(MKC_DEPFILE) $(MKC_REQLIB) $(objects)
	@CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-link -exec -r $(MKC_REQLIB) \
                -o $(MKC_PREFIX)$(EXE_EXT) $(objects)

# required libraries
$(MKC_REQLIB):  $(MKC_FILET) $(MKC_OUTPUT)
	@CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh \
		-reqlib -o $(MKC_REQLIB) $(MKC_OUTPUT)

# dependencies
$(MKC_DEPFILE):	$(MKC_FILET) $(MKC_OUTPUT) $(sources)
	$(CC) -E -MM $(sources) > $(MKC_DEPFILE)
	sed -e '/# DO NOT DELETE/,$$ d' Makefile > Makefile.bak
	(cat Makefile.bak;echo "# DO NOT DELETE";cat $(MKC_DEPFILE)) > Makefile
	@rm -f Makefile.bak

# output configuration file
$(MKC_OUTPUT):	$(MKC_FILET) $(MKC_ENVFILE) $(MKC_CONF)
	@CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh $(MKC_CONF)

# object files
.c$(OBJ_EXT):
	@CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkc.sh -compile $<

# environment variables
$(MKC_ENVFILE):		$(MKC_FILET) $(MKC_ENV_CONF)
	@CC=$(CC) $(_MKCONFIG_SHELL) $(MKC_DIR)/mkconfig.sh $(MKC_ENV_CONF)

$(MKC_FILET):
	@test -d $(MKC_FILES) || mkdir $(MKC_FILES)
	@touch $(MKC_FILET)

.PHONY: mkc-clean
mkc-clean:
	-rm -f $(objects) \
		$(MKC_PREFIX)$(EXE_EXT) \
                > /dev/null 2>&1; exit 0

.PHONY: mkc-distclean
mkc-distclean:	$(MKC_FILET)
	-$(MAKE) mkc-clean > /dev/null 2>&1
	-rm -rf mkc_* mkconfig.cache mkconfig.log \
		$(MKC_FILES) $(MKC_OUTPUT) $(MKC_ENVFILE) \
		_tmp_mkconfig _mkconfig_runtests \
                > /dev/null 2>&1; exit 0
