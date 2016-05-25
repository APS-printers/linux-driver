/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : testaps.c
* DESCRIPTION   : Test program for APS library
* CVS           : $Id: testaps.c,v 1.8 2007/12/10 15:14:36 nicolas Exp $
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

/*command-line options*/
static const char *     uri;
static int              opt_type;

static void *port;

static const char ticket[] = "This is a test ticket\n\n";
static const char test_B115200[] = "Printed at 115200 bauds\n\n\n";
static const char test_B9600[] = "Printed at 9600 bauds\n\n\n";

static const unsigned char cmd_get_identity[] = {ESC,'I'};
static const unsigned char cmd_get_identity_hsp[] = {GS,'I',67};

static const unsigned char cmd_set_B9600[] = {GS,'B',0x83};
static const unsigned char cmd_set_B115200[] = {GS,'B',0x87};

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  clean
Purpose   :  Clean ressources
             This function is called by exit() to clean port ressources
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void clean(void)
{
        int errnum;
        
        if (port!=NULL) {

                if ((errnum = aps_flush(port))<0) {
                        fprintf(stderr,"clean: %s\n",aps_get_strerror_full(errnum,port));
                }

                if ((errnum = aps_close(port))<0) {
                        fprintf(stderr,"clean: %s\n",aps_get_strerror_full(errnum,port));
                }

                if ((errnum = aps_destroy_port(port))<0) {
                        fprintf(stderr,"clean: %s\n",aps_get_strerror_full(errnum,port));
                }
        }
}

/*-----------------------------------------------------------------------------
Name      :  check
Purpose   :  Check APS library error code. Print error string on error
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void check(int errnum)
{
        if (errnum<0) {
                fprintf(stderr,"check: %s\n",aps_get_strerror_full(errnum,port));
                exit(1);
        }
}

/*-----------------------------------------------------------------------------
Name      :  parse_options
Purpose   :  Parse command-line options and update global variables
Inputs    :  argc : number of command-line arguments
             argv : array of command-line arguments
Outputs   :  Update global variables
Return    :  <>
-----------------------------------------------------------------------------*/
static void parse_options(int argc,char **argv)
{
        int i;
        
        /*set defaults*/
        uri = NULL;
        opt_type = APS_MRS;

        /*parse options*/
        for (i=1; i<argc; i++) {
                if (strcmp(argv[i],"-t")==0) {
                        i++;

                        if (i==argc) {
                                fprintf(stderr,"parse_options: missing model type\n");
                                exit(1);
                        }
                        else if (strcmp(argv[i],"mrs")==0) {
                                opt_type = APS_MRS;
                        }
                        else if (strcmp(argv[i],"hrs")==0) {
                                opt_type = APS_HRS;
                        }
                        else if (strcmp(argv[i],"kcp")==0) {
                                opt_type = APS_KCP;
                        }
                        else if (strcmp(argv[i],"hsp")==0) {
                                opt_type = APS_HSP;
                        }
                        else {
                                fprintf(stderr,"parse_options: invalid model type (%s)\n",argv[i]);
                                exit(1);
                        }
                }
                else if (i==argc-1) {
                        uri = argv[i];
                }
                else {
                        fprintf(stderr,"parse_options: unrecognized option (%s)\n",argv[i]);
                        exit(1);
                }
        }

        /*URI string is mandatory*/
        if (uri==NULL) {
                fprintf(stderr,"parse_options: missing device uri\n");
                exit(1);
        }
}

/*-----------------------------------------------------------------------------
Name      :  test_common
Purpose   :  Test libaps features common to all port types
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void test_common(void)
{
        char line[100];
        char identity[32];
        int i;

        /*write simple ticket*/
        check(aps_write(port,ticket,strlen(ticket)));

        /*get printer identity*/
        check(aps_set_read_timeout(port,1000));
        
        check(aps_sync(port));
        check(aps_flush(port));
        if (opt_type == APS_HSP)
        {
            check(aps_write(port,cmd_get_identity_hsp,sizeof(cmd_get_identity_hsp)));
        }
        else
        {
            check(aps_write(port,cmd_get_identity,sizeof(cmd_get_identity)));
        }
        check(aps_gets(port,identity,sizeof(identity)));
        
        printf("identity=%s\n",identity);

        /*print a large amount of data with timeout*/
        check(aps_set_write_timeout(port,5000));
        for (i=0; i<100; i++) {
                sprintf(line,"line %d: ABCDEFGHIJKLMNOPQRSTUVWXYZ\n",i);
                check(aps_write(port,line,strlen(line)));
        }
}

/*-----------------------------------------------------------------------------
Name      :  test_serial
Purpose   :  Test libaps features specific to serial ports
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void test_serial(void)
{
        /* default baudrate should be 9600 bauds
         * default handshake should be RTS/CTS
         */

        printf("current serial baudrate=%d\n",aps_serial_get_baudrate(port));
        printf("current serial handshake=%d\n",aps_serial_get_handshake(port));

        /* switch to 115200 bauds */
        check(aps_write(port,cmd_set_B115200,sizeof(cmd_set_B115200)));
        check(aps_sync(port));
        check(aps_serial_set_baudrate(port,APS_B115200));
        check(aps_write(port,test_B115200,strlen(test_B115200)));

        /* revert to 9600 bauds */
        check(aps_write(port,cmd_set_B9600,sizeof(cmd_set_B9600)));
        check(aps_sync(port));
        check(aps_serial_set_baudrate(port,APS_B9600));
        check(aps_write(port,test_B9600,strlen(test_B9600)));
}

/*-----------------------------------------------------------------------------
Name      :  test_parallel
Purpose   :  Test libaps features specific to parallel ports
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void test_parallel(void)
{
        /*no specific features*/
}

/*-----------------------------------------------------------------------------
Name      :  test_usb
Purpose   :  Test libaps features specific to USB ports
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void test_usb(void)
{
        /*no specific features*/
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
        port = NULL;

        atexit(clean);

        if (argc<2) {
                printf("testaps compiled with APS library %d.%d.%d\n",
                                APS_MAJOR,
                                APS_MINOR,
                                APS_BUGFIX);

                printf("usage: testaps [-t mrs|hrs|kcp|hsp] uri\n");
                return 0;
        }

        parse_options(argc,argv);

        /*create port structure*/
        port = aps_create_port(uri);

        if (port==NULL) {
                fprintf(stderr,"main: error creating printer port\n");
                exit(1);
        }

        check(aps_get_error(port));
        
        /*open port*/
        check(aps_open(port));

        /*test features common to all port types*/
        test_common();

        /*test port type specific features*/
        switch (aps_get_port_type(port)) {
            case APS_SERIAL:
                test_serial();
                break;
            case APS_PARALLEL:
                test_parallel();
                break;
            case APS_USB:
                test_usb();
                break;
            case APS_ETHERNET:
//                test_ethernet();
                fprintf(stderr,"no implemented yet\n");
                break;
            default:
                fprintf(stderr,"main: unknown printer port type\n");
                exit(1);
        }
                
        /*close port*/
        check(aps_close(port));

        /*destroy port structure*/
        check(aps_destroy_port(port));
        
        port = NULL;

        return 0;
}

