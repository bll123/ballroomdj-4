#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

TMPLDIR = ../../templates
INSTDIR = ../../install
PODIR = po

TMPLOUT=potemplates.c

TMPLFILES = \
	$(TMPLDIR)/dancetypes.txt \
	$(TMPLDIR)/dances.txt \
	$(TMPLDIR)/ratings.txt \
	$(TMPLDIR)/genres.txt \
	$(TMPLDIR)/levels.txt \
	$(TMPLDIR)/status.txt \
	$(TMPLDIR)/bdjconfig.txt.p \
	$(TMPLDIR)/bdjconfig.q0.txt \
	$(TMPLDIR)/bdjconfig.q1.txt \
	$(TMPLDIR)/bdjconfig.q2.txt \
	$(TMPLDIR)/bdjconfig.q3.txt \
	$(TMPLDIR)/bdjconfig.q4.txt \
	$(TMPLDIR)/bdj.html \
	$(TMPLDIR)/bdj-flex.html \
	$(TMPLDIR)/bdj-flex-dark.html \
	$(INSTDIR)/bdj4.desktop

all: potemplates.c \
	rebuildpot bdj4.pot $(PODIR)/en_GB.po $(PODIR)/en_US.po

potemplates.c:	$(TMPLFILES) poexttmpl.sh
	@echo "-- $$(date +%T) update potemplates.c"
	@./poexttmpl.sh

# for now, skip any method to create the deps on *all* the .c
# files containing a translatable string.
# let the script replace bdj4.pot if necessary.
.PHONY: rebuildpot
rebuildpot:
	@./poextract.sh

$(PODIR)/en_GB.po: bdj4.pot po-en-gb.sh po-mk.sh
	@echo "-- $$(date +%T) update en_GB.po"
	@./po-en-gb.sh

$(PODIR)/en_US.po: $(PODIR)/en_GB.po po-en-us.sh mken_us.awk
	@echo "-- $$(date +%T) update en_US.po"
	@./po-en-us.sh

# localization.txt file
# this does not go into the 'all' target.

# prevent nothing to be done message
.PHONY: ltxt
ltxt: $(TMPLDIR)/localization.txt
	@:

# gnu make extension: wildcard
$(TMPLDIR)/localization.txt: complete.txt $(wildcard $(PODIR)/*.po) po-ltxt.sh
	@echo "-- $$(date +%T) updating localization.txt"
	@./po-ltxt.sh
