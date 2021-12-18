
TCLPATH = /usr
TCLVER = 8.5

TCLCONFIG = $(TCLPATH)/lib/tcl$(TCLVER)/tclConfig.sh
TCLINCLUDE = $(TCLPATH)/include/tcl$(TCLVER)
CC = cc
DC = dmd
MKCONFIGPATH = ../..

.PHONY: all
all:	tcl/tcl.d tk/tk.d x11/x.d x11/xlib.d

.PHONY: clean
clean:
	-rm -rf hw_tk_d1 hw_tk_d2 *.o \
		*.log *.vars *.cache _tmp_mkconfig

distclean:
	@$(MAKE) clean
	-rm -rf tcl.env tcl tk x11

tcl.env:	env.dat
	CC=$(CC) DC=$(DC) CFLAGS="-I$(TCLINCLUDE) -I$(TCLINCLUDE)/generic" \
		TCLVER=$(TCLVER) TCLPATH=$(TCLPATH) \
		$(MKCONFIGPATH)/mkconfig.sh env.dat

tcl/tcl.d:	tcl.dat tcl.env
	. ./tcl.env; \
		CFLAGS="$${TCL_DEFS} $${CFLAGS} " \
		LIBS="$${TCL_LIB_FLAG} $${TCL_LIBS}" \
		$(MKCONFIGPATH)/mkconfig.sh tcl.dat

tk/tk.d:	tk.dat tcl.env
	. ./tcl.env; \
		CFLAGS="$${TK_INCLUDE_SPEC} $${TK_DEFS} $${CFLAGS} " \
		LIBS="$${TK_LIB_FLAG} $${TK_LIBS}" \
		$(MKCONFIGPATH)/mkconfig.sh tk.dat

x11/x.d x11/xlib.d:	x.dat tcl.env
	. ./tcl.env; \
		CFLAGS="$${CFLAGS} " LIBS="$${LIBS} " \
		$(MKCONFIGPATH)/mkconfig.sh x.dat

TKFILES=tcl/tcl.d tk/tk.d x11/x.d x11/xlib.d

hw_tk_d1:	hw_tk_d1.d $(TKFILES)
	. ./tcl.env; \
		$(MKCONFIGPATH)/mkc.sh -complink -c $(DC) -o hw_tk_d1 -- \
		$(DFLAGS) $(LDFLAGS) \
		$${TK_LIB_SPEC} $${TK_LIBS} \
		$${TCL_LIB_SPEC} $${TCL_LIBS} hw_tk_d1.d $(TKFILES)

hw_tk_d2:	hw_tk_d2.d $(TKFILES)
	. ./tcl.env; \
		$(MKCONFIGPATH)/mkc.sh -complink -c $(DC) -o hw_tk_d2 -- \
		$(DFLAGS) $(LDFLAGS) \
		$${TK_LIB_SPEC} $${TK_LIBS} \
		$${TCL_LIB_SPEC} $${TCL_LIBS} hw_tk_d2.d $(TKFILES)
