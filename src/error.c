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

/* Error handler module */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "error.h"
#include "serialDevice.h"
#include "fileIO.h"
#include "radio.h"

// Abnormal termination: if argument is != NULL print argument string and close all device and file then exit with "1" exit code
void abnormalTermination(const char *errorString)
{
	if(errorString != NULL)
		fprintf(stderr, "\n%s\n", errorString);

	closeSerialDevice();
	closeFile();
	exit(1);
}

// Same as abnormalTermination, with writing protocol terminal character  
void errorTerminateProtocol(const char *errorStr)
{
	int serialFd = getSerialFd();
	int written = 0, writeLeft = 1;
	unsigned char protocolTermChar = TERMC;

	if(errorStr != NULL)
		fprintf(stderr, "\n%s\n", errorStr);

	while(written != 1)
	{	
		int ret = write(serialFd, &protocolTermChar, (writeLeft-written));
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

	closeSerialDevice();
	closeFile();
	exit(1);
}
