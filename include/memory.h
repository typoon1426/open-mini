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

/* Channels and squelch code memory module header file */

#ifndef __MEMORY_H__
#define __MEMORY_H__ 1
#include "types.h"

#define RX 0
#define TX 1
#define RF_HIGH 1
#define RF_LOW 0
#define CHANNEL_NUM 128
#define USABLE_CH_NUM 101

channel radioMemory[CHANNEL_NUM];
unsigned char channelSetNumber;

void initMemory(void);
void channelsMemoryPrint(unsigned char globalMode);
unsigned int freqReadBCD(unsigned char *freqBCD);
void freqWriteBCD(unsigned char *freqBCD, unsigned int freq);
void printFreq(unsigned int freq, char *buf, unsigned char len);
unsigned int readFreq(char *freqString, unsigned char len);
void printCode(unsigned short code, char *buf, unsigned char len);
unsigned short readCode(char *codeString, unsigned char len);
unsigned char readAsciiPower(char *powerStr);
void writePower2Bitmap(unsigned char *power, unsigned char rfPower);
unsigned char readPowerBitmap(unsigned char *power);
void setChannelsBitmap(unsigned char channelsSetted);
void codeWrite(unsigned short code, unsigned char *codeRadio);
unsigned short codeRead(unsigned char *codeRadio);
unsigned char countChannel(unsigned char *readBuffer, unsigned int len);
void printMemory(void);
void countChannels(void);
#endif
