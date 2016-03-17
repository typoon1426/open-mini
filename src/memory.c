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

/* Channels and squelch code memory module */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "main.h"

#define GENERALBLOCK_ID 118
#define TAGBLOCK_ID 122
#define BITMAP_CBLOCK_IT 120
#define BITMAP_SCANBLOCK_IT 121

#define POWER_BUF_LEN 6
#define CODE_BUF_LEN 6
#define FREQ_BUF_LEN 11
#define FREQLEN 4

// 65536 string pointer, hashtable of digital code squelch
static char *dcsHashTable[0xFFFF];

// digital code squelch string array memory
static char dcsStrings[][6] = {
	 "D023N\0", "D025N\0", "D026N\0", "D031N\0", "D032N\0", "D043N\0", "D047N\0", "D051N\0", "D054N\0", "D065N\0", "D071N\0", "D072N\0", "D073N\0", "D074N\0", "D114N\0", 
	 "D115N\0", "D116N\0", "D125N\0", "D131N\0", "D132N\0", "D134N\0", "D143N\0", "D152N\0", "D155N\0", "D156N\0", "D162N\0", "D165N\0", "D172N\0", "D174N\0", "D205N\0", 
	 "D223N\0", "D226N\0", "D243N\0", "D244N\0", "D245N\0", "D251N\0", "D261N\0", "D263N\0", "D265N\0", "D271N\0", "D306N\0", "D311N\0", "D315N\0", "D331N\0", "D343N\0", 
	 "D346N\0", "D351N\0", "D364N\0", "D365N\0", "D371N\0", "D411N\0", "D412N\0", "D413N\0", "D423N\0", "D431N\0", "D432N\0", "D445N\0", "D464N\0", "D465N\0", "D466N\0", 
	 "D503N\0", "D506N\0", "D516N\0", "D532N\0", "D546N\0", "D565N\0", "D606N\0", "D612N\0", "D624N\0", "D627N\0", "D631N\0", "D632N\0", "D654N\0", "D662N\0", "D664N\0", 
	 "D703N\0", "D712N\0", "D723N\0", "D731N\0", "D732N\0", "D734N\0", "D743N\0", "D754N\0", "D023I\0", "D025I\0", "D026I\0", "D031I\0", "D032I\0", "D043I\0", "D047I\0", 
	 "D051I\0", "D054I\0", "D065I\0", "D071I\0", "D072I\0", "D073I\0", "D074I\0", "D114I\0", "D115I\0", "D116I\0", "D125I\0", "D131I\0", "D132I\0", "D134I\0", "D143I\0", 
	 "D152I\0", "D155I\0", "D156I\0", "D162I\0", "D165I\0", "D172I\0", "D174I\0", "D205I\0", "D223I\0", "D226I\0", "D243I\0", "D244I\0", "D245I\0", "D251I\0", "D261I\0", 
	 "D263I\0", "D265I\0", "D271I\0", "D306I\0", "D311I\0", "D315I\0", "D331I\0", "D343I\0", "D346I\0", "D351I\0", "D364I\0", "D365I\0", "D371I\0", "D411I\0", "D412I\0", 
	 "D413I\0", "D423I\0", "D431I\0", "D432I\0", "D445I\0", "D464I\0", "D465I\0", "D466I\0", "D503I\0", "D506I\0", "D516I\0", "D532I\0", "D546I\0", "D565I\0", "D606I\0", 
	 "D612I\0", "D624I\0", "D627I\0", "D631I\0", "D632I\0", "D654I\0", "D662I\0", "D664I\0", "D703I\0", "D712I\0", "D723I\0", "D731I\0", "D732I\0", "D734I\0", "D743I\0", 
	 "D754I\0"};

static const char lowPowerStr[] = "low\0";
static const char highPowerStr[] = "high\0";
static const char offString[] = "00000\0";

