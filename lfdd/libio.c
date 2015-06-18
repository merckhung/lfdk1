/*
 * LFDD - Linux Firmware Debug Driver
 * File: libio.c
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

#include <linux/highmem.h>

#include "lfdd.h"


extern spinlock_t lfdd_lock;


unsigned char lfdd_io_read_byte( unsigned int addr ) {

    unsigned long flags;
    unsigned char value;

    spin_lock_irqsave( &lfdd_lock, flags );

    value = inb( addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return value;
}


unsigned short lfdd_io_read_word( unsigned int addr ) {

    unsigned long flags;
    unsigned short value;

    spin_lock_irqsave( &lfdd_lock, flags );

    value = inw( addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return value;
}


unsigned int lfdd_io_read_dword( unsigned int addr ) {

    unsigned long flags;
    unsigned int value;

    spin_lock_irqsave( &lfdd_lock, flags );

    value = inl( addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return value;
}


void lfdd_io_write_byte( unsigned char value, unsigned int addr ) {

    unsigned long flags;

    spin_lock_irqsave( &lfdd_lock, flags );

    outb( value, addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_io_write_word( unsigned short value, unsigned int addr ) {

    unsigned long flags;

    spin_lock_irqsave( &lfdd_lock, flags );

    outw( value, addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_io_write_dword( unsigned int value, unsigned int addr ) {

    unsigned long flags;

    spin_lock_irqsave( &lfdd_lock, flags );

    outl( value, addr );

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_io_read_256byte( struct lfdd_io_t *pio ) { 

    int i;
    unsigned long flags;

    spin_lock_irqsave( &lfdd_lock, flags );

    for( i = 0 ; i < LFDD_MASSBUF_SIZE ; i++ ) {
    
        pio->mass_buf[ i ] = inb( pio->addr + i );
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


