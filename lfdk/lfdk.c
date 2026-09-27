// LFDK - Linux Firmware Debug Kit
//
// Entry point, command line parsing and the main event loop.
//
// Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "lfdk.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#ifndef LFDK_DATADIR
#define LFDK_DATADIR "/usr/local/share/lfdk"
#endif

enum {
  kRefreshMs = 100,  // Screen refresh period; also the blink period / 4.
  kMaxPciBus = 255,
};

static int lfdd_fd = -1;
static char status[kScreenCols + 1];

bool LfddIoctl(unsigned long request, void* arg) {
  if (ioctl(lfdd_fd, request, arg) == 0) return true;
  SetStatus("%s: %s", LFDD_DEVICE_PATH, strerror(errno));
  return false;
}

void SetStatus(const char* format, ...) {
  va_list args;

  va_start(args, format);
  vsnprintf(status, sizeof(status), format, args);
  va_end(args);
}

static void Usage(void) {
  fprintf(stderr,
          "%s %s, Linux Firmware Debug Kit\n"
          "Copyright (C) 2006 - 2026, Merck Hung <merckhung@gmail.com>\n"
          "\n"
          "Usage: %s [-h] [-d /dev/lfdd] [-n pci.ids] [-b 255]\n"
          "  -d  Device node of the lfdd driver (default %s)\n"
          "  -n  PCI ID database (default: bundled, then system copy)\n"
          "  -b  Highest PCI bus number to scan (default %d)\n"
          "  -h  Print this message\n",
          LFDK_PROGNAME, LFDK_VERSION, LFDK_PROGNAME, LFDD_DEVICE_PATH,
          kMaxPciBus);
}

// Returns the first pci.ids that exists: the one installed with lfdk, the
// one next to the executable, then the system copies.
static const char* FindPciIds(void) {
  static char next_to_exe[PATH_MAX + 16];
  char exe[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  const char* candidates[] = {
      LFDK_DATADIR "/pci.ids",
      NULL,
      "/usr/share/hwdata/pci.ids",
      "/usr/share/misc/pci.ids",
  };

  if (len > 0) {
    exe[len] = '\0';
    snprintf(next_to_exe, sizeof(next_to_exe), "%s/pci.ids", dirname(exe));
    candidates[1] = next_to_exe;
  }

  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
    if (candidates[i] && access(candidates[i], R_OK) == 0) return candidates[i];
  return NULL;
}

static const char* HelpOf(ScreenId screen) {
  switch (screen) {
    case kScreenPciList:
      return "(Q)uit (P)CI (M)emory (I)O CM(O)S   Enter:Open  PgUp/PgDn:Page";
    case kScreenPciDevice:
      return "(Q)uit (P)CI (M)emory (I)O CM(O)S   PgUp/PgDn:Device  [ ]:Offset";
    case kScreenCmos:
      return "(Q)uit (P)CI (M)emory (I)O CM(O)S   0-F:Edit  Enter:Write";
    default:
      return "(Q)uit (P)CI (M)emory (I)O CM(O)S   PgUp/PgDn:Page  (G)oto";
  }
}

static ScreenId HandleKey(ScreenId screen, int key) {
  switch (key) {
    case 'q':
    case 'Q':
      return kScreenQuit;
    case 'p':
    case 'P':
      return kScreenPciList;
    case 'm':
    case 'M':
      SpaceActivate(kScreenMemory);
      return kScreenMemory;
    case 'i':
    case 'I':
      SpaceActivate(kScreenIo);
      return kScreenIo;
    case 'o':
    case 'O':
      SpaceActivate(kScreenCmos);
      return kScreenCmos;
    default:
      break;
  }

  switch (screen) {
    case kScreenPciList:
      return PciListKey(key);
    case kScreenPciDevice:
      return PciDeviceKey(key);
    default:
      return SpaceKey(screen, key);
  }
}

static void Draw(ScreenId screen) {
  struct timespec now;
  bool blink;

  clock_gettime(CLOCK_MONOTONIC, &now);
  blink = (now.tv_nsec / 250000000) % 2;

  erase();
  switch (screen) {
    case kScreenPciList:
      PciListDraw();
      break;
    case kScreenPciDevice:
      PciDeviceDraw(blink);
      break;
    default:
      SpaceDraw(screen, blink);
      break;
  }
  UiDrawFrame(HelpOf(screen), status);
  refresh();
}

int main(int argc, char** argv) {
  const char* device = LFDD_DEVICE_PATH;
  const char* pci_ids = NULL;
  int max_bus = kMaxPciBus;
  ScreenId screen = kScreenPciList;
  int opt;

  while ((opt = getopt(argc, argv, "b:d:n:h")) != -1) {
    switch (opt) {
      case 'b':
        max_bus = atoi(optarg);
        if (max_bus < 0 || max_bus > kMaxPciBus) {
          fprintf(stderr, "PCI bus number must be 0-%d\n", kMaxPciBus);
          return 1;
        }
        break;
      case 'd':
        device = optarg;
        break;
      case 'n':
        pci_ids = optarg;
        break;
      default:
        Usage();
        return opt == 'h' ? 0 : 1;
    }
  }

  lfdd_fd = open(device, O_RDWR | O_CLOEXEC);
  if (lfdd_fd < 0) {
    fprintf(stderr, "Cannot open %s: %s\n", device, strerror(errno));
    fprintf(stderr, "Is the lfdd module loaded, and are you root?\n");
    return 1;
  }
  if (!pci_ids) pci_ids = FindPciIds();

  UiInit();
  if (LINES < kScreenRows || COLS < kScreenCols) {
    UiShutdown();
    fprintf(stderr, "The terminal must be at least %dx%d\n", kScreenCols,
            kScreenRows);
    close(lfdd_fd);
    return 1;
  }

  erase();
  UiDrawFrame(HelpOf(screen), "");
  PciScan(max_bus);
  PciLoadNames(pci_ids);
  if (!pci_ids) SetStatus("pci.ids not found; device names are unavailable");

  timeout(kRefreshMs);
  while (screen != kScreenQuit) {
    int key;

    Draw(screen);
    key = getch();
    if (key == ERR || key == KEY_RESIZE) continue;

    status[0] = '\0';
    screen = HandleKey(screen, key);
  }

  UiShutdown();
  close(lfdd_fd);
  return 0;
}
