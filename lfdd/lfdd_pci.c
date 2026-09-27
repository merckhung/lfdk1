// SPDX-License-Identifier: GPL-2.0
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * PCI / PCI Express configuration space access.
 *
 * Accesses go through the kernel's PCI core (pci_bus_{read,write}_config_*),
 * so they are serialized with the rest of the kernel and use whatever
 * mechanism the platform provides: legacy 0xCF8/0xCFC, ECAM/MMCONFIG for the
 * 4 KiB PCIe extended space, or a host bridge specific method.
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#include <linux/pci.h>

#include "lfdd.h"

static bool lfdd_pci_valid(const struct lfdd_pci* req, unsigned int len) {
  return req->dev < 32 && req->fn < 8 && req->reg % len == 0 &&
         req->reg + len <= LFDD_PCI_CFG_SPACE_SIZE;
}

// Returns the value of a configuration register, or all ones if the bus does
// not exist or the access fails. Must be called with the rescan/remove lock
// held.
static u32 lfdd_pci_cfg_read(const struct lfdd_pci* req, unsigned int reg,
                             unsigned int width) {
  struct pci_bus* bus = pci_find_bus(req->domain, req->bus);
  unsigned int devfn = PCI_DEVFN(req->dev, req->fn);
  u8 val8;
  u16 val16;
  u32 val32;

  if (!bus) return GENMASK(width * 8 - 1, 0);

  switch (width) {
    case 1:
      if (pci_bus_read_config_byte(bus, devfn, reg, &val8)) return 0xff;
      return val8;
    case 2:
      if (pci_bus_read_config_word(bus, devfn, reg, &val16)) return 0xffff;
      return val16;
    default:
      if (pci_bus_read_config_dword(bus, devfn, reg, &val32)) return ~0U;
      return val32;
  }
}

int lfdd_pci_read(struct lfdd_pci* req) {
  if (!lfdd_valid_width(req->width) || !lfdd_pci_valid(req, req->width))
    return -EINVAL;

  pci_lock_rescan_remove();
  req->value = lfdd_pci_cfg_read(req, req->reg, req->width);
  pci_unlock_rescan_remove();
  return 0;
}

int lfdd_pci_write(const struct lfdd_pci* req) {
  unsigned int devfn = PCI_DEVFN(req->dev, req->fn);
  struct pci_bus* bus;
  int ret;

  if (!lfdd_valid_width(req->width) || !lfdd_pci_valid(req, req->width))
    return -EINVAL;

  pci_lock_rescan_remove();
  bus = pci_find_bus(req->domain, req->bus);
  if (!bus) {
    ret = -ENODEV;
  } else {
    switch (req->width) {
      case 1:
        ret = pci_bus_write_config_byte(bus, devfn, req->reg, req->value);
        break;
      case 2:
        ret = pci_bus_write_config_word(bus, devfn, req->reg, req->value);
        break;
      default:
        ret = pci_bus_write_config_dword(bus, devfn, req->reg, req->value);
        break;
    }
    ret = pcibios_err_to_errno(ret);
  }
  pci_unlock_rescan_remove();
  return ret;
}

int lfdd_pci_read_block(struct lfdd_pci* req) {
  unsigned int i;

  if (req->reg % LFDD_BLOCK_SIZE || !lfdd_pci_valid(req, LFDD_BLOCK_SIZE))
    return -EINVAL;

  pci_lock_rescan_remove();
  for (i = 0; i < LFDD_BLOCK_SIZE; i += 4) {
    u32 val = lfdd_pci_cfg_read(req, req->reg + i, 4);

    // Configuration space is little endian.
    req->block[i] = val;
    req->block[i + 1] = val >> 8;
    req->block[i + 2] = val >> 16;
    req->block[i + 3] = val >> 24;
  }
  pci_unlock_rescan_remove();
  return 0;
}
