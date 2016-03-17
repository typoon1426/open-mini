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

/* File I/O module */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "fileIO.h"
#include "error.h"
#include "main.h"
#include "memory.h"

/* Example of data file :
Global channel counter   
Channel number,RX Frequency,TX Frequency,RX Code,TX Code,RF output power, LineFeed character


   002\n
   001,433.000.00,433.000.00,D024N,D024N,H\n 
   002,433.000.00,433.000.00,067.0,067.0,H\n
*/

#define FILE_PATH_BUF_LEN 32
#define LINE_LEN 40

static const char incorrectData[] = "Data file not correctly formatted.";
static const char channelsNumberWrong[] = "Global channels counter is out of allowed range: allowed range is between 1 and 101.";
static const char channelNumberWrong[] = "Incorrect channel index: can be between 1 and 101.";
static const char frequencyOutOfRange[] = "One frequency is out of range: allowed range is between 420.000.00-450.000.00 Mhz.";
static const char incorrectCode[] = "Incorrect code squelch.";
static const char powerCharWrong[] = "Incorrect rf power configuration character: can be H or L.";
static const char emptyFile[] = "Errore the file is empty.";
static const char errorGlobalChannelsCount[] = "Error the global channels counter is different from the number of inserted channels.";

static char filePath[FILE_PATH_BUF_LEN];
static int fileFd = -1;
static char lineBuffer[LINE_LEN];

// Set filename global variable
void setFilePath(char *filePathName)
{
	strncpy(filePath, filePathName, FILE_PATH_BUF_LEN);
}

