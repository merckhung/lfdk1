// SPDX-License-Identifier: GPL-2.0
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * CMOS / NVRAM access.
 *
 * The standard bank (0x00-0x7f) is accessed through the kernel's
 * CMOS_READ()/CMOS_WRITE() helpers, which keep the NMI mask bit of port 0x70
 * intact. The extended bank (0x80-0xff) found on most PC chipsets is reached
 * through ports 0x72/0x73. Both are serialized against the RTC driver with
 * rtc_lock.
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#include <linux/mc146818rtc.h>
#include <linux/spinlock.h>

#include "lfdd.h"

#ifdef CONFIG_X86

#define LFDD_CMOS_BANK_SIZE 0x80
#define LFDD_CMOS_EXT_INDEX_PORT 0x72
#define LFDD_CMOS_EXT_DATA_PORT 0x73

static u8 lfdd_cmos_get(unsigned int index) {
  if (index < LFDD_CMOS_BANK_SIZE) return CMOS_READ(index);
  outb(index - LFDD_CMOS_BANK_SIZE, LFDD_CMOS_EXT_INDEX_PORT);
  return inb(LFDD_CMOS_EXT_DATA_PORT);
}

static void lfdd_cmos_set(unsigned int index, u8 value) {
  if (index < LFDD_CMOS_BANK_SIZE) {
    CMOS_WRITE(value, index);
    return;
  }
  outb(index - LFDD_CMOS_BANK_SIZE, LFDD_CMOS_EXT_INDEX_PORT);
  outb(value, LFDD_CMOS_EXT_DATA_PORT);
}

int lfdd_cmos_read(struct lfdd_cmos* req) {
  unsigned long flags;

  spin_lock_irqsave(&rtc_lock, flags);
  req->value = lfdd_cmos_get(req->index);
  spin_unlock_irqrestore(&rtc_lock, flags);
  return 0;
}

int lfdd_cmos_write(const struct lfdd_cmos* req) {
  unsigned long flags;

  spin_lock_irqsave(&rtc_lock, flags);
  lfdd_cmos_set(req->index, req->value);
  spin_unlock_irqrestore(&rtc_lock, flags);
  return 0;
}

int lfdd_cmos_read_block(struct lfdd_cmos* req) {
  unsigned long flags;
  unsigned int i;

  spin_lock_irqsave(&rtc_lock, flags);
  for (i = 0; i < LFDD_CMOS_SIZE; i++) req->block[i] = lfdd_cmos_get(i);
  spin_unlock_irqrestore(&rtc_lock, flags);
  return 0;
}

#else  // !CONFIG_X86

int lfdd_cmos_read(struct lfdd_cmos* req) { return -EOPNOTSUPP; }
int lfdd_cmos_write(const struct lfdd_cmos* req) { return -EOPNOTSUPP; }
int lfdd_cmos_read_block(struct lfdd_cmos* req) { return -EOPNOTSUPP; }

#endif  // CONFIG_X86
