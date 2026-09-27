// LFDK - Linux Firmware Debug Kit
//
// Hex editors for physical memory, I/O port space and CMOS / NVRAM.
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
#include <ncurses.h>
#include <string.h>

#include "lfdk.h"

typedef struct {
  const char* name;
  const char* title;
  int addr_digits;     // Width of the address; 0 if it is fixed.
  uint64_t last_base;  // Highest base address of a 256-byte page.
  bool (*read_block)(uint64_t base, uint8_t* buf);
  bool (*write_byte)(uint64_t addr, uint8_t value);
} SpaceOps;

typedef struct {
  const SpaceOps* ops;
  uint64_t base;
  bool entering;  // The user is typing a new base address.
  bool typed;     // A digit has been typed since entering started.
  uint64_t entry;
  HexCursor cursor;
} SpaceView;

static bool MemReadBlock(uint64_t base, uint8_t* buf) {
  struct lfdd_mem req = {.addr = base};

  if (!LfddIoctl(LFDD_MEM_READ_BLOCK, &req)) return false;
  memcpy(buf, req.block, LFDD_BLOCK_SIZE);
  return true;
}

static bool MemWriteByte(uint64_t addr, uint8_t value) {
  struct lfdd_mem req = {.addr = addr, .width = 1, .value = value};
  return LfddIoctl(LFDD_MEM_WRITE, &req);
}

static bool IoReadBlock(uint64_t base, uint8_t* buf) {
  struct lfdd_io req = {.port = (uint16_t)base};

  if (!LfddIoctl(LFDD_IO_READ_BLOCK, &req)) return false;
  memcpy(buf, req.block, LFDD_BLOCK_SIZE);
  return true;
}

static bool IoWriteByte(uint64_t addr, uint8_t value) {
  struct lfdd_io req = {.port = (uint16_t)addr, .width = 1, .value = value};
  return LfddIoctl(LFDD_IO_WRITE, &req);
}

static bool CmosReadBlock(uint64_t base, uint8_t* buf) {
  struct lfdd_cmos req = {0};

  (void)base;
  if (!LfddIoctl(LFDD_CMOS_READ_BLOCK, &req)) return false;
  memcpy(buf, req.block, LFDD_CMOS_SIZE);
  return true;
}

static bool CmosWriteByte(uint64_t addr, uint8_t value) {
  struct lfdd_cmos req = {.index = (uint8_t)addr, .value = value};
  return LfddIoctl(LFDD_CMOS_WRITE, &req);
}

static const SpaceOps kMemOps = {
    .name = "Memory",
    .title = "Physical memory: System RAM, MMIO and firmware",
    .addr_digits = 16,
    .last_base = UINT64_MAX - 0xff,
    .read_block = MemReadBlock,
    .write_byte = MemWriteByte,
};

static const SpaceOps kIoOps = {
    .name = "I/O",
    .title = "I/O port space",
    .addr_digits = 4,
    .last_base = LFDD_IO_SPACE_SIZE - 0x100,
    .read_block = IoReadBlock,
    .write_byte = IoWriteByte,
};

static const SpaceOps kCmosOps = {
    .name = "CMOS",
    .title = "CMOS/NVRAM  00-7F: ports 70h/71h  80-FF: ports 72h/73h",
    .read_block = CmosReadBlock,
    .write_byte = CmosWriteByte,
};

static SpaceView mem_view = {.ops = &kMemOps};
static SpaceView io_view = {.ops = &kIoOps};
static SpaceView cmos_view = {.ops = &kCmosOps};

static SpaceView* ViewOf(ScreenId id) {
  switch (id) {
    case kScreenMemory:
      return &mem_view;
    case kScreenIo:
      return &io_view;
    default:
      return &cmos_view;
  }
}

void SpaceActivate(ScreenId id) {
  SpaceView* view = ViewOf(id);

  view->cursor.editing = false;
  if (view->ops->addr_digits) {
    view->entering = true;
    view->typed = false;
    view->entry = view->base;
  }
}

static void EnterAddressKey(SpaceView* view, int key) {
  uint64_t mask = view->ops->addr_digits >= 16
                      ? UINT64_MAX
                      : (UINT64_C(1) << (view->ops->addr_digits * 4)) - 1;
  int digit;

  if (key == '\n' || key == KEY_ENTER) {
    view->base =
        view->entry > view->ops->last_base ? view->ops->last_base : view->entry;
    view->entering = false;
  } else if (key == 27) {
    view->entering = false;
  } else if (key == KEY_BACKSPACE || key == 127 || key == '\b') {
    view->entry >>= 4;
  } else if (UiHexDigit(key, &digit)) {
    // The first digit replaces the current address.
    if (!view->typed) view->entry = 0;
    view->typed = true;
    view->entry = ((view->entry << 4) | digit) & mask;
  }
}

ScreenId SpaceKey(ScreenId id, int key) {
  SpaceView* view = ViewOf(id);

  if (view->entering) {
    EnterAddressKey(view, key);
    return id;
  }

  switch (key) {
    case KEY_NPAGE:
      if (view->base <= view->ops->last_base - LFDD_BLOCK_SIZE)
        view->base += LFDD_BLOCK_SIZE;
      else
        view->base = view->ops->last_base;
      view->cursor.editing = false;
      return id;
    case KEY_PPAGE:
      view->base =
          view->base >= LFDD_BLOCK_SIZE ? view->base - LFDD_BLOCK_SIZE : 0;
      view->cursor.editing = false;
      return id;
    case 'g':
    case 'G':
      SpaceActivate(id);
      return id;
    default:
      break;
  }

  if (HexCursorKey(&view->cursor, key) == kHexCommit)
    view->ops->write_byte(view->base + HexCursorOffset(&view->cursor),
                          view->cursor.pending);
  return id;
}

void SpaceDraw(ScreenId id, bool blink) {
  SpaceView* view = ViewOf(id);
  const SpaceOps* ops = view->ops;
  uint8_t data[LFDD_BLOCK_SIZE];

  if (view->entering || !ops->read_block(view->base, data))
    memset(data, 0xff, sizeof(data));

  UiPrint(1, 1, kColorLabel, "%s", ops->title);
  UiDrawHexGrid(data, view->entering ? NULL : &view->cursor, 0, true, blink);

  if (!ops->addr_digits) {
    UiPrint(kInfoRow, 0, kColorBody, "Type: %s", ops->name);
    return;
  }

  // "Type: " + 8 character name + " Address: " is 24 columns wide.
  const int col = 24;

  UiPrint(kInfoRow, 0, kColorBody, "Type: %-8s Address: ", ops->name);
  if (view->entering)
    UiPrint(kInfoRow, col, blink ? kColorEditOn : kColorEditOff, "%0*llX",
            ops->addr_digits, (unsigned long long)view->entry);
  else
    UiPrint(kInfoRow, col, kColorBody, "%0*llX", ops->addr_digits,
            (unsigned long long)view->base);
  UiPrint(kInfoRow, col + ops->addr_digits, kColorBody, "h");
  if (view->entering)
    UiPrint(kInfoRow, col + ops->addr_digits + 3, kColorLabel,
            "Type an address, then press Enter");
}
