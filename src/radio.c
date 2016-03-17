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

/* Radio I/O protocol module */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "memory.h"
#include "radio.h"
#include "error.h"
#include "serialDevice.h"
#include "main.h"

#define POLL_TIMEOUT 1500
#define MAXCOUNT_TIMEOUT 5

static const char notCompatibleRadio[] = "Error incompatible radio device. Abort operation.\n";
static unsigned char program[] = {'P', 'R', 'O', 'G', 'R', 'A', 'M'}; // "PROGRAM" with ascii characters
static unsigned char deviceTag[] = {'N', 'F', '5', '6', '9', 'U', 0xff, 0xff}; // ascii device tag, NF569U followed by two bytes setted to 0xFF

static unsigned char readBuffer[READ_BUF_LEN];
static unsigned char writeBuffer[WRITE_BUF_LEN];

// read from radio and write to memory structure
void radioRead(void)
{
	int serialFd = getSerialFd();
	int toWrite = 7; // "program" length
	int toRead = 1; // ack length
	unsigned char *writePtr = program; 
	unsigned char *readPtr = readBuffer;
	unsigned char radioAddress[2] = {0, 0};
	unsigned char *mostSHalfAddress = &radioAddress[1];
	unsigned char *leastSHalfAddress = &radioAddress[0];
	unsigned char terminate = FALSE, stop = FALSE, rep = FALSE, count = 0;

	do	
	{
		/* Write to radio, do an initial loop for writing program because radio sometime don't ack immediatly */
		int written = 0, writeLeft = toWrite;

		do
		{
			if(writePtr == program)
			{
				rep = FALSE;
				written = 0;
				writeLeft = toWrite;
			}

			while(written != toWrite)
			{	
				int ret = write(serialFd, (writePtr+written), (writeLeft-written));
				if (ret == -1)
				{
					if(errno != EINTR)
					{
						perror("Error writing on serial device: ");
						abnormalTermination(NULL);
					}
				}
				else
					written += ret;
			}

			if(writePtr == program)
			{
				struct pollfd devFds[] = {{serialFd, POLLIN, 0}};

				int ret = poll(devFds, 1, POLL_TIMEOUT);

				if(ret == -1)
				{
					perror("Error poll: ");
					errorTerminateProtocol(NULL);
				}
				else if(ret == 0)
				{
					rep = TRUE;
					count++;
				}
			}

		} while ((rep) && (count < MAXCOUNT_TIMEOUT));

		if(count == MAXCOUNT_TIMEOUT)
			errorTerminateProtocol("Serial line problem. Check cable connections and restart radio device then retry.\n");

		/* Read from radio */
		int readB = 0, readLeft = toRead;

		if(terminate)
			stop = TRUE;
		else
		{
			while(readB != toRead)
			{
				int ret = read(serialFd, (readPtr+readB), (readLeft - readB));
				if(ret == -1)
				{
					if(errno != EINTR)
					{
						perror("Error loading from serial device: ");
						abnormalTermination(NULL);
					}
				}
				else
					readB += ret;
			}
		}

		// with number and value of read bytes, prepare the next writing operation
		if(readB == 1)
		{
			// read 1 byte can be only ACK	
			if(readBuffer[0] == ACK)
			{
				// read ack, if previous writing was a program now write STX and waiting for read device tag
				if(written == 7)
				{
					// if read ACK prepare for write STX and for read device tag in the next reading operation
					toWrite = 1;
					writeBuffer[0] = STX;
					writePtr = writeBuffer;

					// prepare tag read
					toRead = 8; 
				}
				else if(written == 1)
				{
					// check if memory address is less than 2048 for terminate read operations
					unsigned short saveAddress = codeRead(radioAddress);

					if(saveAddress < 0x0800)
					{
						// if read ack and writed ack set to write read command and waiting for read 68 bytes of channels block
						toWrite = 4;
						writeBuffer[0] = READC;
						writeBuffer[1] = *mostSHalfAddress;
						writeBuffer[2] = *leastSHalfAddress;
						writeBuffer[3] = READ_OFFSET;
						writePtr = writeBuffer;

						// prepare the read
						toRead = 68;
					}
					else
					{
						// prepare terminate character writing
						toWrite = 1;
						writeBuffer[0] = TERMC;
						writePtr = writeBuffer;

						// stop reading loop
						terminate = TRUE;
					}
				}
			}
			else
				errorTerminateProtocol("Error not expected byte!\n");
		}
		else if(readB == 8)
		{
			// read device tag check if device is compatible, set next write to ack and next read to ack
			// letto il device tag verifico che sia quello dell'apparato compatibile, poi setto la prossima scrittura un ack e la prossima lettura un altro ack
			unsigned char i=0;

			for(i=0;i<8;i++) 
			{
				if(readBuffer[i] != deviceTag[i])
					errorTerminateProtocol(notCompatibleRadio);
			}

			toWrite = 1;
			writeBuffer[0] = ACK;
			writePtr = writeBuffer;

			toRead = 1;
		}
		else if(readB == 68)
		{
			// read header with channels block
			// check if header is ok
			unsigned char i=0;
			unsigned short saveAddress=0;
			
			for(i=0;i<4;i++)
			{
				if(i==0)
				{
					if(readBuffer[i] != WRITEC)
						errorTerminateProtocol("Error read command different from written command!\n");
				}
				else
				{
					if(readBuffer[i] != writeBuffer[i])
						errorTerminateProtocol("Error read command different from written command!\n");
				}
			}

			// set address to read and write as unsigned short
			saveAddress = codeRead(radioAddress);
			// copy 64 bytes from buffer to channels memory
			memcpy((((unsigned char *) radioMemory) + saveAddress), (readBuffer + 4), READ_OFFSET);

			// set new address of next channels block
			saveAddress += READ_OFFSET;

			codeWrite(saveAddress, radioAddress);
			
			toWrite = 1;
			writeBuffer[0] = ACK;
			writePtr = writeBuffer;

			toRead = 1;
		}

		/**********************/
		
	} while (!stop);

	countChannels();
}