// count channels and return number
unsigned char countChannel(unsigned char *readBuffer, unsigned int len)
{
	unsigned int nBlocks = (len / 16), blocksCount = 0, bytesCount = 0;
	unsigned char retVal = 0;

	while((blocksCount < nBlocks))
	{
		for(bytesCount=0;bytesCount<16;bytesCount++)
		{
			if(*(readBuffer+(blocksCount*16)+bytesCount) != 0xff)
			{
				retVal += 1;
				break;	
			}
		}
		
		blocksCount++;
	}

	return retVal;
}

// write short int (2 byte word) with machine endianness to 2 byte little endian
void codeWrite(unsigned short code, unsigned char *codeRadio)
{
	unsigned char *codePtr = (unsigned char *) &code;

	if(code == 0)
		code = 0xffff;

	if(order == LITTLE)
	{
		codeRadio[0] = codePtr[0];
		codeRadio[1] = codePtr[1];				
	}
	else if(order == BIG)
	{
		codeRadio[0] = codePtr[1];
		codeRadio[1] = codePtr[0];
	}
}

// read 2 bytes little endian word to machine endianness 2 bytes word
unsigned short codeRead(unsigned char *codeRadio)
{
	unsigned short retVal = 0;
	unsigned char *retPtr = (unsigned char *) &retVal;

	if(order == LITTLE)
	{
		retPtr[0] = codeRadio[0];
		retPtr[1] = codeRadio[1];				
	}
	else if(order == BIG)
	{
		retPtr[0] = codeRadio[1];
		retPtr[1] = codeRadio[0];
	}

	return retVal;
}

// power
static unsigned long long int mathPower(unsigned int base, unsigned int exp)
{
	unsigned int i=0;
	unsigned long long int result = 1;
	
	if(exp == 0)
		return 1;
	else if(exp == 1)
		return base;
	else
	{
		for(i=0; i<exp; i++)
			result *= base;
	}

	return result;
}

// Read BCD frequency and write to unsigned int
unsigned int freqReadBCD(unsigned char *freqBCD)
{
	unsigned int frequency = 0;
	unsigned char selByte = 0;

	for(selByte=0; selByte<FREQLEN; selByte++)
	{
		if(freqBCD[selByte] != 0xFF)
		{
			unsigned char leastHalfByte = (freqBCD[selByte] & 0x0F);
			unsigned char mostHalfByte = ((freqBCD[selByte] & 0xF0) >> 4);
			unsigned char exp = (selByte*2);
		
			frequency += (leastHalfByte * mathPower(10, exp));
			frequency += (mostHalfByte * mathPower(10, (exp + 1))); 
		}
	}

	return frequency;
}

// Read unsigned int frequency and write to BCD
void freqWriteBCD(unsigned char *freqBCD, unsigned int freq)
{
	unsigned char sel, halfByte;

	for(sel=0; sel<4; sel++)
	{
		freqBCD[sel] = 0;

		for(halfByte=0; halfByte<2; halfByte++)
		{
			unsigned char digit = (freq % 10);
			freq /= 10;

			if(halfByte == 0)
				freqBCD[sel] = digit;
			else if(halfByte == 1)
				freqBCD[sel] += (digit << 4);
		}
	}
}

// Read unsigned int frequency and write to ascii format "433.000.00"
void printFreq(unsigned int freq, char *buf, unsigned char len)
{
	char sel = 0, index = 9;
	buf[len-1] = 0;

	for(sel=0; sel<8; sel++)
	{
		char asciiDigit[] = {0, 0};
		unsigned char digit = (freq % 10);
		freq /= 10;

		snprintf(asciiDigit, 2, "%hhu", digit);
	
		if((index == 3) || (index == 7))
		{
			buf[index] = '.';
			index--;
			buf[index] = asciiDigit[0];
			index--;
		}
		else
		{
			buf[index] = asciiDigit[0];
			index--;
		}
	}
}

