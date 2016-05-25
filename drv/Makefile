srcdir=.
top_srcdir=..

ppddir=ppd

models=cp290mrs.ppd \
       cp324mrs.ppd \
       cp424mrs.ppd \
       cp295mrs.ppd \
       cp305mrs.ppd \
       cp405mrs.ppd \
       cp205mrs.ppd \
       epm205mrs.ppd \
       epm224mrs.ppd \
       epm203mrs.ppd \
       bpm205.ppd \
       bpm224l.ppd \
       bpm224.ppd \
       cp290hrs.ppd \
       cp324hrs.ppd \
       cp324hrsw.ppd \
       cp424hrs.ppd \
       epm205hrs.ppd \
       kcp200.ppd \
       hsp3100fc.ppd \
       pos1525.ppd \
	   fcb500.ppd  \
	   lcb500.ppd \
	   lpm400.ppd

datadir=`cups-config --datadir`
modeldir=$(datadir)/model

INSTALL=/usr/bin/install

#search path of targets and prerequisites
VPATH=$(ppddir)

all: $(models)

$(models): aps.drv aps_option.defs aps_common.defs $(top_srcdir)/aps/models.def $(top_srcdir)/aps/version.def
	echo "Genrating PPD file for $@..."
	ppdc -I $(top_srcdir) -d $(ppddir) aps.drv

clean:

purge_ppd:
	@echo "remove all ppd... in $(ppddir)"
	@rm $(addprefix $(ppddir)/,$(models))

install:
	@echo "Install PPD files ..."
	@for model in $(models); do\
		echo Installing printer model $$model... ;\
		$(INSTALL) -m 644 $(ppddir)/$$model $(modeldir) || exit 1;\
	done

uninstall:
	@echo "Unrinstall PPD files ..."
	@for model in $(models); do\
		echo Uninstalling printer model $$model... ;\
		$(RM) $(modeldir)/$$model ;\
	done

