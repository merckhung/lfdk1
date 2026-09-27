// LFDK - Linux Firmware Debug Kit
//
// PCI device list and PCI / PCI Express configuration space editor.
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
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "lfdk.h"

enum {
  kMaxFunctions = 1024,
  kMaxName = 80,
  kListRows = 20,
};

typedef struct {
  uint8_t bus;
  uint8_t dev;
  uint8_t fn;
  uint16_t vendor_id;
  uint16_t device_id;
  char vendor_name[kMaxName];
  char device_name[kMaxName];
} PciFunction;

static PciFunction functions[kMaxFunctions];
static int num_functions;

// PCI list state: first visible row and highlighted row.
static int list_top;
static int list_bar;

// PCI device state.
static int current;
static unsigned int window;  // Offset of the displayed 256-byte window.
static HexCursor cursor;

static uint32_t Le32(const uint8_t* p) {
  return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

static bool ReadConfig(const PciFunction* f, unsigned int reg, int width,
                       uint32_t* value) {
  struct lfdd_pci req = {
      .bus = f->bus, .dev = f->dev, .fn = f->fn, .reg = reg, .width = width};

  if (!LfddIoctl(LFDD_PCI_READ, &req)) return false;
  *value = req.value;
  return true;
}

static void ReadBlock(const PciFunction* f, unsigned int reg, uint8_t* buf) {
  struct lfdd_pci req = {.bus = f->bus, .dev = f->dev, .fn = f->fn, .reg = reg};

  if (LfddIoctl(LFDD_PCI_READ_BLOCK, &req))
    memcpy(buf, req.block, LFDD_BLOCK_SIZE);
  else
    memset(buf, 0xff, LFDD_BLOCK_SIZE);
}

void PciScan(int max_bus) {
  num_functions = 0;

  for (int bus = 0; bus <= max_bus; bus++) {
    UiPrint(kInfoRow, 0, kColorBody, "Scanning PCI bus %02X ...", bus);
    refresh();

    for (int dev = 0; dev < 32; dev++) {
      for (int fn = 0; fn < 8; fn++) {
        PciFunction f = {.bus = bus, .dev = dev, .fn = fn};
        uint32_t id;
        uint32_t header_type;

        if (!ReadConfig(&f, 0x00, 4, &id)) return;
        if ((id & 0xffff) == 0xffff || (id & 0xffff) == 0x0000) {
          // Functions 1-7 may exist without function 0 only if it exists.
          if (fn == 0) break;
          continue;
        }

        f.vendor_id = id & 0xffff;
        f.device_id = id >> 16;
        if (num_functions < kMaxFunctions) functions[num_functions++] = f;

        // A single-function device only decodes function 0.
        if (fn == 0 && ReadConfig(&f, 0x0e, 1, &header_type) &&
            !(header_type & 0x80))
          break;
      }
    }
  }
}

// Copies the name that follows the ID of a pci.ids line.
static void CopyName(char* dst, const char* line) {
  while (isxdigit((unsigned char)*line)) line++;
  while (isspace((unsigned char)*line)) line++;
  snprintf(dst, kMaxName, "%s", line);
  dst[strcspn(dst, "\r\n")] = '\0';
}

// Resolves vendor and device names of all functions in a single pass over the
// pci.ids database (https://pci-ids.ucw.cz/).
void PciLoadNames(const char* path) {
  char line[512];
  unsigned int vendor = 0x10000;
  FILE* file;

  if (!path || !(file = fopen(path, "r"))) return;

  while (fgets(line, sizeof(line), file)) {
    unsigned int id;

    if (line[0] == '#' || line[0] == '\n') continue;
    // The device class list follows the vendor list.
    if (line[0] == 'C' && line[1] == ' ') break;

    if (line[0] != '\t') {
      if (sscanf(line, "%4x", &id) != 1) id = 0x10000;
      vendor = id;
      for (int i = 0; i < num_functions; i++)
        if (functions[i].vendor_id == vendor)
          CopyName(functions[i].vendor_name, line);
    } else if (line[1] != '\t' && vendor < 0x10000 &&
               sscanf(line + 1, "%4x", &id) == 1) {
      for (int i = 0; i < num_functions; i++)
        if (functions[i].vendor_id == vendor && functions[i].device_id == id)
          CopyName(functions[i].device_name, line + 1);
    }
  }
  fclose(file);
}

// ---------------------------------------------------------------------------
// PCI device list.

ScreenId PciListKey(int key) {
  switch (key) {
    case KEY_UP:
      if (list_bar > 0)
        list_bar--;
      else if (list_top > 0)
        list_top--;
      break;
    case KEY_DOWN:
      if (list_top + list_bar + 1 >= num_functions) break;
      if (list_bar + 1 < kListRows)
        list_bar++;
      else
        list_top++;
      break;
    case KEY_PPAGE:
      list_top -= kListRows;
      if (list_top < 0) list_top = 0;
      list_bar = 0;
      break;
    case KEY_NPAGE:
      list_top += kListRows;
      if (list_top > num_functions - kListRows)
        list_top = num_functions - kListRows;
      if (list_top < 0) list_top = 0;
      list_bar = 0;
      break;
    case '\n':
    case KEY_ENTER:
      if (num_functions == 0) break;
      current = list_top + list_bar;
      window = 0;
      cursor = (HexCursor){0};
      return kScreenPciDevice;
    default:
      break;
  }
  return kScreenPciList;
}

void PciListDraw(void) {
  UiFillRow(1, kColorHeader);
  UiPrint(1, 0, kColorHeader, "%-50s%s", "Name",
          "Vendor  Device  Bus# Dev# Fun#");

  for (int row = 0; row < kListRows; row++) {
    const PciFunction* f;
    char name[kMaxName * 2];
    int i = list_top + row;

    if (i >= num_functions) break;
    f = &functions[i];

    if (f->vendor_name[0] && f->device_name[0])
      snprintf(name, sizeof(name), "%.20s, %s", f->vendor_name, f->device_name);
    else if (f->vendor_name[0])
      snprintf(name, sizeof(name), "%.20s, %04X", f->vendor_name, f->device_id);
    else
      snprintf(name, sizeof(name), "%04X, %04X", f->vendor_id, f->device_id);

    UiPrint(2 + row, 0, row == list_bar ? kColorCursor : kColorBody,
            "%-49.49s ", name);
    UiPrint(2 + row, 50, kColorBody, "%04X    %04X     %02X   %02X   %02X",
            f->vendor_id, f->device_id, f->bus, f->dev, f->fn);
  }

  if (num_functions == 0) UiPrint(3, 2, kColorBody, "No PCI devices found.");
  UiPrint(kInfoRow, 0, kColorBody, "Type: PCI    %d functions found",
          num_functions);
}

// ---------------------------------------------------------------------------
// PCI configuration space editor.

ScreenId PciDeviceKey(int key) {
  if (num_functions == 0) return kScreenPciList;

  switch (key) {
    case KEY_NPAGE:
      current = (current + 1) % num_functions;
      window = 0;
      cursor.editing = false;
      return kScreenPciDevice;
    case KEY_PPAGE:
      current = (current + num_functions - 1) % num_functions;
      window = 0;
      cursor.editing = false;
      return kScreenPciDevice;
    case ']':
      if (window + LFDD_BLOCK_SIZE < LFDD_PCI_CFG_SPACE_SIZE)
        window += LFDD_BLOCK_SIZE;
      cursor.editing = false;
      return kScreenPciDevice;
    case '[':
      if (window > 0) window -= LFDD_BLOCK_SIZE;
      cursor.editing = false;
      return kScreenPciDevice;
    default:
      break;
  }

  if (HexCursorKey(&cursor, key) == kHexCommit) {
    const PciFunction* f = &functions[current];
    struct lfdd_pci req = {.bus = f->bus,
                           .dev = f->dev,
                           .fn = f->fn,
                           .reg = window + HexCursorOffset(&cursor),
                           .width = 1,
                           .value = cursor.pending};
    LfddIoctl(LFDD_PCI_WRITE, &req);
  }
  return kScreenPciDevice;
}

// Prints the base address registers of a type 0 or type 1 header.
static int DrawBars(const uint8_t* cfg, int row) {
  int header = cfg[0x0e] & 0x7f;
  int num_bars = header == 0 ? 6 : header == 1 ? 2 : 0;

  for (int i = 0; i < num_bars; i++) {
    uint32_t bar = Le32(cfg + 0x10 + i * 4);

    if (bar & 0x1) {
      UiPrint(row++, kSideCol, kColorLabel, "BAR%d I/O %08X", i, bar & ~0x3u);
    } else if ((bar & 0x6) == 0x4 && i + 1 < num_bars) {
      uint64_t addr = (uint64_t)Le32(cfg + 0x14 + i * 4) << 32 | (bar & ~0xfu);
      UiPrint(row++, kSideCol, kColorLabel, "BAR%d M64 %012llX", i,
              (unsigned long long)addr);
      i++;
    } else {
      UiPrint(row++, kSideCol, kColorLabel, "BAR%d M32 %08X", i, bar & ~0xfu);
    }
  }

  if (header == 0 || header == 1) {
    uint32_t rom = Le32(cfg + (header == 0 ? 0x30 : 0x38));
    UiPrint(row++, kSideCol, kColorLabel, "ROM      %08X", rom & ~0x7ffu);
  }
  return row;
}

void PciDeviceDraw(bool blink) {
  const PciFunction* f = &functions[current];
  uint8_t header[LFDD_BLOCK_SIZE];
  uint8_t data[LFDD_BLOCK_SIZE];
  int row = kGridRow;

  if (num_functions == 0) return;

  ReadBlock(f, 0, header);
  if (window == 0)
    memcpy(data, header, sizeof(data));
  else
    ReadBlock(f, window, data);

  if (f->vendor_name[0])
    UiPrint(1, 1, kColorLabel, "Vendor: %.70s", f->vendor_name);
  else
    UiPrint(1, 1, kColorLabel, "Unknown Vendor");
  if (f->device_name[0])
    UiPrint(2, 1, kColorLabel, "Device: %.70s", f->device_name);
  else
    UiPrint(2, 1, kColorLabel, "Unknown Device");

  UiDrawHexGrid(data, &cursor, window, false, blink);

  UiPrint(row++, kSideCol, kColorLabel, "VID:DID  %04X:%04X", f->vendor_id,
          f->device_id);
  UiPrint(row++, kSideCol, kColorLabel, "Rev ID   %02X", header[0x08]);
  UiPrint(row++, kSideCol, kColorLabel, "Class    %02X%02X%02X", header[0x0b],
          header[0x0a], header[0x09]);
  UiPrint(row++, kSideCol, kColorLabel, "Header   %02X", header[0x0e]);
  UiPrint(row++, kSideCol, kColorLabel, "IRQ      %02X  Pin %02X", header[0x3c],
          header[0x3d]);
  if ((header[0x0e] & 0x7f) == 1)
    UiPrint(row++, kSideCol, kColorLabel, "Bus      %02X/%02X/%02X",
            header[0x18], header[0x19], header[0x1a]);
  row++;
  DrawBars(header, row);

  UiPrint(kInfoRow, 0, kColorBody,
          "Type: PCI    Bus %02X  Dev %02X  Fun %02X    Offset %03X-%03X",
          f->bus, f->dev, f->fn, window, window + LFDD_BLOCK_SIZE - 1);
}