// Read ascii frequency, "433.000.00" and write to unsigned int
unsigned int readFreq(char *freqString, unsigned char len)
{
	unsigned char index, digitIndex=7;
	unsigned int returnValue = 0;

	for(index=0; index<(len-1); index++)
	{
		char selectedChar[]  = {0, 0}; // second byte for \0
		strncpy(selectedChar, &freqString[index], 1);

		if((selectedChar[0] >= 0x30) && (selectedChar[0] <= 0x39))
		{
			unsigned char convertedChar = atoi(selectedChar);
			returnValue += (convertedChar * mathPower(10, digitIndex));
			digitIndex--;
		}
	}

	return returnValue;
}

// Read squelch code from unsigned short and write to ascii format "67.0" or "D023N" 
void printCode(unsigned short code, char *buf, unsigned char len)
{
	// DEBUG
	//printf("Codice squelch: %hx\n", code);

	if(code == 0xffff)
		snprintf(buf, len, "%s", offString);
	else if((code >= 670) && (code <= 2541))
	{
		// CTCSS
		unsigned char codeInt = (code / 10);
		unsigned char codeDecimal = (code % 10);

		int ret = snprintf(buf, len, "%03hhu", codeInt);
		snprintf((buf+ret), 2, "%s", ".");
		snprintf((buf+ret+1), 2, "%hhu", codeDecimal);
	}
	else
	{
		// DCS
		char *tmp = dcsHashTable[code];

		/*if(tmp == NULL)
		{
			printf("ATTENZIONE SQUELCH CODE NULLO\n");
		}
		else
		{*/
			snprintf(buf, len, "%s", tmp);
		//}
	}
}

// Read squelch code from ascii format "67.0" or "D023N" and write to unsigned short
unsigned short readCode(char *codeString, unsigned char len)
{
	unsigned short returnValue = 0;

	if((codeString[0] >= 0x30) && (codeString[0] <= 0x39))
	{

		unsigned char index = 0, digitCount=0, digits[4], digitIndex=0, ctcss = FALSE;

		do
		{
			if(codeString[index] != 0x30)
				ctcss = TRUE;
			index++;
		} while((!ctcss) && (index < 5));
		
		if(ctcss)
		{
			// CTCSS

			for(index=0; index<len; index++)
			{
				char selectedChar[]  = {0, 0}; // second byte for \0
				strncpy(selectedChar, &codeString[index], 1);
				
				if((selectedChar[0] >= 0x30) && (selectedChar[0] <= 0x39))
				{
					digits[digitCount] = atoi(selectedChar);
					digitCount++;
				}
			}
		
			char i;
			for(i=(digitCount-1); i>=0; i--)
			{
				returnValue += (digits[digitIndex] * mathPower(10, i));
				digitIndex++;
			}
		}
	}
	else
	{
		// DCS
		unsigned char stop = FALSE;

		if(codeString[4] == 0x4E) // ASCII VALUE N CHAR 0x4E
		{
			returnValue = 0x2813; //XXX
			
			do
			{
				char *strSel = dcsHashTable[returnValue];

				if((strSel != NULL) && (strncmp(strSel, codeString, 5) == 0))
					stop = TRUE;
				else
					returnValue++;
			} while((returnValue <= 0xA9EC) && !stop);

		}		
		else if(codeString[4] == 0x49) // ASCII VALUE I CHAR 0x49
		{
			returnValue = 0xA813;
			
			do
			{
				char *strSel = dcsHashTable[returnValue];

				if((strSel != NULL) && (strncmp(strSel, codeString, 5) == 0))
					stop = TRUE;
				else
					returnValue++;
			} while((returnValue <= 0xA9EC) && !stop);
		}	
	}

	return returnValue;
}

// Read RF power from radio memory's bitmapped format and write to ascii
static void printPower(unsigned char *power, char *buf, unsigned char len)
{
	unsigned char powerValue = power[1];

	if((powerValue & 0xF0) == 0xD0)
		strncpy(buf, lowPowerStr, len);
	else if((powerValue & 0xF0) == 0xF0)
		strncpy(buf, highPowerStr, len);
}

