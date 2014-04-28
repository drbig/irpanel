PARTS=firmware irpaneld tools
CLOC=cloc.pl

BUILDDIRS=$(PARTS:%=build-%)
CLEANDIRS=$(PARTS:%=clean-%)

all: $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) all

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

docs:
	doxygen doxygen.conf

stats:
	$(CLOC) $(PARTS)

.PHONY: subdirs $(PARTS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all clean docs stats
