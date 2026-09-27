// LFDK - Linux Firmware Debug Kit
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
#ifndef LFDK_LFDK_H_
#define LFDK_LFDK_H_

#include <stdbool.h>
#include <stdint.h>

#include "../lfdd/lfdd_ioctl.h"

#define LFDK_VERSION LFDD_VERSION
#define LFDK_PROGNAME "lfdk"

// Fixed 80x24 layout.
enum {
  kScreenRows = 24,
  kScreenCols = 80,
  kBytesPerLine = 16,
  kGridRow = 4,     // Row of the hex grid column header.
  kGridCol = 1,     // Column of the hex grid offset labels.
  kSideCol = 56,    // Column of the side panel next to the grid.
  kInfoRow = 22,    // Row of the per-screen information line.
  kStatusRow = 21,  // Row of transient error messages.
  kHelpRow = 23,    // Row of the key help bar.
};

enum ColorPair {
  kColorTitle = 1,  // White on red.
  kColorBody,       // White on blue.
  kColorHelp,       // Black on white.
  kColorLabel,      // Cyan on blue.
  kColorOffset,     // Red on blue.
  kColorNonZero,    // Yellow on blue.
  kColorHeader,     // Black on green.
  kColorCursor,     // Black on yellow.
  kColorEditOn,     // Yellow on red.
  kColorEditOff,    // Yellow on black.
  kColorError,      // White on red.
};

typedef enum {
  kScreenQuit,
  kScreenPciList,
  kScreenPciDevice,
  kScreenMemory,
  kScreenIo,
  kScreenCmos,
} ScreenId;

// lfdk.c

// Issues an ioctl to /dev/lfdd. On failure the error is shown on the status
// line and false is returned.
bool LfddIoctl(unsigned long request, void* arg);

// Shows a message on the status line until the next key press.
void SetStatus(const char* format, ...) __attribute__((format(printf, 1, 2)));

// ui.c

// Cursor of the 16x16 hex editor grid.
typedef struct {
  int row;
  int col;
  bool editing;     // A new value is being typed.
  uint8_t pending;  // The value typed so far.
} HexCursor;

typedef enum {
  kHexIgnored,  // The key is not a grid key.
  kHexHandled,  // The cursor moved or the pending value changed.
  kHexCommit,   // Enter was pressed; write |pending| at the cursor.
} HexKeyResult;

void UiInit(void);
void UiShutdown(void);
void UiPrint(int row, int col, int pair, const char* format, ...)
    __attribute__((format(printf, 4, 5)));
void UiFillRow(int row, int pair);
void UiDrawFrame(const char* help, const char* status);
bool UiHexDigit(int key, int* value);

HexKeyResult HexCursorKey(HexCursor* cursor, int key);
int HexCursorOffset(const HexCursor* cursor);

// Draws 256 bytes as a 16x16 grid. |label_base| is the offset shown on the
// first row. |cursor| may be NULL. |blink| toggles the edit highlight.
void UiDrawHexGrid(const uint8_t* data, const HexCursor* cursor,
                   unsigned int label_base, bool show_ascii, bool blink);

// pci.c
void PciScan(int max_bus);
void PciLoadNames(const char* path);
ScreenId PciListKey(int key);
void PciListDraw(void);
ScreenId PciDeviceKey(int key);
void PciDeviceDraw(bool blink);

// space.c (memory, I/O and CMOS views)
void SpaceActivate(ScreenId id);
ScreenId SpaceKey(ScreenId id, int key);
void SpaceDraw(ScreenId id, bool blink);

#endif  // LFDK_LFDK_H_
