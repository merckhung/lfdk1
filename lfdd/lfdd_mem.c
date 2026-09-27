// SPDX-License-Identifier: GPL-2.0
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * Physical memory access.
 *
 * Requests are split into chunks that never cross a page boundary. Each
 * chunk is classified with region_intersects():
 *
 *  - System RAM is read through the kernel direct map with
 *    copy_from_kernel_nofault(), so pages that are not mapped (for example
 *    secretmem) read as all ones instead of oopsing. Writes go through a
 *    temporary vmap() alias, which is writable even when the direct map of
 *    the page is read-only (kernel text and rodata).
 *  - Everything else (MMIO, firmware tables, reserved ranges) is mapped
 *    uncached with ioremap().
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <linux/mm.h>
#include <linux/pfn.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_X86
#include <asm/processor.h>
#endif

#include "lfdd.h"

static bool lfdd_mem_valid(u64 addr, size_t len) {
  return addr <= PHYS_ADDR_MAX && len - 1 <= PHYS_ADDR_MAX - addr;
}

// Whether the range is within the physical address width of the CPU;
// ioremap() warns about anything beyond it.
static bool lfdd_mem_addressable(phys_addr_t addr, size_t len) {
#ifdef CONFIG_X86
  return !((addr + len - 1) >> boot_cpu_data.x86_phys_bits);
#else
  return true;
#endif
}

static bool lfdd_mem_is_ram(phys_addr_t addr, size_t len) {
  return region_intersects(addr, len, IORESOURCE_SYSTEM_RAM, IORES_DESC_NONE) ==
         REGION_INTERSECTS;
}

static bool lfdd_mem_is_mmio(phys_addr_t addr, size_t len) {
  return lfdd_mem_addressable(addr, len) &&
         region_intersects(addr, len, IORESOURCE_SYSTEM_RAM, IORES_DESC_NONE) ==
             REGION_DISJOINT;
}

// Reads |len| bytes that do not cross a page boundary. |width| is the MMIO
// access size, or 0 to let memcpy_fromio() choose.
static int lfdd_mem_read_chunk(void* dst, phys_addr_t addr, size_t len,
                               unsigned int width) {
  void __iomem* mmio;
  void* va;
  int ret;

  if (lfdd_mem_is_ram(addr, len)) {
    va = memremap(addr, len, MEMREMAP_WB);
    if (!va) return -ENXIO;
    ret = copy_from_kernel_nofault(dst, va, len);
    memunmap(va);
    return ret;
  }

  if (!lfdd_mem_is_mmio(addr, len)) return -ENXIO;

  mmio = ioremap(addr, len);
  if (!mmio) return -ENXIO;

  switch (width) {
    case 1:
      *(u8*)dst = readb(mmio);
      break;
    case 2:
      *(u16*)dst = readw(mmio);
      break;
    case 4:
      *(u32*)dst = readl(mmio);
      break;
    default:
      memcpy_fromio(dst, mmio, len);
      break;
  }
  iounmap(mmio);
  return 0;
}

int lfdd_mem_read(struct lfdd_mem* req) {
  union {
    u8 b;
    u16 w;
    u32 l;
  } val;

  if (!lfdd_valid_width(req->width) || req->addr % req->width ||
      !lfdd_mem_valid(req->addr, req->width))
    return -EINVAL;

  if (lfdd_mem_read_chunk(&val, req->addr, req->width, req->width)) {
    req->value = GENMASK(req->width * 8 - 1, 0);
    return 0;
  }

  switch (req->width) {
    case 1:
      req->value = val.b;
      break;
    case 2:
      req->value = val.w;
      break;
    default:
      req->value = val.l;
      break;
  }
  return 0;
}

int lfdd_mem_read_block(struct lfdd_mem* req) {
  phys_addr_t addr = req->addr;
  size_t done = 0;

  if (!lfdd_mem_valid(req->addr, LFDD_BLOCK_SIZE)) return -EINVAL;

  while (done < LFDD_BLOCK_SIZE) {
    size_t len =
        min_t(size_t, LFDD_BLOCK_SIZE - done, PAGE_SIZE - offset_in_page(addr));

    if (lfdd_mem_read_chunk(req->block + done, addr, len, 0))
      memset(req->block + done, 0xff, len);

    done += len;
    addr += len;
  }
  return 0;
}

static int lfdd_mem_write_ram(phys_addr_t addr, unsigned int width, u32 value) {
  unsigned long pfn = PHYS_PFN(addr);
  struct page* page;
  void* base;
  void* va;

  if (!pfn_valid(pfn)) return -ENXIO;

  page = pfn_to_page(pfn);
  base = vmap(&page, 1, VM_MAP, PAGE_KERNEL);
  if (!base) return -ENOMEM;

  va = base + offset_in_page(addr);
  switch (width) {
    case 1:
      WRITE_ONCE(*(u8*)va, value);
      break;
    case 2:
      WRITE_ONCE(*(u16*)va, value);
      break;
    default:
      WRITE_ONCE(*(u32*)va, value);
      break;
  }
  vunmap(base);
  return 0;
}

static int lfdd_mem_write_mmio(phys_addr_t addr, unsigned int width,
                               u32 value) {
  void __iomem* mmio = ioremap(addr, width);

  if (!mmio) return -ENXIO;

  switch (width) {
    case 1:
      writeb(value, mmio);
      break;
    case 2:
      writew(value, mmio);
      break;
    default:
      writel(value, mmio);
      break;
  }
  iounmap(mmio);
  return 0;
}

int lfdd_mem_write(const struct lfdd_mem* req) {
  if (!lfdd_valid_width(req->width) || req->addr % req->width ||
      !lfdd_mem_valid(req->addr, req->width))
    return -EINVAL;

  if (lfdd_mem_is_ram(req->addr, req->width))
    return lfdd_mem_write_ram(req->addr, req->width, req->value);
  if (lfdd_mem_is_mmio(req->addr, req->width))
    return lfdd_mem_write_mmio(req->addr, req->width, req->value);
  return -ENXIO;
}
