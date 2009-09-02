/*
 * LFDK - Linux Firmware Debug Kit
 * File: lfdk.h
 *
 * Copyright (C) 2006 - 2009 Merck Hung <merckhung@gmail.com>
 *										<merck_hung@asus.com>
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
#define LFDK_VERSION            "2.0.0pre"
#define LFDK_PROGNAME           "lfdk"
#define LFDK_VERTEXT            LFDK_PROGNAME" version "LFDK_VERSION", Linux Firmware Debug Kit"
#define LFDK_MAX_PCIBUF         50
#define LFDK_MAX_PATH           40
#define LFDK_MAX_PCIBUS			256
#define LFDK_MAX_READBUF        512
#define LFDK_DEFAULT_PCINAME    "/usr/share/misc/pci.ids"
#define LFDK_MAX_PCINAME        75
#define LFDK_BYTE_PER_LINE		16

#define LFDK_CMOS_RANGE_BYTES	256
#define LFDK_CMOS_IO_START		0x70
#define LFDK_CMOS_IO_END		0x72
#define LFDK_CMOS_ADDR_PORT		0x70
#define LFDK_CMOS_DATA_PORT		0x71
#define LFDK_MEM_DEV			"/dev/mem"


#define LFDD_IOCTL( FDESC, IOCTL_CMD, DATA ) {              \
                                                            \
    if( ioctl( FDESC, IOCTL_CMD, &DATA ) ) {                \
                                                            \
        endwin();                                           \
        fprintf( stderr, "Cannot execute command\n\n" );    \
        exit( 1 );                                          \
    }                                                       \
}


#define PrintWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {   \
                                                                                    \
    RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
    RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                   \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                       \
    wprintw( RESRC.NAME, FORMAT, ##ARGS );                                          \
    wattrset( RESRC.NAME, A_NORMAL );                                               \
}


#define PrintFixedWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {  \
                                                                                        \
    if( !RESRC.NAME ) {                                                                 \
                                                                                        \
        RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
        RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    }                                                                                   \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                       \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                           \
    mvwprintw( RESRC.NAME, 0, 0, FORMAT, ##ARGS );                                      \
    wattrset( RESRC.NAME, A_NORMAL );                                                   \
}


#define DestroyWin( RESRC, NAME ) {     \
                                        \
    if( RESRC.p_##NAME ) {              \
                                        \
        del_panel( RESRC.p_##NAME );    \
        RESRC.p_##NAME = NULL;          \
    }                                   \
                                        \
    if( RESRC.NAME ) {                  \
                                        \
        delwin( RESRC.NAME );           \
        RESRC.NAME = NULL;              \
    }                                   \
}


enum {

    PCI_DEVICE_FUNC = 1,
    PCI_LIST_FUNC,
    MEM_SPACE_FUNC,
    IO_SPACE_FUNC,
	CMOS_SPACE_FUNC,
	I2C_SPACE_FUNC,
};


enum {

    WHITE_RED = 1,
    WHITE_BLUE,
    BLACK_WHITE,
    CYAN_BLUE,
    RED_BLUE,
    YELLOW_BLUE,
    BLACK_GREEN,
    BLACK_YELLOW,
    YELLOW_RED,
    YELLOW_BLACK,
    WHITE_YELLOW
};


typedef struct {

    PANEL   *p_bg;
    PANEL   *p_logo;
    PANEL   *p_copyright;
    PANEL   *p_help;
    PANEL   *p_time;

    WINDOW  *bg;
    WINDOW  *logo;
    WINDOW  *copyright;
    WINDOW  *help;
    WINDOW  *time;

} BasePanel;


typedef struct {

    PANEL *p_ven;
    PANEL *p_dev;
    PANEL *p_offset;
    PANEL *p_info;
    PANEL *p_rtitle;
    PANEL *p_value;

    WINDOW *ven;
    WINDOW *dev;
    WINDOW *offset;
    WINDOW *info;
    WINDOW *rtitle;
    WINDOW *value;

} PCIPanel;


typedef struct {

    PANEL *p_title;
    PANEL *p_devname;
    PANEL *p_vendev;
    PANEL *p_scan;

    WINDOW *title;
    WINDOW *devname;
    WINDOW *vendev;
    WINDOW *scan;

} PCILPanel;


typedef struct {

    PANEL *p_offset;
    PANEL *p_info;
    PANEL *p_value;
    PANEL *p_ascii;

    WINDOW *offset;
    WINDOW *info;
    WINDOW *value;
    WINDOW *ascii;

} MemPanel;


typedef struct {

    unsigned short int      venid;
    unsigned short int      devid;
    unsigned char           bus;
    unsigned char           dev;
    unsigned char           fun;
    unsigned char           ventxt[ LFDK_MAX_PCINAME + 1 ];
    unsigned char           devtxt[ LFDK_MAX_PCINAME + 1 ];

} PCIData;


