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

release-gtk:
	BDJ4_BUILD=Release-gtk $(MAKE) cmake

debug-gtk:
	BDJ4_BUILD=Debug-gtk $(MAKE) cmake

profile-gtk:
	BDJ4_BUILD=Profile-gtk $(MAKE) cmake

debug-clang-gtk:
	BDJ4_BUILD=Debug-gtk $(MAKE) cmakeclang

sanitizeaddress:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmake

sanitizeaddressclang:
	BDJ4_BUILD=SanitizeAddress $(MAKE) cmakeclang

sanitizeleak:
	BDJ4_BUILD=SanitizeLeak $(MAKE) cmake

sanitizeundef:
	BDJ4_BUILD=SanitizeUndef $(MAKE) cmake

.PHONY: cmake cmakeclang cmake-unix cmake-windows

cmake:
	if test $$(uname -s) = Linux; then \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) COMPXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

cmakesinglecpu:
	if test $$(uname -s) = Linux; then \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-unix; \
	  $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) COMPXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  $(MAKE) build; \
	else \
	  COMP=$(GCC) COMPXX=$(GXX) $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

cmakeclang:
	if test $$(uname -s) = Linux; then \
	  COMP=$(CLANG) COMPXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) COMPXX=$(CLANGXX) $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  COMP=/mingw64/bin/clang.exe \
	  COMPXX=/mingw64/bin/clang++.exe \
	  $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

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

# windows installs extra unneeded crap
# --parallel does not work correctly on msys2
build:
	cmake --build $(BUILDDIR) $(pmode)
	cmake --install $(BUILDDIR)
	@-rm -f ../bin/liblib*.a

# testing
.PHONY: check checkfast checksuite testsuite testsection testtest teststart
check:
	(./utils/mktestsetup.sh;cd ..;./bin/bdj4 --check_all)

checkfast:
	(cd ..;CK_EXCLUDE_TAGS=slow ./bin/bdj4 --check_all)

checktag:
	(cd ..;CK_INCLUDE_TAGS=$(tag) ./bin/bdj4 --check_all)

checksuite:
	(cd ..;CK_RUN_SUITE=$(suite) ./bin/bdj4 --check_all)

testsuite:
	# --force is needed for windows
	./utils/mktestsetup.sh --force
	(cd ..;./bin/bdj4 --testsuite)

testsection:
	./utils/mktestsetup.sh
	(cd ..;./bin/bdj4 --testsuite --runsection $(section))

testtest:
	./utils/mktestsetup.sh
	(cd ..;./bin/bdj4 --testsuite --runtest $(test))

teststart:
	./utils/mktestsetup.sh
	(cd ..;./bin/bdj4 --testsuite --starttest $(start))

# recommended to change PLAYER and VOLUME in data/bdjconfig.txt to
# be the null libraries.
.PHONY: mcheck mchecksupp mcheckcheck
mcheck:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--read-var-info=yes \
		$(VALGRIND_OPTS) \
		./bin/bdj4 --debug 15)

mchecksupp:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--suppressions=src/utils/valgrind-suppression.txt \
		$(VALGRIND_OPTS) \
		./bin/bdj4 --debug 15)

mcheckcheck:
	(cd ..;valgrind --tool=memcheck \
		--leak-check=full \
		--read-inline-info=yes \
		--trace-children=yes \
		--track-origins=yes \
		--show-leak-kinds=all \
		--read-var-info=yes \
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
	@-test -d build && cmake --build build --target clean

.PHONY: eclean
eclean:
	@-$(RM) -f $(BINDIR)/*

.PHONY: tclean
tclean:
	@find .. -name '*~' -print0 | xargs -0 rm -f
	@find .. -name '*.orig' -print0 | xargs -0 rm -f
	@-$(RM) -f w ww www ../w ../ww ../www ../*/w ../*/ww ../*/www
	@-$(RM) -f po/w po/ww ../asan.* ./asan.*
	@-$(RM) -f test.log
	@-$(RM) -f ../vlc-log.txt vlc-log.txt
	@-$(RM) -f ../vgcore.* ../core gmon.out ../gmon.out
	@-$(RM) -f launcher.rc bdj4_icon.ico
	@-$(RM) -f selauncher.rc bdj4_icon_inst.ico
	@-$(RM) -f dep-in.txt dep-sort.txt

