# LFDD/LFDK - Linux Firmware Debug Driver and Kit
#
#   make                 Build the kernel module and the tool into bin/.
#   make KDIR=<path>     Build the module against another kernel tree.
#   make format          Format all C sources with clang-format (Google style).
#   make update-pciids   Download the latest PCI ID database.

KDIR ?= /lib/modules/$(shell uname -r)/build
PCI_IDS_URL ?= https://pci-ids.ucw.cz/v2.2/pci.ids
SOURCES := $(wildcard lfdd/*.[ch] lfdk/*.[ch])

all: driver tool
	mkdir -p bin
	cp -f lfdd/lfdd.ko lfdk/lfdk lfdk/pci.ids bin/

driver:
	$(MAKE) -C lfdd KDIR=$(KDIR)

tool:
	$(MAKE) -C lfdk

install:
	$(MAKE) -C lfdk install

format:
	clang-format -i $(SOURCES)

update-pciids:
	curl -fsSL -o lfdk/pci.ids.tmp $(PCI_IDS_URL)
	mv lfdk/pci.ids.tmp lfdk/pci.ids

clean:
	$(MAKE) -C lfdd KDIR=$(KDIR) clean
	$(MAKE) -C lfdk clean
	rm -rf bin

.PHONY: all driver tool install format update-pciids clean
