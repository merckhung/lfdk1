/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * Internal interface between the ioctl dispatcher and the access backends.
 * Every function returns 0 on success or a negative errno.
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#ifndef LFDD_LFDD_H_
#define LFDD_LFDD_H_

#include "lfdd_ioctl.h"

static inline bool lfdd_valid_width(unsigned int width) {
  return width == 1 || width == 2 || width == 4;
}

int lfdd_pci_read(struct lfdd_pci* req);
int lfdd_pci_write(const struct lfdd_pci* req);
int lfdd_pci_read_block(struct lfdd_pci* req);

int lfdd_mem_read(struct lfdd_mem* req);
int lfdd_mem_write(const struct lfdd_mem* req);
int lfdd_mem_read_block(struct lfdd_mem* req);

int lfdd_io_read(struct lfdd_io* req);
int lfdd_io_write(const struct lfdd_io* req);
int lfdd_io_read_block(struct lfdd_io* req);

int lfdd_cmos_read(struct lfdd_cmos* req);
int lfdd_cmos_write(const struct lfdd_cmos* req);
int lfdd_cmos_read_block(struct lfdd_cmos* req);

#endif  // LFDD_LFDD_H_