// Read RF power from ascii format and write to radio memory's bitmapped format
static void writePower(unsigned char *power, char *buf, unsigned char len)
{
	if(strncmp(buf, lowPowerStr, len) == 0)
		power[1] = 0xDF;
	else if(strncmp(buf, highPowerStr, len) == 0)
		power[1] = 0xFF;
}

// Read RF power from numerical code and write to memory's bitmapped format
void writePower2Bitmap(unsigned char *power, unsigned char rfPower)
{
	if(rfPower == RF_HIGH)
		power[1] = 0xFF;
	else if(rfPower == RF_LOW)
		power[1] = 0xDF;
}

// Read RF Power from memory's bitmapped format and return as numerical code
unsigned char readPowerBitmap(unsigned char *power)
{
	unsigned char powerValue = power[1], retVal = 0;

	if((powerValue & 0xF0) == 0xD0)
		retVal = RF_LOW;
	else if((powerValue & 0xF0) == 0xF0)
		retVal = RF_HIGH;
	
	return retVal;
}

// Read ascii POWER code and write numerical value 
unsigned char readAsciiPower(char *powerStr)
{
	unsigned char retVal;
	if(powerStr[0] == 0x48)
		retVal = RF_HIGH;
	else if(powerStr[0] == 0x4c)
		retVal = RF_LOW;

	return retVal;
}

// Count real used channels 
void countChannels(void)
{
	unsigned int index;

	for(index=1;index<=USABLE_CH_NUM; index++)
	{
		unsigned int rxFreq = freqReadBCD(radioMemory[index].rxFreq);
		unsigned int txFreq = freqReadBCD(radioMemory[index].txFreq);
	
		if((rxFreq != 0) && (txFreq != 0))
			channelSetNumber++;
	}
}

// Print memory of channels
void channelsMemoryPrint(unsigned char globalMode)
{
	unsigned int selCh=0;

	printf("The number of read channels is: %hhu.\n", channelSetNumber);
	
	printf("\nRead channels:\n\n");

	for(selCh=1; selCh<=USABLE_CH_NUM; selCh++)
	{
		unsigned int rxFreq = freqReadBCD(radioMemory[selCh].rxFreq);
		unsigned int txFreq = freqReadBCD(radioMemory[selCh].txFreq);
		
		if((rxFreq != 0) && (txFreq != 0))
		{
			unsigned short rxCode = codeRead(radioMemory[selCh].sqDecode);
			unsigned short txCode =  codeRead(radioMemory[selCh].sqEncode);
			char rxFreqString[FREQ_BUF_LEN], txFreqString[FREQ_BUF_LEN];
			char rxCodeString[CODE_BUF_LEN], txCodeString[CODE_BUF_LEN];
			char powerString[POWER_BUF_LEN];
		
			printFreq(rxFreq, rxFreqString, FREQ_BUF_LEN);
			printFreq(txFreq, txFreqString, FREQ_BUF_LEN);

			printCode(rxCode, rxCodeString, CODE_BUF_LEN);
			printCode(txCode, txCodeString, CODE_BUF_LEN);

			printPower(radioMemory[selCh].power, powerString, POWER_BUF_LEN);

			printf("Channel %03hhu : ", selCh);
			printf("RX Frequency: %s, ", rxFreqString);
			printf("TX Frequency: %s, ", txFreqString);
			printf("RX Squelch Code: %s, ", rxCodeString);
			printf("TX Squelch Code: %s, ", txCodeString);
			printf("RF Power: %s \n", powerString);
		}
	}
}

