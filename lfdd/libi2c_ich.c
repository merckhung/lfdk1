/*
 * LFDD - Linux Firmware Debug Driver
 * File: libi2c_ich.c
 *
 * Copyright (C) 2006 - 2010 Merck Hung <merckhung@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>

#include <linux/delay.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include "lfdd.h"


extern spinlock_t lfdd_lock;


unsigned char lfdd_i2c_read_byte( unsigned int addr ) {

    return 0;
}


unsigned short int lfdd_i2c_read_word( unsigned int addr ) {

	return 0;
}


unsigned int lfdd_i2c_read_dword( unsigned int addr ) {

    return 0;
}


void lfdd_i2c_write_byte( unsigned int value, unsigned int addr ) {

}


void lfdd_i2c_write_word( unsigned int value, unsigned int addr ) {

}


void lfdd_i2c_write_dword( unsigned int value, unsigned int addr ) {

}


void lfdd_i2c_read_256byte( struct lfdd_i2c_t *pi2c ) { 

}


