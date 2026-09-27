/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * User-space interface of the driver. This header is shared between the
 * kernel module (lfdd) and the user-space tool (lfdk).
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#ifndef LFDD_LFDD_IOCTL_H_
#define LFDD_LFDD_IOCTL_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define LFDD_VERSION "3.0.0"
#define LFDD_DEVICE_PATH "/dev/lfdd"

// Number of bytes transferred by the *_READ_BLOCK commands.
#define LFDD_BLOCK_SIZE 256

// Size of the PCI Express extended configuration space of a function.
#define LFDD_PCI_CFG_SPACE_SIZE 4096

// Size of the x86 I/O port space.
#define LFDD_IO_SPACE_SIZE 0x10000

// Standard (0x00-0x7f) plus extended (0x80-0xff) CMOS/NVRAM bank.
#define LFDD_CMOS_SIZE 256

// PCI / PCI Express configuration space access.
//
// |width| (1, 2 or 4) selects the access size of LFDD_PCI_READ and
// LFDD_PCI_WRITE, and |reg| must be naturally aligned to it.
// LFDD_PCI_READ_BLOCK reads LFDD_BLOCK_SIZE bytes starting at |reg|, which
// must be a multiple of LFDD_BLOCK_SIZE. Registers of functions that do not
// exist, or that are behind a bus the kernel has not enumerated, read back
// as all ones.
struct lfdd_pci {
  __u16 domain;
  __u8 bus;
  __u8 dev;
  __u8 fn;
  __u8 width;
  __u16 reg;
  __u32 value;
  __u32 reserved;
  __u8 block[LFDD_BLOCK_SIZE];
};

// Physical memory access. |addr| is a physical address. System RAM and
// MMIO regions are both supported; unmappable addresses read as all ones.
struct lfdd_mem {
  __u64 addr;
  __u32 value;
  __u8 width;
  __u8 reserved[3];
  __u8 block[LFDD_BLOCK_SIZE];
};

// I/O port access. Ports past the end of the I/O space read as all ones.
struct lfdd_io {
  __u16 port;
  __u8 width;
  __u8 reserved;
  __u32 value;
  __u8 block[LFDD_BLOCK_SIZE];
};

// CMOS / NVRAM access. LFDD_CMOS_READ_BLOCK always returns the whole
// LFDD_CMOS_SIZE bytes and ignores |index|.
struct lfdd_cmos {
  __u8 index;
  __u8 reserved[3];
  __u32 value;
  __u8 block[LFDD_CMOS_SIZE];
};

#define LFDD_IOC_MAGIC 'L'

#define LFDD_PCI_READ _IOWR(LFDD_IOC_MAGIC, 0x40, struct lfdd_pci)
#define LFDD_PCI_WRITE _IOW(LFDD_IOC_MAGIC, 0x41, struct lfdd_pci)
#define LFDD_PCI_READ_BLOCK _IOWR(LFDD_IOC_MAGIC, 0x42, struct lfdd_pci)

#define LFDD_MEM_READ _IOWR(LFDD_IOC_MAGIC, 0x43, struct lfdd_mem)
#define LFDD_MEM_WRITE _IOW(LFDD_IOC_MAGIC, 0x44, struct lfdd_mem)
#define LFDD_MEM_READ_BLOCK _IOWR(LFDD_IOC_MAGIC, 0x45, struct lfdd_mem)

#define LFDD_IO_READ _IOWR(LFDD_IOC_MAGIC, 0x46, struct lfdd_io)
#define LFDD_IO_WRITE _IOW(LFDD_IOC_MAGIC, 0x47, struct lfdd_io)
#define LFDD_IO_READ_BLOCK _IOWR(LFDD_IOC_MAGIC, 0x48, struct lfdd_io)

#define LFDD_CMOS_READ _IOWR(LFDD_IOC_MAGIC, 0x49, struct lfdd_cmos)
#define LFDD_CMOS_WRITE _IOW(LFDD_IOC_MAGIC, 0x4a, struct lfdd_cmos)
#define LFDD_CMOS_READ_BLOCK _IOWR(LFDD_IOC_MAGIC, 0x4b, struct lfdd_cmos)

#endif  // LFDD_LFDD_IOCTL_H_