// read channels from memory and write to radio device
void radioWrite(void)
{
	int serialFd = getSerialFd();
	int toWrite = 7; 
	int toRead = 1; 
	unsigned char *writePtr = program; 
	unsigned char *readPtr = readBuffer; 
	unsigned char radioAddress[2] = {0, 0};
	unsigned char *mostSHalfAddress = &radioAddress[1];
	unsigned char *leastSHalfAddress = &radioAddress[0];
	unsigned char terminate = FALSE, stop = FALSE, rep = FALSE, count = 0;

	do
	{
		/* Write to radio */ 
		int written = 0, writeLeft = toWrite;

		do
		{
			if(writePtr == program)
			{
				rep = FALSE;
				written = 0;
				writeLeft = toWrite;
			}

			while(written != toWrite)
			{	
				int ret = write(serialFd, (writePtr+written), (writeLeft-written));
				if (ret == -1)
				{
					if(errno != EINTR)
					{
						perror("Error writing on serial device: ");
						abnormalTermination(NULL);
					}
				}
				else
					written += ret;
			}

			if(writePtr == program)
			{
				struct pollfd devFds[] = {{serialFd, POLLIN, 0}};

				int ret = poll(devFds, 1, POLL_TIMEOUT);

				if(ret == -1)
				{
					perror("Error poll: ");
					errorTerminateProtocol(NULL);
				}
				else if(ret == 0)
				{
					rep = TRUE;
					count++;
				}
			}

		} while ((rep) && (count < MAXCOUNT_TIMEOUT));

		if(count == MAXCOUNT_TIMEOUT)
			errorTerminateProtocol("Serial line problem. Check cable connections and restart radio device then retry.\n");

		/* Read from radio */ 
		int readB = 0, readLeft = toRead;

		if(terminate)
			stop = TRUE;
		else
		{
			while(readB != toRead)
			{
				int ret = read(serialFd, (readPtr+readB), (readLeft - readB));
				if(ret == -1)
				{
					if(errno != EINTR)
					{
						perror("Error reading from serial device: ");
						abnormalTermination(NULL);
					}
				}
				else
					readB += ret;
			}
		}

	
		if(readB == 1)
		{	
			if(readBuffer[0] == ACK)
			{
				if(written == 7)
				{
					toWrite = 1;
					writeBuffer[0] = STX;
					writePtr = writeBuffer;
					toRead = 8;
				}
				else if(written == 1)
				{
					// prepare first channel block writing
					toWrite = 20;
					writeBuffer[0] = WRITEC;
					writeBuffer[1] = *mostSHalfAddress;
					writeBuffer[2] = *leastSHalfAddress;
					writeBuffer[3] = WRITE_OFFSET;

					memset((writeBuffer + 4), 0xff, 16);
					writePtr = writeBuffer;
					toRead = 1;
				}
				else if(written == 20)
				{
					unsigned short saveAddress = codeRead(radioAddress);

					saveAddress += WRITE_OFFSET;

					codeWrite(saveAddress, radioAddress);

					if(saveAddress < 0x07c0)
					{
						toWrite = 20;
						writeBuffer[0] = WRITEC;
						writeBuffer[1] = *mostSHalfAddress;
						writeBuffer[2] = *leastSHalfAddress;
						writeBuffer[3] = WRITE_OFFSET;

						// copy channel block from memory to buffer
						memcpy((writeBuffer + 4), (((unsigned char *) radioMemory) + saveAddress), WRITE_OFFSET);
						writePtr = writeBuffer;

						toRead = 1;
					}
					else
					{
						// prepare to write terminate operation character
						toWrite = 1;
						writeBuffer[0] = TERMC;
						writePtr = writeBuffer;

						// stop reading loop
						terminate = TRUE;
					}
					
				}
			}
			else
				errorTerminateProtocol("Error not expected byte!\n");
		}
		else if(readB == 8)
		{
			unsigned char i=0;
			for(i=0; i<8; i++)
			{
				if(readBuffer[i] != deviceTag[i])
					errorTerminateProtocol(notCompatibleRadio);
			}

			toWrite = 1;
			writeBuffer[0] = ACK;
			writePtr = writeBuffer;
			toRead = 1;
		}

	} while (!stop);
}
