/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : testdetect.c
* DESCRIPTION   : Test program for automatic printers detection
* CVS           : $Id: testdetect.c,v 1.5 2007/12/10 15:14:36 nicolas Exp $
*******************************************************************************
*   Copyright (C) 2006  APS Engineering
*
*   This file is part of libaps.
*
*   libaps is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   libaps is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with libaps; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*******************************************************************************
* HISTORY       :
*   10feb2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aps/aps.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

#define ESC     27
#define GS      29

#define PRINTERS_MAX    31
static aps_printer_t printers[PRINTERS_MAX];

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

static void detect(unsigned char set,int baudrate,int handshake)
{
        void *port;
        int n;
        int i;
        unsigned char cmd[3] = {GS,'B',set};

        /*unused variables*/
        (void)baudrate;
        (void)handshake;

        printf("Printer serial settings = 0x%02X\n",set);

        /*configure serial printer*/
        port = aps_create_serial_port("/dev/ttyS0");

        aps_open(port);
        aps_write(port,cmd,sizeof(cmd));
        aps_sync(port);
        aps_close(port);

        aps_destroy_port(port);
        
        /*detect printer*/
        n = aps_detect_printers(printers,PRINTERS_MAX);

        if (n==0) {
                fputs("No printers found\n",stderr);
                exit(1);
        }

        printf("%d printer(s) found\n",n);

        for (i=0; i<n; i++) {
                printf("printers[%d].model = %d\n",i,printers[i].model);
                printf("printers[%d].identity = %s\n",i,printers[i].identity);
                printf("printers[%d].uri = %s\n",i,printers[i].uri);
        }

        /*leave printer in default state*/
        port = aps_create_port(printers[0].uri);

        cmd[2] = 0x83;  /*default*/
        
        aps_open(port);
        aps_write(port,cmd,sizeof(cmd));
        aps_sync(port);
        aps_close(port);

        aps_destroy_port(port);
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  main
Purpose   :  Program main function
Inputs    :  argc : number of command-line arguments
             argv : array of command-line arguments
Outputs   :  <>
Return    :  0 if success, 1 on error
-----------------------------------------------------------------------------*/
int main(int argc,char **argv)
{
        (void)argc;
        (void)argv;

        printf("testdetect compiled with APS library %d.%d.%d\n",
                        APS_MAJOR,
                        APS_MINOR,
                        APS_BUGFIX);

        /*FIXME: 2400 bauds detection does not work*/
        
        detect(0x80,APS_B1200,APS_RTSCTS);
//        detect(0x81,APS_B2400,APS_RTSCTS);
        detect(0x82,APS_B4800,APS_RTSCTS);
        detect(0x83,APS_B9600,APS_RTSCTS);
        detect(0x84,APS_B19200,APS_RTSCTS);
        detect(0x85,APS_B38400,APS_RTSCTS);
        detect(0x86,APS_B57600,APS_RTSCTS);
        detect(0x87,APS_B115200,APS_RTSCTS);

        detect(0x00,APS_B1200,APS_XONXOFF);
//        detect(0x01,APS_B2400,APS_XONXOFF);
        detect(0x02,APS_B4800,APS_XONXOFF);
        detect(0x03,APS_B9600,APS_XONXOFF);
        detect(0x04,APS_B19200,APS_XONXOFF);
        detect(0x05,APS_B38400,APS_XONXOFF);
        detect(0x06,APS_B57600,APS_XONXOFF);
        detect(0x07,APS_B115200,APS_XONXOFF);

        return 0;
}

