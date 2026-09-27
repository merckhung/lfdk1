# SPDX-License-Identifier: GPL-2.0
#
# LFDD - Linux Firmware Debug Driver
#
# Out-of-tree kernel module. Build against the running kernel with `make`, or
# against another tree with `make KDIR=/path/to/linux`.

ifneq ($(KERNELRELEASE),)

obj-m := lfdd.o
lfdd-y := lfdd_main.o lfdd_pci.o lfdd_mem.o lfdd_io.o lfdd_cmos.o

else

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean

endif
