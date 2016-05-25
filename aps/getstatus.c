/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : getstatus.c
* DESCRIPTION   : Retrieve status from APS printer and print to stdout
* CVS           : $Id: getstatus.c,v 1.8 2006/10/09 14:29:09 nicolas Exp $
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
*   09mar2006   nico    Initial revision
*   07oct2006   nico    Added NEOP status retrieval (MRS/HRS/KCP)
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

static const unsigned char cmd_get_status_aps[] = {ESC,'v'};
static const unsigned char cmd_get_neop_status_aps[] = {ESC,'n','s'};

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
                fprintf(stderr,"check: %s\n",aps_strerror(errnum));
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
Name      :  print_status
Purpose   :  Print status to standard output in human-readable format
Inputs    :  status : printer status
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void print_status(aps_status_t *status)
{
        if (status->printing)
                puts("Printer is printing");
        else
                puts("Printer is idle");

        if (status->online)
                puts("Printer is online");
        else
                puts("Printer is offline");

        if (status->end_of_paper)
                puts("End of paper");
        else
                puts("Paper present");

        if (status->near_end_of_paper)
                puts("Near end of paper");
        else
                puts("No near end of paper");

        if (status->head_up)
                puts("Head up");
        else
                puts("Head down");

        if (status->cover_open)
                puts("Printer cover open");
        else
                puts("Printer cover closed");

        if (status->temp_error)
                puts("Head temperature error");
        else
                puts("Head temperature OK");

        if (status->supply_error)
                puts("Power supply error");
        else
                puts("Power supply OK");

        if (status->mark_error)
                puts("Mark detection error");
        else
                puts("Mark detection OK");

        if (status->cutter_error)
                puts("Cutter error");
        else
                puts("Cutter OK");

        if (status->mechanical_error)
                puts("Mechanical error");
        else
                puts("Mechanism OK");

        if (status->presenter_error)
                puts("Presenter error");
        else
                puts("Presenter OK");

        switch (status->presenter_action) {
        case APS_PRESENTER_IDLE:
                puts("Presenter is idle/absent");
                break;
        case APS_PRESENTER_PRESENTING:
                puts("Presenter is presenting");
                break;
        case APS_PRESENTER_CUTTING:
                puts("Presenter is cutting");
                break;
        case APS_PRESENTER_RETRACTING:
                puts("Presenter is retracting");
                break;
        case APS_PRESENTER_REPRESENTING:
                puts("Presenter is presenting again");
                break;
        case APS_PRESENTER_JAM_CLEARING:
                puts("Presenter is performing jam clearing");
                break;
        default:
                puts("Unknown presenter action");
                break;
        }
        
        if (status->rcpt_front_exit)
                puts("Receipt at front exit");
        else
                puts("No receipt at front exit");

        if (status->rcpt_retract_exit)
                puts("Receipt at retract exit");
        else
                puts("No receipt at retract exit");

        switch (status->rcpt_status) {
        case APS_RCPT_NOT_COLLECTED:
                puts("Receipt not collected");
                break;
        case APS_RCPT_COLLECTED:
                puts("Receipt collected");
                break;
        case APS_RCPT_RETRACT_COMPLETE:
                puts("Receipt retraction is complete");
                break;
        case APS_RCPT_RETRACT_PARTIAL:
                puts("Receipt retraction is partial");
                break;
        default:
                puts("Unknown receipt status");
                break;
        }

        if (status->front_exit_jam)
                puts("Jam at front exit");
        else
                puts("No jam at front exit");

        if (status->retract_exit_jam)
                puts("Jam at retract exit");
        else
                puts("No jam at retract exit");
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
        int port_type;
        unsigned char buf[4];
        int bufsize;
        aps_status_t status;

        port = NULL;

        atexit(clean);

        if (argc<2) {
                printf("getstatus compiled with APS library %d.%d.%d\n",
                                APS_MAJOR,
                                APS_MINOR,
                                APS_BUGFIX);

                printf("usage: getstatus [-t mrs|hrs|kcp|hsp] uri\n");
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

        /*setup timeouts*/
        check(aps_set_write_timeout(port,1000));
        check(aps_set_read_timeout(port,1000));

        /*get printer status*/
        check(port_type = aps_get_port_type(port));
        
        if (port_type==APS_USB) {
                aps_usb_ctrltransfer_t ctrl;

                memset(&ctrl,0,sizeof(ctrl));

                switch (opt_type) {
                case APS_MRS:
                        fprintf(stderr,"main: USB status not implemented for MRS printers\n");
                        exit(1);
                        break;
                case APS_HRS:
                        ctrl.bRequestType = 0xc1;
                        ctrl.bRequest = 1;
                        ctrl.wValue = 0;
                        ctrl.wIndex = 0;
                        ctrl.wLength = 1;
                        ctrl.data = buf;
                        break;
                case APS_KCP:
                        ctrl.bRequestType = 0xc1;
                        ctrl.bRequest = 1;
                        ctrl.wValue = 0;
                        ctrl.wIndex = 0;
                        ctrl.wLength = 3;
                        ctrl.data = buf;
                        break;
                case APS_HSP:
                        ctrl.bRequestType = 0xc1;
                        ctrl.bRequest = 0;
                        ctrl.wValue = 0;
                        ctrl.wIndex = 0;
                        ctrl.wLength = 4;
                        ctrl.data = buf;
                        break;
                }

                bufsize = ctrl.wLength;

                check(aps_usb_control(port,&ctrl));
        }
        else {
                const unsigned char *cmd;
                int cmdsize;

                switch (opt_type) {
                case APS_MRS:
                case APS_HRS:
                        cmd = cmd_get_status_aps;
                        cmdsize = sizeof(cmd_get_status_aps);
                        bufsize = 1;
                        break;
                case APS_KCP:
                        cmd = cmd_get_status_aps;
                        cmdsize = sizeof(cmd_get_status_aps);
                        bufsize = 3;
                        break;
                case APS_HSP:
                        fprintf(stderr,"main: command not implemented for HSP printers\n");
                        exit(1);
                        break;
                }
                
                check(aps_write(port,cmd,cmdsize));

                check(aps_read(port,buf,bufsize));
        }

        check(aps_decode_status(opt_type,buf,bufsize,&status));

        /*retrieve NEOP status separately for MRS/HRS/KCP printers*/
        switch (opt_type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                check(aps_write(port,cmd_get_neop_status_aps,sizeof(cmd_get_neop_status_aps)));

                check(aps_read(port,buf,1));

                if (buf[0]==0) {
                        status.near_end_of_paper = 0;
                }
                else if (buf[0]==1) {
                        status.near_end_of_paper = 1;
                }
                else {
                        fprintf(stderr,"main: invalid NEOP status\n");
                        exit(1);
                }

                break;
        case APS_HSP:
                /*NEOP status already retrieved*/
                break;
        }

        print_status(&status);

        /*close port*/
        check(aps_close(port));

        /*destroy port structure*/
        check(aps_destroy_port(port));
        
        port = NULL;

        return 0;
}

