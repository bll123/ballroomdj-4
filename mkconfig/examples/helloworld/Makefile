# Example Makefile
#
# OBJ_EXT and EXE_EXT are used for portability to windows.
#

MKC_DIR = ../..
MKC_CONFDIR = .
MKC_PREFIX = hello
MKC_OUTPUT = config.h
OBJ_EXT = .o
EXE_EXT = 

sources = hello.c
objects = hello$(OBJ_EXT)

include $(MKC_DIR)/Makefile.inc

.PHONY: clean
clean:
	$(MAKE) mkc-clean
	rm -f *~

.PHONY: distclean
distclean:
	$(MAKE) mkc-distclean
	rm -f *~
# DO NOT DELETE
hello.o: hello.c config.h hello.h
