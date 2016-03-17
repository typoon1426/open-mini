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

/* Serial device handler module */

#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "serialDevice.h"
#include "error.h"
#include "main.h"

static char serialDevicePath[BUFLEN];
static int serialFd = -1;

// set configurations to serial ports
static void configureSerialDev(void)
{
	int ret = -1;
	struct termios options;

	
	// set structures to actual interface configuration
	ret = tcgetattr(serialFd, &options);
	if(ret == -1)
	{
		perror("Error tcgetattr: ");
		abnormalTermination(NULL);
	}

	// set baud speed to 9600 bps
	cfsetispeed(&options, B9600);
    	cfsetospeed(&options, B9600);
	
	// Set wordsize, parity, and stop bit 
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	// unset hardware flow control
	options.c_cflag &= ~CRTSCTS;

	// set CREAD and CLOCAL
	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;

	// unset software flow control
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	// set Output raw
	options.c_oflag = 0;

	// unset canonical mode, echo e all local modes flags
	options.c_lflag = 0;

	// set minimal characters number to read return and timeout between characters
	options.c_cc[VMIN]=1;
        options.c_cc[VTIME]=0;

	// apply modifies
	ret = tcsetattr(serialFd, TCSANOW, &options);
	if(ret == -1)
	{
		perror("Errore tcsetattr: ");
		abnormalTermination(NULL);
	}
}

// open serial port
void openSerialDevice(void)
{
	serialFd = open(serialDevicePath, O_RDWR | O_NOCTTY);
	if(serialFd == -1)
	{
		perror("Error opening serial port: ");
		abnormalTermination(NULL);
	}

	configureSerialDev();
}

// close serial port
void closeSerialDevice(void)
{
	unsigned char repeat = FALSE;
	
	if(serialFd != -1)
	{
		do
		{
			if(close(serialFd) == -1)
			{
				if(errno == EINTR)
					repeat = TRUE;
				else
					perror("Error closing serial port: ");
			}
			else
				repeat = FALSE;
		} while (repeat);

		serialFd = -1;
	}
}

// set serial device path
void setSerialDevPath(char *devicePathName)
{
	strncpy(serialDevicePath, devicePathName, BUFLEN);
}

int getSerialFd(void)
{
	return serialFd;
}