// Set channels bitmaps with channels number argument
void setChannelsBitmap(unsigned char channelsSetted)
{
	const unsigned char mask = 0x01;
	unsigned char nBlocks = (channelsSetted / 8);
	unsigned char notComplete = (channelsSetted % 8);

	if(nBlocks == 0)
	{
		unsigned char i, bitMap = 0x00;

		for(i=0;i<notComplete;i++)
			bitMap |= (mask << i);

		*((unsigned char *) &radioMemory[BITMAP_CBLOCK_IT]) = *((unsigned char *) &radioMemory[BITMAP_SCANBLOCK_IT]) = bitMap;
	}
	else
	{
		unsigned char i, bitMap = 0x00;

		for(i=0;i<nBlocks;i++)
			*(((unsigned char *) &radioMemory[BITMAP_CBLOCK_IT])+i) = *(((unsigned char *) &radioMemory[BITMAP_SCANBLOCK_IT])+i) = 0xff;

		if(notComplete != 0)
		{
			if(nBlocks == 13)
				bitMap = 0xe0;

			for(i=0;i<notComplete;i++)
				bitMap |= (mask << i);

			*(((unsigned char *) &radioMemory[BITMAP_CBLOCK_IT])+nBlocks) = *(((unsigned char *) &radioMemory[BITMAP_SCANBLOCK_IT])+nBlocks) = bitMap;
		}
	}
	
	radioMemory[BITMAP_CBLOCK_IT].rxFreq[13] = radioMemory[BITMAP_CBLOCK_IT].rxFreq[14] = radioMemory[BITMAP_CBLOCK_IT].rxFreq[15] = 0xff;
	radioMemory[BITMAP_SCANBLOCK_IT].rxFreq[13] = radioMemory[BITMAP_SCANBLOCK_IT].rxFreq[14] = radioMemory[BITMAP_SCANBLOCK_IT].rxFreq[15] = 0xff;
}

