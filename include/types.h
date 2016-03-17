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

/* Channel data structure definition */

#ifndef __TYPES_H__
#define __TYPES_H__ 1

typedef struct channel {
	// Frequency saved in BCD
	unsigned char rxFreq[4];
	unsigned char txFreq[4];

	// Squelch tone: Digital Code Squelch and CTCSS, Little endian 2 byte word
	unsigned char sqDecode[2];
	unsigned char sqEncode[2];

	// RF output power saved in the second byte of word, values: 0xFF == high power 2W (nominal), 0xDF == low power 500mW (nominal), first byte is always set to 0xFF
	unsigned char power[2];

	// 2 bytes of padding always set to 0xFF
	unsigned char padding[2];
} __attribute__ ((__packed__)) channel;
#endif
