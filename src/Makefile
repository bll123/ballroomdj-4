#!/usr/bin/make
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

MAKEFLAGS += --no-print-directory

BINDIR = ../bin
BUILDDIR = build
CLANG = clang
CLANGXX = clang++
CLANGDIR = ucrt64

# bdj4 options
BDJ4_UI = GTK3

.PHONY: release
release:
	BDJ4_BUILD=Release $(MAKE) cmake

.PHONY: memdebug
memdebug:
	BDJ4_BUILD=Memdebug $(MAKE) cmake

.PHONY: memdebug-sanitize
memdebug-sanitize:
	BDJ4_BUILD=Memdebug-Sanitize $(MAKE) cmake

.PHONY: singlecpu
singlecpu:
	BDJ4_BUILD=Release $(MAKE) cmakesinglecpu

.PHONY: debug
debug:
	BDJ4_BUILD=Debug $(MAKE) cmake

.PHONY: profile
profile:
	BDJ4_BUILD=Profile $(MAKE) cmake

.PHONY: debug-clang
debug-clang:
	BDJ4_BUILD=Debug $(MAKE) cmakeclang

.PHONY: sanitizeaddress
sanitizeaddress:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmake

.PHONY: sanitizeaddressclang
sanitizeaddressclang:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmakeclang

.PHONY: sanitizeaddressclang64
sanitizeaddressclang64:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmakeclang64

.PHONY: sanitizeundef
sanitizeundef:
	BDJ4_BUILD=SanitizeUndef $(MAKE) cmake

.PHONY: cmake cmakesinglecpu cmakesinglecpuverbose
.PHONY: cmakeclang cmake-unix cmake-windows

# parallel doesn't seem to work under msys2
.PHONY: cmake
cmake:
	@if test $$(uname -s) = Linux; then \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  CC=$(CLANG) CXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

.PHONY: cmakesinglecpu
cmakesinglecpu:
	@if test $$(uname -s) = Linux; then \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-unix; \
	  $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  CC=$(CLANG) CXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  $(MAKE) build; \
	else \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

.PHONY: cmakesinglecpuverbose
cmakesinglecpuverbose:
	@if test $$(uname -s) = Linux; then \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-unix; \
	  VERBOSE=1 $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  CC=$(CLANG) CXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  VERBOSE=1 $(MAKE) build; \
	else \
	  CC=$(CC) CXX=$(CXX) $(MAKE) cmake-windows; \
	  VERBOSE=1 $(MAKE) build; \
	fi

# for the windows clang64 msys2 environment
.PHONY: cmakeclang64
cmakeclang64:
	CLANGDIR=clang64 $(MAKE) cmakeclang

.PHONY: cmakeclang
cmakeclang:
	@if test $$(uname -s) = Linux; then \
	  CC=$(CLANG) CXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  CC=$(CLANG) CXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  CC=/$(CLANGDIR)/bin/clang.exe \
	  CXX=/$(CLANGDIR)/bin/clang++.exe \
	  $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

.PHONY: cmake-unix
cmake-unix:
	. ../VERSION.txt; \
	cmake \
		-DBDJ4_BUILD_VERS:STATIC=$${VERSION} \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-DBDJ4_UI=$(BDJ4_UI) \
		-S . -B $(BUILDDIR) -Werror=deprecated

.PHONY: cmake-windows
cmake-windows:
	. ../VERSION.txt; \
	cmake \
		-DBDJ4_BUILD_VERS:STATIC=$${VERSION} \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-DBDJ4_UI=$(BDJ4_UI) \
		-G "MSYS Makefiles" \
		-S . -B $(BUILDDIR) -Werror=deprecated

# cmake on windows installs extra unneeded crap
# --parallel does not work correctly on msys2
.PHONY: build
build:
	cmake --build $(BUILDDIR) $(pmode)
	cmake --install $(BUILDDIR)
	@-rm -f ../bin/liblib*.a

# testing

.PHONY: check
check:
	./utils/mktestsetup.sh --force
	(cd ..;./bin/bdj4 --check_all)

.PHONY: checktag
checktag:
	(cd ..;CK_INCLUDE_TAGS=$(tag) ./bin/bdj4 --check_all)

.PHONY: checksuite
checksuite:
	(cd ..;CK_RUN_SUITE=$(suite) ./bin/bdj4 --check_all)

# test suite

# testoption could have:
#   --pli VLC3/VLC4/GST/MPRISVLC
#   --vol pipewire
.PHONY: testsuite
testsuite:
	./utils/mktestsetup.sh --force $(testoption)
	(cd ..;./bin/bdj4 --testsuite)

.PHONY: testsuitemprisvlc
testsuitemprisvlc:
	./utils/mktestsetup.sh --force --pli MPRISVLC
	# & is a shell command terminator, no semicolon
	vlc --intf=dummy --no-video & \
        pid=$$$$; \
	(cd ..;./bin/bdj4 --testsuite); \
	kill $${pid}; \
	true

.PHONY: testsection
testsection:
	./utils/mktestsetup.sh $(testoption)
	(cd ..;./bin/bdj4 --testsuite --runsection $(start))

.PHONY: testtest
testtest:
	./utils/mktestsetup.sh $(testoption)
	(cd ..;./bin/bdj4 --testsuite --verbose --runtest $(start))

.PHONY: teststart
teststart:
	./utils/mktestsetup.sh $(testoption)
	(cd ..;./bin/bdj4 --testsuite --starttest $(start))

# cleaning

.PHONY: dataclean
dataclean:
	@-test -d ../data && $(RM) -rf ../data/*

.PHONY: distclean
distclean:
	@-$(RM) -rf build
	@-$(MAKE) clean
	@-$(RM) -f ../install/manifest.txt ../install/manifest-src.txt
	@-$(RM) -f libmpris/mpris-root.c libmpris/mpris-player.c
	@-$(RM) -f include/mpris-root.h include/mpris-player.h
	@-$(RM) -f testall.log
	@mkdir build

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(RM) -f zz ../zz ../*/zz
	@-$(MAKE) eclean
	@-test -f build/CMakeCache.txt && cmake --build build --target clean

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@find .. -name '*~' -print0 | xargs -0 rm -f
	@find .. -name '*.orig' -print0 | xargs -0 rm -f
	@find ../wiki -name '*.txt.bak' -print0 | xargs -0 rm -f
	@-$(RM) -f w ww www ../w ../ww ../www */w */ww ../*/w ../*/ww ../*/www
	@-$(RM) -f ../asan.* ./asan.* po/*.debug
	@-$(RM) -f ../mdebug.txt ../out.txt out.txt
	@-$(RM) -f ../out-acoustid.xml ../out-acr.json ../out-mb.xml
	@-$(RM) -f ../vlc-log.txt vlc-log.txt ../vlc-out.txt ../mpvdebug.txt
	@-$(RM) -f ../vgcore.* vgcore.* ../core core gmon.out ../gmon.out
	@-$(RM) -f launcher.rc bdj4_icon.ico
	@-$(RM) -f selauncher.rc bdj4_icon_inst.ico
	@-$(RM) -f dep-*.txt ../tmp/*.dot $$HOME/.cache/BDJ4/volreg.*