// Initialize memory
void initMemory(void)
{
	// Set all bytes of memory structure to 0xFF
	memset(radioMemory, 0xff, CHANNEL_NUM);
	
	unsigned char *generalBlock = (unsigned char *) &radioMemory[GENERALBLOCK_ID];

	// Set general setting block, address 0x0760 with general radio settings
	generalBlock[0] = generalBlock[1] = generalBlock[2] = 0xff;
	generalBlock[3] = 0x04;
	generalBlock[4] = generalBlock[5] = generalBlock[6] = generalBlock[7] = generalBlock[8] = generalBlock[9] = 0x02;
	generalBlock[10] = 0x00;
	generalBlock[11] = generalBlock[12] = generalBlock[13] = generalBlock[14] = generalBlock[15] = 0xff;
	
	unsigned char *tagBlock = (unsigned char *) &radioMemory[TAGBLOCK_ID];

	// Set channel block with 0x07a0 address with radio identity tag 
	tagBlock[0] = 0x4e;
	tagBlock[1] = 0x46;
	tagBlock[2] = 0x35;
	tagBlock[3] = 0x36;
	tagBlock[4] = 0x39;
	tagBlock[5] = 0x55;
	tagBlock[6] =  tagBlock[7] = tagBlock[8] = 
	tagBlock[9] = tagBlock[10] = tagBlock[11] = 
	tagBlock[12] = tagBlock[13] = tagBlock[14] = tagBlock[15] = 0xff;
	
	// Set dcsHashTable pointers
	dcsHashTable[0x2813] = &dcsStrings[0][0]; dcsHashTable[0x2815] = &dcsStrings[1][0]; dcsHashTable[0x2816] = &dcsStrings[2][0]; 
	dcsHashTable[0x2819] = &dcsStrings[3][0]; dcsHashTable[0x281A] = &dcsStrings[4][0]; dcsHashTable[0x2823] = &dcsStrings[5][0];
	dcsHashTable[0x2827] = &dcsStrings[6][0]; dcsHashTable[0x2829] = &dcsStrings[7][0]; dcsHashTable[0x282C] = &dcsStrings[8][0]; 
	dcsHashTable[0x2835] = &dcsStrings[9][0]; dcsHashTable[0x2839] = &dcsStrings[10][0]; dcsHashTable[0x283A] = &dcsStrings[11][0];
	dcsHashTable[0x283B] = &dcsStrings[12][0]; dcsHashTable[0x283C] = &dcsStrings[13][0]; dcsHashTable[0x284C] = &dcsStrings[14][0]; 
	dcsHashTable[0x284D] = &dcsStrings[15][0]; dcsHashTable[0x284E] = &dcsStrings[16][0]; dcsHashTable[0x2855] = &dcsStrings[17][0];
	dcsHashTable[0x2859] = &dcsStrings[18][0]; dcsHashTable[0x285A] = &dcsStrings[19][0]; dcsHashTable[0x285C] = &dcsStrings[20][0]; 
	dcsHashTable[0x2863] = &dcsStrings[21][0]; dcsHashTable[0x286A] = &dcsStrings[22][0]; dcsHashTable[0x286D] = &dcsStrings[23][0];
	dcsHashTable[0x286E] = &dcsStrings[24][0]; dcsHashTable[0x2872] = &dcsStrings[25][0]; dcsHashTable[0x2875] = &dcsStrings[26][0]; 
	dcsHashTable[0x287A] = &dcsStrings[27][0]; dcsHashTable[0x287C] = &dcsStrings[28][0]; dcsHashTable[0x2885] = &dcsStrings[29][0];
	dcsHashTable[0x2893] = &dcsStrings[30][0]; dcsHashTable[0x2896] = &dcsStrings[31][0]; dcsHashTable[0x28A3] = &dcsStrings[32][0]; 
	dcsHashTable[0x28A4] = &dcsStrings[33][0]; dcsHashTable[0x28A5] = &dcsStrings[34][0]; dcsHashTable[0x28A9] = &dcsStrings[35][0];
	dcsHashTable[0x28B1] = &dcsStrings[36][0]; dcsHashTable[0x28B3] = &dcsStrings[37][0]; dcsHashTable[0x28B5] = &dcsStrings[38][0]; 
	dcsHashTable[0x28B9] = &dcsStrings[39][0]; dcsHashTable[0x28C6] = &dcsStrings[40][0]; dcsHashTable[0x28C9] = &dcsStrings[41][0];
	dcsHashTable[0x28CD] = &dcsStrings[42][0]; dcsHashTable[0x28D9] = &dcsStrings[43][0]; dcsHashTable[0x28E3] = &dcsStrings[44][0]; 
	dcsHashTable[0x28E6] = &dcsStrings[45][0]; dcsHashTable[0x28E9] = &dcsStrings[46][0]; dcsHashTable[0x28F4] = &dcsStrings[47][0];
	dcsHashTable[0x28F5] = &dcsStrings[48][0]; dcsHashTable[0x28F9] = &dcsStrings[49][0]; dcsHashTable[0x2909] = &dcsStrings[50][0]; 
	dcsHashTable[0x290A] = &dcsStrings[51][0]; dcsHashTable[0x290B] = &dcsStrings[52][0]; dcsHashTable[0x2913] = &dcsStrings[53][0];
	dcsHashTable[0x2919] = &dcsStrings[54][0]; dcsHashTable[0x291A] = &dcsStrings[55][0]; dcsHashTable[0x2925] = &dcsStrings[56][0]; 
	dcsHashTable[0x2934] = &dcsStrings[57][0]; dcsHashTable[0x2935] = &dcsStrings[58][0]; dcsHashTable[0x2936] = &dcsStrings[59][0];
	dcsHashTable[0x2943] = &dcsStrings[60][0]; dcsHashTable[0x2946] = &dcsStrings[61][0]; dcsHashTable[0x294E] = &dcsStrings[62][0]; 
	dcsHashTable[0x295A] = &dcsStrings[63][0]; dcsHashTable[0x2966] = &dcsStrings[64][0]; dcsHashTable[0x2975] = &dcsStrings[65][0];
	dcsHashTable[0x2986] = &dcsStrings[66][0]; dcsHashTable[0x298A] = &dcsStrings[67][0]; dcsHashTable[0x2994] = &dcsStrings[68][0]; 
	dcsHashTable[0x2997] = &dcsStrings[69][0]; dcsHashTable[0x2999] = &dcsStrings[70][0]; dcsHashTable[0x299A] = &dcsStrings[71][0];
	dcsHashTable[0x29AC] = &dcsStrings[72][0]; dcsHashTable[0x29B2] = &dcsStrings[73][0]; dcsHashTable[0x29B4] = &dcsStrings[74][0]; 
	dcsHashTable[0x29C3] = &dcsStrings[75][0]; dcsHashTable[0x29CA] = &dcsStrings[76][0]; dcsHashTable[0x29D3] = &dcsStrings[77][0];
	dcsHashTable[0x29D9] = &dcsStrings[78][0]; dcsHashTable[0x29DA] = &dcsStrings[79][0]; dcsHashTable[0x29DC] = &dcsStrings[80][0]; 
	dcsHashTable[0x29E3] = &dcsStrings[81][0]; dcsHashTable[0x29EC] = &dcsStrings[82][0];

	dcsHashTable[0xA813] = &dcsStrings[0][0]; dcsHashTable[0xA815] = &dcsStrings[1][0]; dcsHashTable[0xA816] = &dcsStrings[2][0]; 
	dcsHashTable[0xA819] = &dcsStrings[3][0]; dcsHashTable[0xA81A] = &dcsStrings[4][0]; dcsHashTable[0xA823] = &dcsStrings[5][0];
	dcsHashTable[0xA827] = &dcsStrings[6][0]; dcsHashTable[0xA8A9] = &dcsStrings[7][0]; dcsHashTable[0xA82C] = &dcsStrings[8][0]; 
	dcsHashTable[0xA835] = &dcsStrings[9][0]; dcsHashTable[0xA839] = &dcsStrings[10][0]; dcsHashTable[0xA83A] = &dcsStrings[11][0];
	dcsHashTable[0xA83B] = &dcsStrings[12][0]; dcsHashTable[0xA83C] = &dcsStrings[13][0]; dcsHashTable[0xA84C] = &dcsStrings[14][0]; 
	dcsHashTable[0xA84D] = &dcsStrings[15][0]; dcsHashTable[0xA84E] = &dcsStrings[16][0]; dcsHashTable[0xA855] = &dcsStrings[17][0];
	dcsHashTable[0xA859] = &dcsStrings[18][0]; dcsHashTable[0xA85A] = &dcsStrings[19][0]; dcsHashTable[0xA85C] = &dcsStrings[20][0]; 
	dcsHashTable[0xA863] = &dcsStrings[21][0]; dcsHashTable[0xA86A] = &dcsStrings[22][0]; dcsHashTable[0xA86D] = &dcsStrings[23][0];
	dcsHashTable[0xA86E] = &dcsStrings[24][0]; dcsHashTable[0xA872] = &dcsStrings[25][0]; dcsHashTable[0xA875] = &dcsStrings[26][0]; 
	dcsHashTable[0xA87A] = &dcsStrings[27][0]; dcsHashTable[0xA87C] = &dcsStrings[28][0]; dcsHashTable[0xA885] = &dcsStrings[29][0];
	dcsHashTable[0xA893] = &dcsStrings[30][0]; dcsHashTable[0xA896] = &dcsStrings[31][0]; dcsHashTable[0xA8A3] = &dcsStrings[32][0]; 
	dcsHashTable[0xA8A4] = &dcsStrings[33][0]; dcsHashTable[0xA8A5] = &dcsStrings[34][0]; dcsHashTable[0xA8A9] = &dcsStrings[35][0];
	dcsHashTable[0xA8B1] = &dcsStrings[36][0]; dcsHashTable[0xA8B3] = &dcsStrings[37][0]; dcsHashTable[0xA8B5] = &dcsStrings[38][0]; 
	dcsHashTable[0xA8B9] = &dcsStrings[39][0]; dcsHashTable[0xA8C6] = &dcsStrings[40][0]; dcsHashTable[0xA8C9] = &dcsStrings[41][0];
	dcsHashTable[0xA8CD] = &dcsStrings[42][0]; dcsHashTable[0xA8D9] = &dcsStrings[43][0]; dcsHashTable[0xA8E3] = &dcsStrings[44][0]; 
	dcsHashTable[0xA8E6] = &dcsStrings[45][0]; dcsHashTable[0xA8E9] = &dcsStrings[46][0]; dcsHashTable[0xA8F4] = &dcsStrings[47][0];
	dcsHashTable[0xA8F5] = &dcsStrings[48][0]; dcsHashTable[0xA8F9] = &dcsStrings[49][0]; dcsHashTable[0xA909] = &dcsStrings[50][0]; 
	dcsHashTable[0xA90A] = &dcsStrings[51][0]; dcsHashTable[0xA90B] = &dcsStrings[52][0]; dcsHashTable[0xA913] = &dcsStrings[53][0];
	dcsHashTable[0xA919] = &dcsStrings[54][0]; dcsHashTable[0xA91A] = &dcsStrings[55][0]; dcsHashTable[0xA925] = &dcsStrings[56][0]; 
	dcsHashTable[0xA934] = &dcsStrings[57][0]; dcsHashTable[0xA935] = &dcsStrings[58][0]; dcsHashTable[0xA936] = &dcsStrings[59][0];
	dcsHashTable[0xA943] = &dcsStrings[60][0]; dcsHashTable[0xA946] = &dcsStrings[61][0]; dcsHashTable[0xA94E] = &dcsStrings[62][0]; 
	dcsHashTable[0xA95A] = &dcsStrings[63][0]; dcsHashTable[0xA966] = &dcsStrings[64][0]; dcsHashTable[0xA975] = &dcsStrings[65][0];
	dcsHashTable[0xA986] = &dcsStrings[66][0]; dcsHashTable[0xA98A] = &dcsStrings[67][0]; dcsHashTable[0xA994] = &dcsStrings[68][0]; 
	dcsHashTable[0xA997] = &dcsStrings[69][0]; dcsHashTable[0xA999] = &dcsStrings[70][0]; dcsHashTable[0xA99A] = &dcsStrings[71][0];
	dcsHashTable[0xA9AC] = &dcsStrings[72][0]; dcsHashTable[0xA9B2] = &dcsStrings[73][0]; dcsHashTable[0xA9B4] = &dcsStrings[74][0]; 
	dcsHashTable[0xA9C3] = &dcsStrings[75][0]; dcsHashTable[0xA9CA] = &dcsStrings[76][0]; dcsHashTable[0xA9D3] = &dcsStrings[77][0];
	dcsHashTable[0xA9D9] = &dcsStrings[78][0]; dcsHashTable[0xA9DA] = &dcsStrings[79][0]; dcsHashTable[0xA9DC] = &dcsStrings[80][0]; 
	dcsHashTable[0xA9E3] = &dcsStrings[81][0]; dcsHashTable[0xA9EC] = &dcsStrings[82][0];
}

// print memory debug function
void printMemory(void)
{
	unsigned int i, j;
	
	printf("\n");
	for(i=0; i<CHANNEL_NUM; i++)
	{
		printf("Channel n %03u RxFreq: ", i);
		
		for(j=0;j<4;j++)
			printf("0x%02hhx ", radioMemory[i].rxFreq[j]);

		printf(" TxFreq: ");
		
		for(j=0;j<4;j++)
			printf("0x%02hhx ", radioMemory[i].txFreq[j]);

		printf(" SqDecode: ");
		
		for(j=0;j<2;j++)
			printf("0x%02hhx ", radioMemory[i].sqDecode[j]);

		printf(" SqEncode: ");
		
		for(j=0;j<2;j++)
			printf("0x%02hhx ", radioMemory[i].sqEncode[j]);

		printf(" Power: ");
		
		for(j=0;j<2;j++)
			printf("0x%02hhx ", radioMemory[i].power[j]);

		printf(" Padding: ");
		
		for(j=0;j<2;j++)
			printf("%02hhx ", radioMemory[i].padding[j]);

		printf("\n");
	}

}
