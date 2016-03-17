/*   Open-Mini Version 0.1
 *   
 *   Copyright 2011 Michele Cucchi <cucchi@cs.unibo.it>
 *   
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License,as
 *   published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */

/* Main module header file */

#ifndef __MAIN_H__
#define __MAIN_H__ 1

#define TRUE 1
#define FALSE 0

#define LITTLE 0
#define BIG 1

#define BUFLEN 100
#define READ 0
#define WRITE 1

// machine word endianness  0 little endian, 1 big endian
unsigned char order;

void closeAll(void);
void setMode(unsigned char newMode);
unsigned char getMode(void);
unsigned char orderTest(void);
#endif
