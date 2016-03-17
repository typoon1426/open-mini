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

/* Main module */

/* TODO POSSIBLE OPTIMIZATION HASTABLE */

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "cmdLineParser.h"
#include "serialDevice.h"
#include "fileIO.h"
#include "radio.h"
#include "memory.h"
#include "main.h"
#include "error.h"

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(*(x)))

static unsigned char mode = 2;

// signal handler
static void sigHandler(int sig)
{
	printf("Caught signal %d, close all socket and exiting.\n", sig);
	abnormalTermination(NULL);
}

// Set signal handler with sigaction
static void setSignalHandlers(void)
{

	// all signals - SIGALRM
	struct sigaction actionIgnore, actionDefault;
	int i, signals[] = {SIGHUP, SIGINT, SIGPIPE, SIGTERM, SIGUSR1, SIGUSR2, SIGPROF, SIGVTALRM, SIGALRM};

	memset(&actionIgnore, 0, sizeof(actionIgnore));
	memset(&actionDefault, 0, sizeof(actionDefault));

	actionIgnore.sa_handler = SIG_IGN;
	actionDefault.sa_handler = sigHandler;

	for(i = 0; i<ARRAY_LENGTH(signals); i++)
	{
		if((signals[i] == SIGHUP) || (signals[i] == SIGINT) || (signals[i] == SIGTERM))
		{
			if(sigaction(signals[i], &actionDefault, NULL) < 0)
			{
				perror("Error sigaction:");
				abnormalTermination(NULL);
			}
		}
		else
		{
			if(sigaction(signals[i], &actionIgnore, NULL) < 0)
			{
				perror("Error sigaction:");
				abnormalTermination(NULL);
			}
		}
	}
}


int main(int argc, char *argv[])
{
	order = orderTest();

	setSignalHandlers();
	
	int argcount = cmdLineParse(argc, argv);

	if(argcount)
	{
		initMemory();
		openSerialDevice();

		printf("\n********* Open-Mini *********\n");
		if(mode == READ)
		{	
			printf("\nRead mode: load channels from radio memory.\nWaiting....\n");
			radioRead();
			channelsMemoryPrint(READ);
			//printMemory();

			if(argcount == OK_B)
			{
				openFile(mode);
				printf("\nWrite loaded channels to file.\n");
				fileWrite();
				printf("Writing terminated correctly.\n\n");
			}
		}		
		else if(mode == WRITE)
		{
			openFile(mode);
			printf("\nWrite mode: load channels from file and write to radio memory.\n");
			fileRead();
			channelsMemoryPrint(WRITE);
			
			printf("\nWrite loaded channels to radio memory.\nWaiting....\n");
			radioWrite();
			printf("Writing terminated correctly.\n\n");
		}

		closeAll();
	}
	else
		printUsage();

	return 0;
}

// set global mode
void setMode(unsigned char newMode)
{
	mode = newMode;
}

// return global mode
unsigned char getMode(void)
{
	return mode;
}


void closeAll(void)
{
	closeFile();
	closeSerialDevice();
}

// Check machine endianness
unsigned char orderTest(void)
{
	unsigned short test = 1;
	unsigned char *testPtr = (unsigned char *) &test;
	unsigned char retVal = 0;

	if(*testPtr == 1)
		retVal = LITTLE;
	else if(*testPtr == 0)
		retVal = BIG;

	return retVal;
}
