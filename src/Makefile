#!/usr/bin/make
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

MAKEFLAGS += --no-print-directory

BINDIR = ../bin
BUILDDIR = build
CLANG = clang
CLANGXX = clang++
CLANGDIR = ucrt64
CMAKE = cmake
CMAKEWIN = ../packages/cmake-3/bin/cmake

.PHONY: release
release:
	$(MAKE) BDJ4_BUILD=Release cmake-build cmake-install

.PHONY: build-only
build-only:
	$(MAKE) BDJ4_BUILD=Release cmake-build

.PHONY: memdebug
memdebug:
	$(MAKE) BDJ4_BUILD=Memdebug cmake-build cmake-install

.PHONY: memwatch
memwatch:
	$(MAKE) BDJ4_BUILD=Memwatch cmake-build cmake-install

.PHONY: memwatch-sanitize
memwatch-sanitize:
	$(MAKE) BDJ4_BUILD=Memwatch-Sanitize cmake-build cmake-install

.PHONY: memdebug-sanitize
memdebug-sanitize:
	$(MAKE) BDJ4_BUILD=Memdebug-Sanitize cmake-build cmake-install

.PHONY: singlecpu
singlecpu:
	$(MAKE) BDJ4_BUILD=Release cmakesinglecpu cmake-install

.PHONY: debug
debug:
	$(MAKE) BDJ4_BUILD=Debug cmak-build cmake-install

.PHONY: profile
profile:
	$(MAKE) BDJ4_BUILD=Profile cmake-build cmake-install

.PHONY: debug-clang
debug-clang:
	$(MAKE) BDJ4_BUILD=Debug cmakeclang cmake-install

.PHONY: sanitizeaddress
sanitizeaddress:
	$(MAKE) BDJ4_BUILD=SanitizeAddress cmake-build cmake-install

.PHONY: sanitizethread
sanitizethread:
	$(MAKE) BDJ4_BUILD=SanitizeThread cmake-build cmake-install

.PHONY: sanitizeaddressclang
sanitizeaddressclang:
	$(MAKE) BDJ4_BUILD=SanitizeAddress cmakeclang

.PHONY: sanitizeaddressclang64
sanitizeaddressclang64:
	$(MAKE) BDJ4_BUILD=SanitizeAddress cmakeclang64

.PHONY: cmake cmakesinglecpu cmakesinglecpuverbose
.PHONY: cmakeclang cmake-unix cmake-windows

# parallel doesn't seem to work under msys2
.PHONY: cmake-build
cmake-build:
	@if test $$(uname -s) = Linux; then \
	  $(MAKE) CC=$(CC) CXX=$(CXX) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  $(MAKE) CC=$(CLANG) CXX=$(CLANGXX) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  $(MAKE) CC=$(CC) CXX=$(CXX) CMAKE=$(CMAKEWIN) cmake-windows; \
	  $(MAKE) CMAKE=$(CMAKEWIN) build; \
	fi

.PHONY: cmake-install
cmake-install:
	@if test $$(uname -s) = Linux; then \
	  pmode=--parallel $(MAKE) install; \
	elif test $$(uname -s) = Darwin; then \
	  pmode=--parallel $(MAKE) install; \
	else \
	  $(MAKE) CMAKE=$(CMAKEWIN) install; \
	fi

.PHONY: cmakesinglecpu
cmakesinglecpu:
	@if test $$(uname -s) = Linux; then \
	  $(MAKE) CC=$(CC) CXX=$(CXX) cmake-unix; \
	  $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  $(MAKE) CC=$(CLANG) CXX=$(CLANGXX) cmake-unix; \
	  $(MAKE) build; \
	else \
	  $(MAKE) CC=$(CC) CXX=$(CXX) CMAKE=$(CMAKEWIN) cmake-windows; \
	  $(MAKE) CMAKE=$(CMAKEWIN) build; \
	fi

.PHONY: cmakesinglecpuverbose
cmakesinglecpuverbose:
	@if test $$(uname -s) = Linux; then \
	  $(MAKE) CC=$(CC) CXX=$(CXX) cmake-unix; \
	  $(MAKE) VERBOSE=1 build; \
	elif test $$(uname -s) = Darwin; then \
	  $(MAKE) CC=$(CLANG) CXX=$(CLANGXX) cmake-unix; \
	  $(MAKE) VERBOSE=1 build; \
	else \
	  $(MAKE) CC=$(CC) CXX=$(CXX) CMAKE=$(CMAKEWIN) cmake-windows; \
	  $(MAKE) VERBOSE=1 CMAKE=$(CMAKEWIN) build; \
	fi

