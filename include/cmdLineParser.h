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

/* Command line parser module header file */

#ifndef __CMDLINEPARSER_H__
#define __CMDLINEPARSER_H__ 1

#define DEVICE_WEIGHT 10
#define READ_WEIGHT 4
#define WRITE_WEIGHT 3
#define FILE_WEIGHT 1
#define HELP_WEIGHT 15
#define OK_A 14
#define OK_B 15

int cmdLineParse(int argc, char *argv[]);
void printUsage(void);
#endif
