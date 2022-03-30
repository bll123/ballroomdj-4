#!/usr/bin/make
#

MAKEFLAGS += --no-print-directory

BINDIR = ../bin
BUILDDIR = build
GCC = gcc
GXX = g++
CLANG = clang
CLANGXX = clang++

.PHONY: build install debug release
.PHONY: sanitizeaddress sanitizeleak sanitizeundef

debug:
	BDJ4_BUILD=Debug $(MAKE) cmake

release:
	BDJ4_BUILD=Release $(MAKE) cmake

sanitizeaddress:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmake

sanitizeleak:
	BDJ4_BUILD=SanitizeLeak $(MAKE) cmake

sanitizeundef:
	BDJ4_BUILD=SanitizeUndef $(MAKE) cmake

cmake:
	if test $$(uname -s) = Linux; then \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-unix; \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) COMPXX=$(CLANGXX) $(MAKE) cmake-unix; \
	else \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-windows; \
	fi
	$(MAKE) build

cmake-unix:
	cmake \
		-DCMAKE_C_COMPILER=$(COMP) \
		-DCMAKE_CXX_COMPILER=$(COMPXX) \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-S . -B $(BUILDDIR) -Werror=deprecated

cmake-windows:
	cmake \
		-DCMAKE_C_COMPILER="$(COMP)" \
		-DCMAKE_CXX_COMPILER="$(COMPXX)" \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-G "MSYS Makefiles" \
		-S . -B $(BUILDDIR) -Werror=deprecated

build:
	cmake --build $(BUILDDIR)
	cmake --install $(BUILDDIR)

# testing
.PHONY: check
check:
	../bin/check_all 2>&1 | tee w

# recommended to change PLAYER and VOLUME in data/bdjconfig.txt to
# be the null libraries.
.PHONY: mcheck mcheckmain mchecksupp
mcheck:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		$(VALGRIND_OPTS) \
		./bin/bdj4 --playerui --debug 15)

mcheckmain:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		$(VALGRIND_OPTS) \
		./bin/bdj4 --main --debug 15)

mchecksupp:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		$(VALGRIND_OPTS) \
		./bin/bdj4 --playerui --debug 15)

mcheckcheck:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		$(VALGRIND_OPTS) \
		./bin/check_all)

# cleaning

.PHONY: dataclean
dataclean:
	@-test -d ../data && $(RM) -rf ../data/*

.PHONY: distclean
distclean:
	@-$(MAKE) clean
	@-$(RM) -rf build/*
	@-$(RM) -f ../install/manifest.txt ../install/manifest-src.txt

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(MAKE) eclean
	@cmake --build build --target clean

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@find .. -name '*~' -print0 | xargs -0 rm -f
	@find .. -name '*.orig' -print0 | xargs -0 rm -f
	@-$(RM) -f w ww ../w ../ww po/w ../asan.* ./asan.*
	@-$(RM) -f ../vlc-log.txt vlc-log.txt
	@-$(RM) -f ../vgcore.* ../core
	@-$(RM) -f launcher.rc bdj4_icon.ico
	@-$(RM) -f selauncher.rc bdj4_icon_inst.ico
	@-$(RM) -f si-in.txt si-sort.txt

