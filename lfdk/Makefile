CROSS   =
AS      = $(CROSS)as
AR      = $(CROSS)ar
CC      = $(CROSS)gcc
CPP     = $(CC) -E
LD      = $(CROSS)ld
NM      = $(CROSS)nm
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
RANLIB  = $(CROSS)ranlib
READELF = $(CROSS)readelf
SIZE    = $(CROSS)size
STRINGS = $(CROSS)strings
STRIP   = $(CROSS)strip

CFLAGS  = -g3
LDFLAGS = -lpanel -lncurses

PROGS   = lfdk

all: $(PROGS)

lfdk: lfdk.c libmem.c libpci.c libio.c libcmos.c
	$(CC) $(CFLAGS) -o $@ $? $(LDFLAGS)
	#$(STRIP) $(PROGS)

clean:
	rm -f *.o $(PROGS)

