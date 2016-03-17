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

/* Radio I/O protocol module header file */

#ifndef __RADIO_H__
#define __RADIO_H__ 1

#define READ_OFFSET 0x40
#define WRITE_OFFSET 0x10 
#define ACK 0x06
#define STX 0x02
#define TERMC 0x45
#define READC 0x52 // Ascii R character
#define WRITEC 0x57 // Ascii W character
#define MEM_BYTES_MAX 2048

#define READ_BUF_LEN 68
#define WRITE_BUF_LEN 20

void radioRead(void);
void radioWrite(void);
#endif
