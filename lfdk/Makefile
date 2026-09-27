# LFDK - Linux Firmware Debug Kit
#
#   make            Build lfdk.
#   make STATIC=1   Build a statically linked lfdk (e.g. for an initramfs).
#   make install    Install lfdk and pci.ids under $(PREFIX).

CC ?= gcc
PKG_CONFIG ?= pkg-config
PREFIX ?= /usr/local
DATADIR ?= $(PREFIX)/share/lfdk

CFLAGS ?= -O2 -g
CFLAGS += -std=gnu11 -Wall -Wextra -DLFDK_DATADIR='"$(DATADIR)"'

ifeq ($(STATIC),1)
LDFLAGS += -static
NCURSES_LIBS := $(shell $(PKG_CONFIG) --static --libs ncurses 2>/dev/null || echo -lncurses -ltinfo)
else
NCURSES_LIBS := $(shell $(PKG_CONFIG) --libs ncurses 2>/dev/null || echo -lncurses)
endif
LDLIBS += $(NCURSES_LIBS)

OBJS := lfdk.o ui.o pci.o space.o

all: lfdk

lfdk: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJS): lfdk.h ../lfdd/lfdd_ioctl.h

install: lfdk
	install -D -m 0755 lfdk $(DESTDIR)$(PREFIX)/bin/lfdk
	install -D -m 0644 pci.ids $(DESTDIR)$(DATADIR)/pci.ids

clean:
	rm -f lfdk $(OBJS)

.PHONY: all install clean
