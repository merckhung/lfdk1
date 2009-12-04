/*
 * LFDK - Linux Firmware Debug Kit
 * File: libcmos.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/io.h>

#include <ncurses.h>
#include <panel.h>

#include "../lfdd/lfdd.h"
#include "lfdk.h"


MemPanel CmosScreen;
struct lfdd_io_t lfdd_io_data;


extern int x, y;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;


unsigned int cmos_addr = 0;


void WriteCmosByteValue() {

	outb( x * LFDK_BYTE_PER_LINE + y, LFDK_CMOS_ADDR_PORT );
	outb( wbuf, LFDK_CMOS_DATA_PORT );
}


void ReadCmos256Bytes( char *buf ) {

	int i;

	for( i = 0 ; i < LFDK_CMOS_RANGE_BYTES ; i++ ) {

		outb( i, LFDK_CMOS_ADDR_PORT );
		buf[ i ] = (char)inb( LFDK_CMOS_DATA_PORT );
	}
}


void ClearCmosScreen() {

    DestroyWin( CmosScreen, offset );
    DestroyWin( CmosScreen, info );
    DestroyWin( CmosScreen, value );
    DestroyWin( CmosScreen, ascii );
}


void PrintCmosScreen() {

    int i, j;
    char tmp;


        if( ibuf == KEY_UP ) {

            if( x > 0 ) {

                x--;
            }

            input = 0;
        }
        else if( ibuf == KEY_DOWN ) {

            if( x < 15 ) {

                x++;
            }

            input = 0;
        }
        else if( ibuf == KEY_LEFT ) {

            if( y > 0 ) {

                y--;
            }

            input = 0;
        }
        else if( ibuf == KEY_RIGHT ) {

            if( y < 15 ) {

                y++;
            }

            input = 0;
        }
        else if( ibuf == KEY_NPAGE ) {

            if( (0xffffffff - cmos_addr) >= LFDD_MASSBUF_SIZE ) {
        
                cmos_addr += LFDD_MASSBUF_SIZE;
            }
            else {
        
                cmos_addr = 0;
            }

            input = 0;
        }
        else if( ibuf == KEY_PPAGE ) {

            if( cmos_addr >= LFDD_MASSBUF_SIZE ) {
        
                cmos_addr -= LFDD_MASSBUF_SIZE;
            }
            else {
        
                cmos_addr = (0xffffffff - LFDD_MASSBUF_SIZE + 1);
            }

            input = 0;
        }
        else if( ibuf == 0x0a ) {

            if( input ) {

                input = 0;
				WriteCmosByteValue();
            }
        }
        else if ( ((ibuf >= '0') && (ibuf <= '9'))
                || ((ibuf >= 'a') && (ibuf <= 'f'))
                || ((ibuf >= 'A') && (ibuf <= 'F')) ) {

            if( !input ) {

                wbuf = 0x00;
                input = 1;
            }


            wbuf <<= 4;
            wbuf &= 0xf0;


            if( ibuf <= '9' ) {

                wbuf |= ibuf - 0x30;
            }
            else if( ibuf > 'F' ) {

                wbuf |= ibuf - 0x60 + 9;
            }
            else {

                wbuf |= ibuf - 0x40 + 9;
            }
        }


    //
    // Print Offset Text
    //
    PrintFixedWin( CmosScreen, offset, 17, 52, 4, 1, RED_BLUE, "0000 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0" );


    //
    // Print memory address
    //
    if( !CmosScreen.info ) {

        CmosScreen.info = newwin( 1, 47, 22, 0 );
        CmosScreen.p_info = new_panel( CmosScreen.info );
    }
    wbkgd( CmosScreen.info, COLOR_PAIR( WHITE_BLUE ) );
    wattrset( CmosScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
    mvwprintw( CmosScreen.info, 0, 0, "Type: CMOS" );


    //
    // Read memory space 256 bytes
    //
	ReadCmos256Bytes( lfdd_io_data.mass_buf );


    //
    // Print ASCII content
    //
    if( !CmosScreen.ascii ) {

        CmosScreen.ascii = newwin( 17, 16, 4, 58 );
        CmosScreen.p_ascii = new_panel( CmosScreen.ascii );
    }


    wbkgd( CmosScreen.ascii, COLOR_PAIR( CYAN_BLUE ) );
    wattrset( CmosScreen.ascii, COLOR_PAIR( CYAN_BLUE ) | A_BOLD );
    mvwprintw( CmosScreen.ascii, 0, 0, "" );

    wprintw( CmosScreen.ascii, "0123456789ABCDEF" );
    for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {

        for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {

            tmp = ((unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ]);
            if( (tmp >= '!') && (tmp <= '~') ) {
            
                wprintw( CmosScreen.ascii, "%c", tmp );
            }
            else {

                wprintw( CmosScreen.ascii, "." ); 
            }
        }
    }

    wattrset( CmosScreen.ascii, A_NORMAL );


    //
    // Print 256bytes content
    //
    if( !CmosScreen.value ) {

        CmosScreen.value = newwin( 17, 47, 5, 6 );
        CmosScreen.p_value = new_panel( CmosScreen.value );
    }


    wbkgd( CmosScreen.value, COLOR_PAIR( WHITE_BLUE ) );
    mvwprintw( CmosScreen.value, 0, 0, "" );


    for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {

        for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {
    
                
            //
            // Change Color Pair
            //
            if( y == j && x == i ) {
              
                if( input ) {
                
                    if( counter % 2 ) {
                    
                        wattrset( CmosScreen.value, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                    }
                    else {
                    
                        wattrset( CmosScreen.value, COLOR_PAIR( YELLOW_BLACK ) | A_BOLD );
                    }
                    
                    counter++;
                }
                else {

                    wattrset( CmosScreen.value, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD ); 
                }
            }
            else if( ((unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ]) ) {
           
                wattrset( CmosScreen.value, COLOR_PAIR( YELLOW_BLUE ) | A_BOLD );            
            }
            else {
            
                wattrset( CmosScreen.value, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
            }


            //
            // Handle input display
            //
            if( y == j && x == i ) {


                if( input ) {
                
                    wprintw( CmosScreen.value, "%2.2X", (unsigned char)wbuf );
                }
                else {
                
                    wprintw( CmosScreen.value, "%2.2X", (unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ] );
                }
            }
            else {

                wprintw( CmosScreen.value, "%2.2X", (unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ] );
            }


            //
            // End of color pair
            //
            wattrset( CmosScreen.value, A_NORMAL );


            //
            // Move to next byte
            //
            if( j != 15 ) {
          
                wprintw( CmosScreen.value, " " );
            }
        }
    }
}