// Open I/O file
void openFile(unsigned char globalMode)
{
	if(globalMode == READ)
		fileFd = open(filePath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  
	else if(globalMode == WRITE)
		fileFd = open(filePath, O_RDONLY, 0);

	if(fileFd == -1)
	{
		perror("Error opening file: ");
		abnormalTermination(NULL);
	}
}

// Close I/O file
void closeFile(void)
{
	if(fileFd != -1)
	{
		if(close(fileFd) == -1)
			perror("Error closing file descriptor of I/O file: ");
		else 
			fileFd = -1;
	}
}

// Write memory to file, (write channels loaded from radio)
void fileWrite(void)
{
	/* Scrivo sul file il numero dei canali */
	int toWrite = 4;
	int written = 0, writeLeft = toWrite;
	
	// convert channels number to ascii and write to buffer 
	snprintf(lineBuffer, 5, "%03hhu\n", channelSetNumber);

	while(written != toWrite)
	{	
		int ret = write(fileFd, (lineBuffer+written), (writeLeft-written));
		if (ret == -1)
		{
			if(errno != EINTR)
			{
				perror("Error writing on file: ");
				abnormalTermination(NULL);
			}
		}
		else
			written += ret;
	}

	// Single channels writing loop
	unsigned char index;

	for(index=1; index<=USABLE_CH_NUM ; index++)
	{
		unsigned char count;
		// Prepare channel's ascii buffer
		char *workingPtr = lineBuffer;

		// channel number
		snprintf(workingPtr, 5, "%03hhu,", index);
		workingPtr += 4;
		
		unsigned int rxFreq = 0, txFreq = 0;
		
		// Frequency
		for(count=0;count<2;count++)
		{
			if(count == 0)
			{
				// RX
				rxFreq = freqReadBCD(radioMemory[index].rxFreq);
				printFreq(rxFreq, workingPtr, 11);
			}			
			else if(count == 1)
			{
				// TX
				txFreq = freqReadBCD(radioMemory[index].rxFreq);
				printFreq(txFreq, workingPtr, 11);
			}

			workingPtr += 10;
			workingPtr[0] = 0x2c; // comma
			workingPtr += 1;
		}

		if((rxFreq != 0) && (txFreq != 0))
		{
			// Code
			for(count=0;count<2;count++)
			{
				unsigned short code = 0;

				if(count == 0)
					// RX
					code = codeRead(radioMemory[index].sqDecode);	
				else if(count == 1)
					//TX
					code = codeRead(radioMemory[index].sqEncode);	
				

				printCode(code, workingPtr, 6);
				workingPtr += 5;
				workingPtr[0] = 0x2c; // comma
				workingPtr += 1;
			}

			// Potenza
			unsigned char rfPower = readPowerBitmap(radioMemory[index].power);

			if(rfPower == RF_HIGH)
			{
				workingPtr[0] = 0x48; // comma
				workingPtr += 1;
			}
			else if(rfPower == RF_LOW)
			{
				workingPtr[0] = 0x4c; // comma
				workingPtr += 1;
			}
		
			workingPtr[0] = 0x0a; // linefeed

			// Write to file
			toWrite = 40;
			writeLeft = toWrite;
			written = 0;
		
			while(written != toWrite)
			{	
				int ret = write(fileFd, lineBuffer, (writeLeft-written));
				if (ret == -1)
				{
					if(errno != EINTR)
					{
						perror("Error writing on file: ");
						abnormalTermination(NULL);
					}
				}
				else
					written += ret;
			}
		}
	}
}

// Check if buffer's bytes are numeric ascii digits followed by character passed with "character" unsigned char argument, return TRUE or FALSE
static unsigned char digitsChar(char *buffer, unsigned char character, unsigned int len)
{
	unsigned int index = 0;
	unsigned char retVal = TRUE;

	while((index < len) && retVal)
	{
		if(index != (len-1))
		{
			if((buffer[index] < 0x30) || (buffer[index] > 0x39))
				retVal = FALSE;
		}
		else
		{
			if(buffer[index] != character)
				retVal = FALSE;
		}
		
		index++;
	}

	return retVal;
}

// Read first 4 bytes of file, the global channels number followed by line feed and do a sanity check on read data 
static unsigned char readFileChannelNumber(void)
{
	unsigned char retVal = 0;
	char nChannelBuf[] = {0, 0, 0, 0};
	unsigned char toRead = 4;

	int readB = 0, readLeft = toRead;

	while(readB != toRead)
	{
		int ret = read(fileFd, (nChannelBuf+readB), (readLeft - readB));
		if(ret == -1)
		{
			if(errno != EINTR)
			{
				perror("Error reading from file: ");
				abnormalTermination(NULL);
			}
		}
		else if(ret == 0)
			abnormalTermination(emptyFile);
		else
			readB += ret;
	}

	
	if(digitsChar(nChannelBuf, 0x0a, 4))
	{
		nChannelBuf[3] = 0;

		retVal = atoi(nChannelBuf);
		
		if(!((retVal >= 1) && (retVal <= 101)))
			// READ DATA INCORRECT, WRONG CHANNEL NUMBER 
			abnormalTermination(channelsNumberWrong); 
	}
	else
		// READ DATA INCORRECT 
		abnormalTermination(incorrectData); 

	return retVal;
}

// Read the index number of selected channel, sanity check data read 
static unsigned char channelIndexRead(unsigned char *buffer, unsigned char channelLen)
{
	char workingBuffer[4]; 
	unsigned char retVal = 0;

	memcpy(workingBuffer, buffer, 4);
				
	if(digitsChar(workingBuffer, 0x2c, 4))
	{
		workingBuffer[3] = 0;

		retVal = atoi(workingBuffer);

		if(!((retVal >= 1) && (retVal <= 101)))
			// READ DATA INCORRECT, WRONG CHANNEL NUMBER
			abnormalTermination(channelNumberWrong); 
	}
	else
		// READ DATA INCORRECT 
		abnormalTermination(incorrectData); 

	return retVal;
}

// Read frequency, sanity check data, and return frequency as unsigned int 
static unsigned int channelReadFrequency(unsigned char *buffer, unsigned char channelLen, unsigned char rxTx)
{
	unsigned int retVal = 0;
	unsigned char offset = 4; 
	char workingBuffer[11];

	if(rxTx == TX)
		offset += 11;

	memcpy(workingBuffer, (buffer+offset), 11);

	// Do a sanity check, check if the frequency is in the "433.000.00" form followed by comma
	unsigned char i = 0, ok = TRUE;

	while((i<3) && ok)
	{
		if(i==0)
			ok = digitsChar(workingBuffer, 0x2e, 4);
		else if(i==1)
			ok = digitsChar((workingBuffer+4), 0x2e, 4);
		else if(i==3)
			ok = digitsChar((workingBuffer+8), 0x2c, 3);	
		i++;
	}

	if(!ok)
		abnormalTermination(incorrectData);
	
	workingBuffer[10] = 0;
	retVal = readFreq(workingBuffer, 10);

	// Check if the frequency is in allowed range
	if(!((retVal >= 42000000) && (retVal <= 45000000)))
		abnormalTermination(frequencyOutOfRange);

	return retVal;
}

// Read squelch code, do sanity check and return data as short int
static unsigned short channelReadCode(unsigned char *buffer, unsigned char channelLen, unsigned char rxTx)
{
	unsigned short retVal = 0;
	unsigned char offset = 26; 
	char workingBuffer[6];

	if(rxTx == TX)
		offset += 6;

	memcpy(workingBuffer, (buffer+offset), 6);
	
	// Check if squelch code is CTCSS or DCS
	
	if((workingBuffer[0] >= 0x30) && (workingBuffer[0] <= 0x39))
	{
		unsigned char i = 0, ok = TRUE, ctcss = FALSE;
		
		do
		{
			if(workingBuffer[i] != 0x30)
				ctcss = TRUE;
			
			i++;
		} while ((i<5) && (!ctcss));

		i=0;
		if(ctcss)
		{
			// CTCSS
			while((i < 2) && ok)
			{
				if(i==0)
					ok = digitsChar((workingBuffer+1), 0x2e, 3);
				else if(i==1)
					ok = digitsChar((workingBuffer+4), 0x2c, 2);
			
				i++;
			}
		}
		else
		{
			// if the bytes are all set to 0, check if the bytes are followed by comma 
			if(workingBuffer[5] != 0x2c)
				ok = FALSE;
		}


		if(!ok)
			abnormalTermination(incorrectData); 
	}
	else if(workingBuffer[0] == 0x44)
	{
		unsigned char i = 0, ok = TRUE;

		// DCS
		while((i < 2) && ok)
		{
			if(i==0)
			{
				ok = digitsChar((workingBuffer+1), 0x4e, 4);
				if(!ok)
					ok = digitsChar((workingBuffer+1), 0x49, 4);
			}			
			else if(i==1)
			{
				if(workingBuffer[5] != 0x2c)
					ok = FALSE;
			}

			i++;
		}

		if(!ok)
			abnormalTermination(incorrectData); 
	}
	else
		abnormalTermination(incorrectData); 

	workingBuffer[5] = 0;
	retVal = readCode(workingBuffer, 5);
	
	if(retVal == 0xffff)
		abnormalTermination(incorrectCode); 

	return retVal;
}

// Read RF power, do sanity check and return power as unsigned char
static unsigned char channelReadPower(unsigned char *buffer, unsigned char channelLen)
{
	unsigned char retVal = 3;

	if(buffer[channelLen-1] != 0x0a)
		abnormalTermination(incorrectData); 

	if(buffer[channelLen-2] == 0x48)
		retVal = RF_HIGH;
	else if(buffer[channelLen-2] == 0x4c)
		retVal = RF_LOW;
	else
		abnormalTermination(powerCharWrong); 

	return retVal;
}

// Convert data in the memory formats and write to memory
void saveToMemory(unsigned char channelIndex, unsigned int rxFreq, unsigned int txFreq, unsigned short rxCode, unsigned short txCode, unsigned char rfPower)
{
	// Save frequency
	freqWriteBCD(radioMemory[channelIndex].rxFreq, rxFreq);
	freqWriteBCD(radioMemory[channelIndex].txFreq, txFreq);

	// Squelch codes
	codeWrite(rxCode, radioMemory[channelIndex].sqDecode);
	codeWrite(txCode, radioMemory[channelIndex].sqEncode);

	// RF output power
	writePower2Bitmap(radioMemory[channelIndex].power, rfPower);
}

// Read data from file and save to memory structure
void fileRead(void)
{
	unsigned char channelNumber = readFileChannelNumber();
	unsigned short readBufferLen = (channelNumber * LINE_LEN);
	// Alloc a buffer sized with number of channels
	unsigned char *readBuffer = malloc(readBufferLen); 

	if(readBuffer == NULL)
	{
		perror("Malloc error: ");
		abnormalTermination(NULL); 
	}

	// Read nChannels * LINE_LEN byte
	unsigned short toRead = readBufferLen;
	int readB = 0, readLeft = toRead;

	while(readB != toRead)
	{
		int ret = read(fileFd, (readBuffer+readB), (readLeft - readB));
		
		if(ret == -1)
		{
			if(errno != EINTR)
			{
				perror("Error reading from file: ");
				abnormalTermination(NULL);
			}
		}
		else if(ret == 0)
			abnormalTermination(errorGlobalChannelsCount);
		else
			readB += ret;
	}

	// Processing read channels
	unsigned char index = 0;

	while(index < channelNumber)
	{
		unsigned int rxFreq = 0, txFreq = 0;
		unsigned short rxCode = 0, txCode = 0;
		unsigned char channelIndex = 0, rfPower = 0;
		unsigned char *selectedChannel = (readBuffer + (index*LINE_LEN));
		

		channelIndex = channelIndexRead(selectedChannel, LINE_LEN);
		rxFreq = channelReadFrequency(selectedChannel, LINE_LEN, RX);
		txFreq = channelReadFrequency(selectedChannel, LINE_LEN, TX);

		rxCode = channelReadCode(selectedChannel, LINE_LEN, RX);
		txCode = channelReadCode(selectedChannel, LINE_LEN, TX);
		rfPower = channelReadPower(selectedChannel, LINE_LEN);
		
		saveToMemory(channelIndex, rxFreq, txFreq, rxCode, txCode, rfPower);

		index++;
	}

	// Setto global number of used channel
	channelSetNumber = channelNumber;

	// Set channels bitmap
	setChannelsBitmap(channelNumber);
}
