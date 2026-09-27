// SPDX-License-Identifier: GPL-2.0
/*
 * LFDD - Linux Firmware Debug Driver
 *
 * Registers the /dev/lfdd misc device and dispatches its ioctls to the PCI,
 * memory, I/O port and CMOS backends.
 *
 * Copyright (C) 2006 - 2026 Merck Hung <merckhung@gmail.com>
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/security.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "lfdd.h"

union lfdd_request {
  struct lfdd_pci pci;
  struct lfdd_mem mem;
  struct lfdd_io io;
  struct lfdd_cmos cmos;
};

static int lfdd_open(struct inode* inode, struct file* file) {
  // Same policy as /dev/mem: raw hardware access needs CAP_SYS_RAWIO and is
  // refused while the kernel is locked down.
  if (!capable(CAP_SYS_RAWIO)) return -EPERM;
  return security_locked_down(LOCKDOWN_DEV_MEM);
}

static long lfdd_dispatch(unsigned int cmd, union lfdd_request* req) {
  switch (cmd) {
    case LFDD_PCI_READ:
      return lfdd_pci_read(&req->pci);
    case LFDD_PCI_WRITE:
      return lfdd_pci_write(&req->pci);
    case LFDD_PCI_READ_BLOCK:
      return lfdd_pci_read_block(&req->pci);
    case LFDD_MEM_READ:
      return lfdd_mem_read(&req->mem);
    case LFDD_MEM_WRITE:
      return lfdd_mem_write(&req->mem);
    case LFDD_MEM_READ_BLOCK:
      return lfdd_mem_read_block(&req->mem);
    case LFDD_IO_READ:
      return lfdd_io_read(&req->io);
    case LFDD_IO_WRITE:
      return lfdd_io_write(&req->io);
    case LFDD_IO_READ_BLOCK:
      return lfdd_io_read_block(&req->io);
    case LFDD_CMOS_READ:
      return lfdd_cmos_read(&req->cmos);
    case LFDD_CMOS_WRITE:
      return lfdd_cmos_write(&req->cmos);
    case LFDD_CMOS_READ_BLOCK:
      return lfdd_cmos_read_block(&req->cmos);
    default:
      return -ENOTTY;
  }
}

static long lfdd_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
  void __user* uarg = (void __user*)arg;
  size_t size = _IOC_SIZE(cmd);
  union lfdd_request req;
  long ret;

  if (_IOC_TYPE(cmd) != LFDD_IOC_MAGIC || size > sizeof(req)) return -ENOTTY;

  memset(&req, 0, sizeof(req));
  if (copy_from_user(&req, uarg, size)) return -EFAULT;

  ret = lfdd_dispatch(cmd, &req);
  if (ret) return ret;

  if ((_IOC_DIR(cmd) & _IOC_READ) && copy_to_user(uarg, &req, size))
    return -EFAULT;

  return 0;
}

static const struct file_operations lfdd_fops = {
    .owner = THIS_MODULE,
    .open = lfdd_open,
    .unlocked_ioctl = lfdd_ioctl,
    .compat_ioctl = compat_ptr_ioctl,
    .llseek = noop_llseek,
};

static struct miscdevice lfdd_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "lfdd",
    .fops = &lfdd_fops,
    .mode = 0600,
};

static int __init lfdd_init(void) {
  int ret = misc_register(&lfdd_dev);

  if (ret) {
    pr_err("failed to register misc device: %d\n", ret);
    return ret;
  }

  pr_info("Linux Firmware Debug Driver %s loaded\n", LFDD_VERSION);
  return 0;
}

static void __exit lfdd_exit(void) {
  misc_deregister(&lfdd_dev);
  pr_info("unloaded\n");
}

module_init(lfdd_init);
module_exit(lfdd_exit);

MODULE_AUTHOR("Merck Hung <merckhung@gmail.com>");
MODULE_DESCRIPTION("Linux Firmware Debug Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(LFDD_VERSION);