# for the windows clang64 msys2 environment
# for some reason, this doesn't work, change the CLANDIR variable
# at the top of the makefile.
.PHONY: cmakeclang64
cmakeclang64:
	$(MAKE) CLANGDIR=clang64 cmakeclang

.PHONY: cmakeclang
cmakeclang:
	@if test $$(uname -s) = Linux; then \
	  $(MAKE) CC=$(CLANG) CXX=$(CLANGXX) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  $(MAKE) CC=$(CLANG) CXX=$(CLANGXX) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  $(MAKE) \
	  CLANGDIR=$(CLANGDIR) \
	  CC=/$(CLANGDIR)/bin/clang.exe \
	  CXX=/$(CLANGDIR)/bin/clang++.exe \
          CMAKE=$(CMAKEWIN) \
	  cmake-windows; \
	  $(MAKE) CMAKE=$(CMAKEWIN) build; \
	fi

.PHONY: cmake-unix
cmake-unix:
	. ../VERSION.txt; \
	$(CMAKE) \
		-DBDJ4_BUILD_VERS:STATIC=$${VERSION} \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-DBDJ4_UI=$(BDJ4_UI) \
		-S . -B $(BUILDDIR) -Werror=deprecated

.PHONY: cmake-windows
cmake-windows:
	. ../VERSION.txt; \
	$(CMAKE) \
		-DBDJ4_BUILD_VERS:STATIC=$${VERSION} \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DBDJ4_BUILD:STATIC=$(BDJ4_BUILD) \
		-DBDJ4_UI=$(BDJ4_UI) \
		-G "MSYS Makefiles" \
		-S . -B $(BUILDDIR) -Werror=deprecated

# --parallel does not work correctly on msys2
.PHONY: build
build:
	$(CMAKE) --build $(BUILDDIR) $(pmode)

# cmake on windows installs extra unneeded crap
.PHONY: install
install:
	$(CMAKE) --install $(BUILDDIR)
	@-rm -f ../bin/liblib*.a

.PHONY: install-target
install-target:
	$(CMAKE) --install $(BUILDDIR) --target $(target)

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
#   --pli VLC3/VLC4/GST/MPRISVLC/WINMP
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

.PHONY: testsectionv
testsectionv:
	./utils/mktestsetup.sh $(testoption)
	(cd ..;./bin/bdj4 --testsuite --verbose --runsection $(start))

.PHONY: testtest
testtest:
	./utils/mktestsetup.sh $(testoption)
	(cd ..;./bin/bdj4 --testsuite --runtest $(start))

.PHONY: testtestv
testtestv:
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
	@-$(MAKE) realclean
	@-$(RM) -f testall.log zzz ../pkg.log

.PHONY: realclean
realclean:
	@-$(RM) -rf build
	@-$(MAKE) clean
	@-$(RM) -f ../install/manifest.txt ../install/manifest-src.txt
	@-$(RM) -f libmpris/mpris-root.c libmpris/mpris-player.c
	@-$(RM) -f include/mpris-root.h include/mpris-player.h
	@mkdir build

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-$(MAKE) eclean
	@-test ! -f build/CMakeCache.txt || \
	  $(CMAKE) --build build --target clean

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@find .. -name '*~' -print0 | xargs -0 rm -f
	@find .. -name '*.orig' -print0 | xargs -0 rm -f
	@find ../wiki -name '*.txt.bak' -print0 | xargs -0 rm -f
	@-$(RM) -f w ww www ../w ../ww ../www */w */ww ../*/w ../*/ww ../*/www
	@-$(RM) -f ww.v ../ww.v
	@-$(RM) -f ../asan.* ./asan.* po/*.debug po/temp.txt
	@-$(RM) -f ../tsan.* ./tsan.*
	@-$(RM) -f ../mdebug.txt ../out.txt out.txt
	@-$(RM) -f ../out-acoustid.xml ../out-acr.json ../out-mb.xml
	@-$(RM) -f ../vlc-log.txt vlc-log.txt ../vlc-out.txt ../mpvdebug.txt
	@-$(RM) -f ../vgcore.* vgcore.* ../core core ../core.[0-9]*
	@-$(RM) -f gmon.out ../gmon.out
	@-$(RM) -f ../tmp/*.dmp
	@-$(RM) -rf ../tmp/gst
	@-$(RM) -f launcher.rc bdj4_icon.ico
	@-$(RM) -f selauncher.rc bdj4_icon_inst.ico
	@-$(RM) -f dep-*.txt
