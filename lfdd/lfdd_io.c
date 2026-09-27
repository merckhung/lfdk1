// SPDX-License-Identifier: GPL-2.0
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * I/O port access.
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#include <linux/io.h>
#include <linux/string.h>

#include "lfdd.h"

#ifdef CONFIG_HAS_IOPORT

static bool lfdd_io_valid(unsigned int port, unsigned int len) {
  return port + len <= LFDD_IO_SPACE_SIZE;
}

int lfdd_io_read(struct lfdd_io* req) {
  if (!lfdd_valid_width(req->width) || !lfdd_io_valid(req->port, req->width))
    return -EINVAL;

  switch (req->width) {
    case 1:
      req->value = inb(req->port);
      break;
    case 2:
      req->value = inw(req->port);
      break;
    default:
      req->value = inl(req->port);
      break;
  }
  return 0;
}

int lfdd_io_write(const struct lfdd_io* req) {
  if (!lfdd_valid_width(req->width) || !lfdd_io_valid(req->port, req->width))
    return -EINVAL;

  switch (req->width) {
    case 1:
      outb(req->value, req->port);
      break;
    case 2:
      outw(req->value, req->port);
      break;
    default:
      outl(req->value, req->port);
      break;
  }
  return 0;
}

int lfdd_io_read_block(struct lfdd_io* req) {
  unsigned int i;

  memset(req->block, 0xff, sizeof(req->block));
  for (i = 0; i < LFDD_BLOCK_SIZE && lfdd_io_valid(req->port + i, 1); i++)
    req->block[i] = inb(req->port + i);
  return 0;
}

#else  // !CONFIG_HAS_IOPORT

int lfdd_io_read(struct lfdd_io* req) { return -EOPNOTSUPP; }
int lfdd_io_write(const struct lfdd_io* req) { return -EOPNOTSUPP; }
int lfdd_io_read_block(struct lfdd_io* req) { return -EOPNOTSUPP; }

#endif  // CONFIG_HAS_IOPORT
