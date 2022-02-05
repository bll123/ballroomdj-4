#!/usr/bin/make
#

MAKEFLAGS += --no-print-directory

BINDIR = ../bin
BUILDDIR = build

.PHONY: all build install
all:
	$(MAKE) cmake
	$(MAKE) build

cmake:
	if test \( $$(uname -s) = Linux \) -o \
		\( $$(uname -s) = Darwin \) ; then \
	  $(MAKE) cmake-unix; \
	else \
	  $(MAKE) cmake-windows; \
	fi

cmake-unix:
	cmake -S . -B $(BUILDDIR) -Werror=deprecated

cmake-windows:
	cmake -G "MSYS Makefiles" -S . -B $(BUILDDIR) -Werror=deprecated

build:
	cmake --build $(BUILDDIR)
	cmake --install $(BUILDDIR)

# windows


# testing
.PHONY: check
check:
	../bin/check_all 2>&1 | tee w

# recommended to change PLAYER and VOLUME in data/bdjconfig.txt to
# be the null libraries.
.PHONY: mcheck mchecksupp mcheckcli
mcheck: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); \
	$(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		./bin/bdj4 --main --debug 11)

mchecksupp: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); \
	$(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/bdj4 --main --debug 11)

mcheckpluisupp: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); \
	$(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/bdj4 --playerui --debug 11)

mcheckcli: $(MKC_ENV_SHR)
	. ./$(MKC_ENV_SHR); $(MAKE) -e rall
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		./bin/bdj4 --bdj4cli)

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

# cleaning

.PHONY: dataclean
dataclean:
	@-test -d ../data && $(RM) -rf ../data/*

.PHONY: distclean
distclean:
	@-$(MAKE) clean
	@-$(RM) -rf build/*
	@-$(RM) -f ../install/manifest.txt ../install/src-manifest.txt

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(MAKE) eclean

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@-$(RM) -f *~ w ww *.orig
	@-$(RM) -f ../conv/*~ ./locale/*~ ../locale/*~ ../*~
	@-$(RM) -f ../install/*~ ../pkg/*~ utils/*~ ../data/*~
	@-$(RM) -f ../templates/*~ ../vlc-log.txt vlc-log.txt
	@-$(RM) -f si-in.txt si-sort.txt
	@-$(RM) -f ../vgcore.*
	@-$(RM) -f launcher.rc bdj4_icon.ico
	@-$(RM) -f ../web/*~ ../http/*~ ../core ../resources/*~

