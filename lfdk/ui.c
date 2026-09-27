// LFDK - Linux Firmware Debug Kit
//
// Shared ncurses drawing helpers and the hex editor grid.
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
#include <stdarg.h>
#include <time.h>

#include "lfdk.h"

void UiInit(void) {
  initscr();
  start_color();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  set_escdelay(25);

  init_pair(kColorTitle, COLOR_WHITE, COLOR_RED);
  init_pair(kColorBody, COLOR_WHITE, COLOR_BLUE);
  init_pair(kColorHelp, COLOR_BLACK, COLOR_WHITE);
  init_pair(kColorLabel, COLOR_CYAN, COLOR_BLUE);
  init_pair(kColorOffset, COLOR_RED, COLOR_BLUE);
  init_pair(kColorNonZero, COLOR_YELLOW, COLOR_BLUE);
  init_pair(kColorHeader, COLOR_BLACK, COLOR_GREEN);
  init_pair(kColorCursor, COLOR_BLACK, COLOR_YELLOW);
  init_pair(kColorEditOn, COLOR_YELLOW, COLOR_RED);
  init_pair(kColorEditOff, COLOR_YELLOW, COLOR_BLACK);
  init_pair(kColorError, COLOR_WHITE, COLOR_RED);

  bkgdset(COLOR_PAIR(kColorBody) | ' ');
}

void UiShutdown(void) { endwin(); }

void UiPrint(int row, int col, int pair, const char* format, ...) {
  va_list args;

  attrset(COLOR_PAIR(pair) | A_BOLD);
  move(row, col);
  va_start(args, format);
  vw_printw(stdscr, format, args);
  va_end(args);
  attrset(A_NORMAL);
}

void UiFillRow(int row, int pair) {
  UiPrint(row, 0, pair, "%*s", kScreenCols, "");
}

void UiDrawFrame(const char* help, const char* status) {
  time_t now = time(NULL);
  struct tm tm;

  localtime_r(&now, &tm);

  UiFillRow(0, kColorTitle);
  UiPrint(0, 0, kColorTitle, "Linux Firmware Debug Kit %s", LFDK_VERSION);
  UiPrint(0, 48, kColorTitle, "Merck Hung <merckhung@gmail.com>");

  UiFillRow(kHelpRow, kColorHelp);
  UiPrint(kHelpRow, 0, kColorHelp, "%.70s", help);
  UiPrint(kHelpRow, 72, kColorHelp, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min,
          tm.tm_sec);

  if (status[0] != '\0')
    UiPrint(kStatusRow, 0, kColorError, " %-78.78s ", status);
}

bool UiHexDigit(int key, int* value) {
  if (key >= '0' && key <= '9') {
    *value = key - '0';
  } else if (key >= 'a' && key <= 'f') {
    *value = key - 'a' + 10;
  } else if (key >= 'A' && key <= 'F') {
    *value = key - 'A' + 10;
  } else {
    return false;
  }
  return true;
}

HexKeyResult HexCursorKey(HexCursor* cursor, int key) {
  int digit;

  switch (key) {
    case KEY_UP:
      if (cursor->row > 0) cursor->row--;
      break;
    case KEY_DOWN:
      if (cursor->row < kBytesPerLine - 1) cursor->row++;
      break;
    case KEY_LEFT:
      if (cursor->col > 0) cursor->col--;
      break;
    case KEY_RIGHT:
      if (cursor->col < kBytesPerLine - 1) cursor->col++;
      break;
    case 27:  // Escape.
      break;
    case '\n':
    case KEY_ENTER:
      if (!cursor->editing) return kHexHandled;
      cursor->editing = false;
      return kHexCommit;
    default:
      if (!UiHexDigit(key, &digit)) return kHexIgnored;
      if (!cursor->editing) {
        cursor->editing = true;
        cursor->pending = 0;
      }
      cursor->pending = (uint8_t)((cursor->pending << 4) | digit);
      return kHexHandled;
  }

  // Navigation and Escape abandon a value that is being typed.
  cursor->editing = false;
  return kHexHandled;
}

int HexCursorOffset(const HexCursor* cursor) {
  return cursor->row * kBytesPerLine + cursor->col;
}

static int ByteColor(const HexCursor* cursor, int row, int col, uint8_t value,
                     bool blink) {
  if (cursor && cursor->row == row && cursor->col == col) {
    if (!cursor->editing) return kColorCursor;
    return blink ? kColorEditOn : kColorEditOff;
  }
  return value ? kColorNonZero : kColorBody;
}

void UiDrawHexGrid(const uint8_t* data, const HexCursor* cursor,
                   unsigned int label_base, bool show_ascii, bool blink) {
  UiPrint(kGridRow, kGridCol, kColorOffset, "%04X", label_base & 0xffff);
  for (int col = 0; col < kBytesPerLine; col++)
    UiPrint(kGridRow, kGridCol + 5 + col * 3, kColorOffset, "%02X", col);

  if (show_ascii) UiPrint(kGridRow, 58, kColorLabel, "0123456789ABCDEF");

  for (int row = 0; row < kBytesPerLine; row++) {
    int y = kGridRow + 1 + row;

    UiPrint(y, kGridCol, kColorOffset, "%04X",
            (label_base + row * kBytesPerLine) & 0xffff);

    for (int col = 0; col < kBytesPerLine; col++) {
      uint8_t value = data[row * kBytesPerLine + col];
      bool at_cursor = cursor && cursor->row == row && cursor->col == col;

      if (at_cursor && cursor->editing) value = cursor->pending;
      UiPrint(y, kGridCol + 5 + col * 3,
              ByteColor(cursor, row, col, value, blink), "%02X", value);

      if (show_ascii) {
        uint8_t c = data[row * kBytesPerLine + col];
        UiPrint(y, 58 + col, kColorLabel, "%c",
                (c >= '!' && c <= '~') ? c : '.');
      }
    }
  }
}
