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

/* Command line parser module */

#include <stdio.h>
#include <getopt.h>

#include "cmdLineParser.h"
#include "fileIO.h"
#include "serialDevice.h"
#include "main.h"

static const char usage[] = 
			"Usage: openMini [OPTIONS]\n"
			"Runs Polmar Mini programming software.\n"
			"  -h, --help                 		Display this help and exit\n"
			"  -r, --read				Select read mode, read data from radio and write to file, if filename option is specified, if unspecified print read data to stdout\n"
			"  -w, --write				Select write mode, read data from file and write to radio, after a sanity check of data read\n"
			"  -f, --filename			Select filename of the file to save data from radio or read data to radio\n"
			"  -d, --device				Select filename of the serial device, attached to radio\n";

// Set device path name and device weight for correctly parsing command line
static void setDevice(char *devicePathName, unsigned int *argumentWeight)
{
	*argumentWeight += DEVICE_WEIGHT;

	setSerialDevPath(devicePathName);
}

// Set read mode and read weight for correctly parsing command line
static void setReadMode(unsigned int *argumentWeight)
{
	*argumentWeight += READ_WEIGHT;

	setMode(READ);
}

// Set write mode and write weight for correctly parsing command line
static void setWriteMode(unsigned int *argumentWeight)
{
	*argumentWeight += WRITE_WEIGHT;
	
	setMode(WRITE);
}

// Set file pathname and filename weight for correctly parsing command line
static void setFileName(char *filePathName, unsigned int *argumentWeight)
{
	*argumentWeight += FILE_WEIGHT;

	setFilePath(filePathName);
}

// Set usage mode and usage weight for correctly parsing command line
static void setUsage(unsigned int *argumentWeight)
{
	*argumentWeight += HELP_WEIGHT;
}

// Command line parsing function
int cmdLineParse(int argc, char *argv[])
{
	int c = 0, option_index = 0;
	unsigned int argcount = 0;
	
	struct option long_options[] = {
	{"device", 1, 0, 'd'},
	{"read", 0, 0, 'r'},
	{"write", 0, 0, 'w'},
	{"filename", 1, 0, 'f'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0},
	};
	
	while(c != -1)
	{
		c = getopt_long(argc, argv, "d:rwf:h", long_options, &option_index);

		switch (c)
		{
			case 'd':
				setDevice(optarg, &argcount);
			break;

			case 'r':
				setReadMode(&argcount);
			break;

			case 'w':
				setWriteMode(&argcount);
			break;
			
			case 'f':
				setFileName(optarg, &argcount);
			break;
			
			case 'h':
			case '?':
				setUsage(&argcount);
			break;
		}
	}

	if(argcount == OK_A)
		return argcount;
	else
		return 0;
}

void printUsage(void)
{
	printf("%s\n", usage);
}
