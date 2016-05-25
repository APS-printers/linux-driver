/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : detect.c
* DESCRIPTION   : APS library - printers autodetection
* CVS           : $Id: detect.c,v 1.13 2009/01/16 16:14:34 pierre Exp $
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
*   09feb2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

#define MAX_SERIAL_PORTS        4
#define MAX_PARALLEL_PORTS      4
#define MAX_USB_PORTS           4

#define CMD_BUFSIZE             4       /*characters*/

typedef struct {
        struct {
                int size;
                unsigned char buf[CMD_BUFSIZE];
        } can;
        struct {
                int size;
                unsigned char buf[CMD_BUFSIZE];
        } get_status;
        struct {
                int size;
                unsigned char buf[CMD_BUFSIZE];
        } get_identity;
} detect_commands_t;

/*APS commands set: MRS, HRS, KCP*/
static const detect_commands_t cmd_aps = {
        {1, {CAN}},
        {2, {ESC, 'v'}},
        {2, {ESC, 'I'}}
};

/*ESC/POS commands set: HSP*/
static const detect_commands_t cmd_escpos = {
        {1, {CAN}},
        {3, {GS, 'I', 1}},
        {3, {GS, 'I', 'C'}}
};

static  int     detect_model(aps_port_t *,char *,int,const detect_commands_t *);
static  int     detect_serial_handshake(aps_port_t *,int);
static  int     detect_serial(aps_printer_t *,int);
static  int     detect_parallel_irq(int);
static  int     detect_parallel(aps_printer_t *,int);
static  int     detect_usb(aps_printer_t *,int);
        
/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  detect_model
Purpose   :  Try to detect printer model behind port
Inputs    :  p        : port structure
             identity : identity string
             size     : identity string size (includes trailing zero)
             cmd      : commands used for detection
Outputs   :  <>
Return    :  model number or error code
-----------------------------------------------------------------------------*/
static int detect_model(aps_port_t *p,char *identity,int size,const detect_commands_t *cmd)
{
        aps_error_t errnum;
        char c;
        const model_t *model;


        if ((errnum = aps_flush(p))<0) {
                return errnum;
        }

        if ((errnum = aps_write(p,cmd->can.buf,cmd->can.size))<0) {
                return errnum;
        }
        if ((errnum = aps_sync(p))<0) {
                return errnum;
        }
        if ((errnum = aps_flush(p))<0) {
                return errnum;
        }

        if ((errnum = aps_set_read_timeout(p,100))<0) {
                return errnum;
        }

        if ((errnum = aps_write(p,cmd->get_status.buf,cmd->get_status.size))<0) {
                return errnum;
        }
        if ((errnum = aps_sync(p))<0) {
                return errnum;
        }
        if ((errnum = aps_read(p,&c,1))<0) {
                return errnum;
        }

        /*read and discard two extra bytes (KCP returns 3 status bytes)*/
        if ((errnum = aps_set_read_timeout(p,10))<0) {
                return errnum;
        }
        aps_read(p,&c,1);
        aps_read(p,&c,1);


        if ((errnum = aps_write(p,cmd->can.buf,cmd->can.size))<0) {
                return errnum;
        }
        if ((errnum = aps_sync(p))<0) {
                return errnum;
        }
        if ((errnum = aps_flush(p))<0) {
                return errnum;
        }

        if ((errnum = aps_set_read_timeout(p,500))<0) {
                return errnum;
        }
        if ((errnum = aps_write(p,cmd->get_identity.buf,cmd->get_identity.size))<0) {
                return errnum;
        }
        if ((errnum = aps_sync(p))<0) {
                return errnum;
        }
        if ((errnum = aps_gets(p,identity,size))<0) {
                return errnum;
        }
	else
	{
		printf("printer returned model string: %s\n", identity);
	}


        /*identify model based on identity string*/
        model = model_find_by_id(identity);

        if (model==NULL) {
                return MODEL_UNKNOWN;
        }
        else {
                return model->model;
        }
}

/*-----------------------------------------------------------------------------
Name      :  detect_serial_handshake
Purpose   :  Detect serial handshake mode
Inputs    :  p     : port structure
             model : detected printer model
Outputs   :  <>
Return    :  handshake mode or error code
-----------------------------------------------------------------------------*/
static int detect_serial_handshake(aps_port_t *p,int model)
{
        aps_error_t errnum;
        int type;
        int handshake;

        if ((errnum = aps_get_model_type(model))<0) {
                return errnum;
        }
        else {
                type = errnum;
        }

        if (type==APS_MRS) {

                /* trailing CAN command to clear print buffer on firmware
                 * that do not support reading parameters from FLASH
                 */

                unsigned char buf[] = {GS,'S','f',28,CAN};
                unsigned char set;
               
                if ((errnum = aps_flush(p))<0) {
                        return errnum;
                }
                if ((errnum = aps_set_read_timeout(p,100))<0) {
                        return errnum;
                }
                if ((errnum = aps_write(p,buf,sizeof(buf)))<0) {
                        return errnum;
                }
                if ((errnum = aps_sync(p))<0) {
                        return errnum;
                }

                errnum = aps_read(p,&set,1);

                /* MRS firmwares type 1.37 do not reply to this command
                 * MRS firmwares before 5.55 return zero (default value)
                 * In these cases we revert to default serial hardware handshake mode
                 *
                 * FIXME: printers with firmware greater than or equal to 5.55
                 *  configured at 1200 bauds, software handshaking are
                 *  incorrectly detected as 'hardware handshake mode'
                 */

                if (errnum<0 || set==0) {
                        handshake = APS_RTSCTS;
                }
                else {
                        if (set&0x80) {
                                handshake = APS_RTSCTS;
                        }
                        else {
                                handshake = APS_XONXOFF;
                        }
                }

                /*clear text buffer in case command is not implemented*/
                if ((errnum = aps_write(p,cmd_aps.can.buf,cmd_aps.can.size))<0) {
                        return errnum;
                }
        }
        else if (type==APS_HRS || type==APS_KCP) {
                unsigned char buf[] = {ESC, GS, 'B'};
                unsigned char set;
               
                if ((errnum = aps_flush(p))<0) {
                        return errnum;
                }
                if ((errnum = aps_set_read_timeout(p,100))<0) {
                        return errnum;
                }
                if ((errnum = aps_write(p,buf,sizeof(buf)))<0) {
                        return errnum;
                }
                if ((errnum = aps_sync(p))<0) {
                        return errnum;
                }
                if ((errnum = aps_read(p,&set,1))<0) {
                        return errnum;
                }

                if (set&0x80) {
                        handshake = APS_RTSCTS;
                }
                else {
                        handshake = APS_XONXOFF;
                }
        }
        else {
                handshake = APS_RTSCTS;
        }

        return handshake;
}

/*-----------------------------------------------------------------------------
Name      :  detect_serial
Purpose   :  Detect serial printers
Inputs    :  printers : printers array
             max      : printers array size
Outputs   :  Fills printers array with model and port information
Return    :  number of printers detected
-----------------------------------------------------------------------------*/
static int detect_serial(aps_printer_t *printers,int max)
{
        aps_port_t *p[MAX_SERIAL_PORTS];
        aps_error_t errnum;
        char device[DEVICE_MAX+1];
        char identity[APS_IDENTITY_MAX+1];
        char uri[APS_URI_MAX+1];
        int baudrate;
        int handshake;
        int model;
        int i,n;

        /*create serial ports and wake-up printers*/
        for (i=0; i<MAX_SERIAL_PORTS; i++) {
                snprintf(device,sizeof(device),"/dev/ttyS%d",i);
               
                p[i] = aps_create_serial_port(device);

                if (p[i]==NULL) {
                        errnum = APS_INVALID_PORT;
                }
                else {
                        errnum = aps_get_error(p[i]);
                }

                if (errnum==APS_OK) {
                        errnum = aps_open(p[i]);
                }
                if (errnum==APS_OK) {
                        errnum = aps_serial_set_baudrate(p[i],APS_B1200);
                }
                if (errnum==APS_OK) {
                        errnum = aps_serial_set_handshake(p[i],APS_NONE);
                }
                if (errnum==APS_OK) {
                        unsigned char c = NUL;
                        errnum = aps_write(p[i],&c,1);  /*wake-up*/
                }
        }

        /*wait for printers to wake up*/
        sleep(2);

        /*detect printers*/
        n = 0;
       
        for (i=0; i<MAX_SERIAL_PORTS && n<max; i++) {
                if (p[i]==NULL) {
                        errnum = APS_INVALID_PORT;
                }
                else {
                        errnum = APS_OK;
                }

                /*TODO: check if printer is connected*/
                
                if (errnum==APS_OK) {
                        errnum = aps_set_write_timeout(p[i],100);
                }

                if (errnum==APS_OK) {
                        model = MODEL_INVALID;

                        for (baudrate=APS_B115200; baudrate>=APS_B1200; baudrate--) {
                                if ((errnum = aps_serial_set_baudrate(p[i],baudrate))<0) {
                                        break;
                                }
                                if ((errnum = aps_serial_set_handshake(p[i],APS_NONE))<0) {
                                        break;
                                }
				memset(identity, 0, sizeof identity);
                                if (model<0) {
                                        model = detect_model(p[i],identity,sizeof(identity),&cmd_aps);
                                }
				memset(identity, 0, sizeof identity);
                                if (model<0) {
                                        model = detect_model(p[i],identity,sizeof(identity),&cmd_escpos);
                                }

                                if (model>=0) {
                                        errnum = detect_serial_handshake(p[i],model);

                                        if (errnum>=0) {
                                                handshake = errnum;

                                                errnum = aps_serial_set_handshake(p[i],handshake);
                                        }

                                        break;
                                }
                               
                        }
                }

                if (errnum==APS_OK && model>=0) {
                        if (aps_get_port_uri(p[i],uri,sizeof(uri))>=0) {
                                printers->model = model;
                                strcpy(printers->identity,identity);
                                strcpy(printers->uri,uri);
                                printers++;
                                n++;
                        }
                }
        }
        
        /*close and destroy ports*/
        for (i=0; i<MAX_SERIAL_PORTS; i++) {
                if (p[i]!=NULL) {
                        aps_flush(p[i]);
                        aps_close(p[i]);
                        aps_destroy_port(p[i]);
                        p[i] = NULL;
                }
        }

        return n;
}

/*-----------------------------------------------------------------------------
Name      :  detect_parallel_irq
Purpose   :  Detect whether parallel port uses an IRQ line or not
Inputs    :  n : parallel port number
Outputs   :  <>
Return    :  1 if parallel uses IRQ, 0 if parallel uses polling
-----------------------------------------------------------------------------*/
static int detect_parallel_irq(int n)
{
        char s[256];
        FILE *f;
        int irq;

        /*try opening procfs file*/
        snprintf(s,sizeof(s),"/proc/sys/dev/parport/parport%d/irq",n);

        f = fopen(s,"r");

        if (f==NULL) {
                return 0;
        }

        if (fscanf(f,"%d",&irq)!=1) {
                irq = -1;
        }

        fclose(f);

        /*if IRQ is -1 then no IRQ is used*/
        if (irq==-1) {
                return 0;
        }
        else {
                return 1;
        }
}

/*-----------------------------------------------------------------------------
Name      :  detect_parallel
Purpose   :  Detect parallel printers
Inputs    :  printers : printers array
             max      : printers array size
Outputs   :  Fills printers array with model and port information
Return    :  number of printers detected
-----------------------------------------------------------------------------*/
static int detect_parallel(aps_printer_t *printers,int max)
{
        aps_port_t *p[MAX_PARALLEL_PORTS];
        aps_error_t errnum;
        char device[DEVICE_MAX+1];
        char identity[APS_IDENTITY_MAX+1];
        char uri[APS_URI_MAX+1];
        int model;
        int i,n;

        /*create parallel ports and wake-up printers*/
        for (i=0; i<MAX_PARALLEL_PORTS; i++) {
                snprintf(device,sizeof(device),"/dev/parport%d",i);
               
                p[i] = aps_create_parallel_port(device);

                if (p[i]==NULL) {
                        errnum = APS_INVALID_PORT;
                }
                else {
                        errnum = aps_get_error(p[i]);
                }

                if (errnum==APS_OK) {
                        /*detect parallel port mode*/
                        if (detect_parallel_irq(i)) {
                                errnum = aps_parallel_set_mode(p[i],APS_IRQ);
                        }
                        else {
                                errnum = aps_parallel_set_mode(p[i],APS_POLL);
                        }
                }

                if (errnum==APS_OK) {
                        errnum = aps_open(p[i]);
                }
                if (errnum==APS_OK) {
                        errnum = aps_parallel_reset(p[i]);
                }
        }

        /*wait for printers to wake up*/
        sleep(2);

        /*detect printers*/
        n = 0;
       
        for (i=0; i<MAX_PARALLEL_PORTS && n<max; i++) {
                if (p[i]==NULL) {
                        errnum = APS_INVALID_PORT;
                }
                else {
                        errnum = APS_OK;
                }

                /*TODO: check if printer is connected*/
                
                if (errnum==APS_OK) {
                        errnum = aps_set_write_timeout(p[i],100);
                }

                if (errnum==APS_OK) {
                        model = MODEL_INVALID;

                        if (model<0) {
                                model = detect_model(p[i],identity,sizeof(identity),&cmd_aps);
                        }
                        if (model<0) {
                                model = detect_model(p[i],identity,sizeof(identity),&cmd_escpos);
                        }
                }

                if (errnum==APS_OK && model>=0) {
                        if (aps_get_port_uri(p[i],uri,sizeof(uri))>=0) {
                                printers->model = model;
                                strcpy(printers->identity,identity);
                                strcpy(printers->uri,uri);
                                printers++;
                                n++;
                        }
                }
        }
        
        /*close and destroy ports*/
        for (i=0; i<MAX_PARALLEL_PORTS; i++) {
                if (p[i]!=NULL) {
                        aps_flush(p[i]);
                        aps_close(p[i]);
                        aps_destroy_port(p[i]);
                        p[i] = NULL;
                }
        }

        return n;
}

/*-----------------------------------------------------------------------------
Name      :  detect_usb
Purpose   :  Detect USB printers
Inputs    :  printers : printers array
             max      : printers array size
Outputs   :  Fills printers array with model and port information
Return    :  number of printers detected
-----------------------------------------------------------------------------*/
static int detect_usb(aps_printer_t *printers,int max)
{
        aps_port_t *p[MAX_USB_PORTS];
        aps_error_t errnum;
        char identity[APS_IDENTITY_MAX+1];
        char uri[APS_URI_MAX+1];
        int i,n;
        int model;
        int detected;

        /*list and open USB ports*/
        n = usb_list_ports(p,MAX_USB_PORTS);

        for (i=0; i<n; i++) {
                aps_open(p[i]);
        }

        /*detect printers*/
        detected = 0;
       
        for (i=0; i<n && detected<max; i++) {
                errnum = APS_OK;

#if 1
                {
                    char buf[256];
                    usb_get_uri(p[i],buf,256);
                    printf("DEBUG: port:%s\n",buf);
                }
#endif

                if (errnum==APS_OK) {
                        errnum = aps_set_write_timeout(p[i],100);
                }

                if (errnum==APS_OK) {
                        model = MODEL_INVALID;

                        if (model<0) {
                                model = detect_model(p[i],identity,sizeof(identity),&cmd_aps);
                        }
                        if (model<0) {
                                model = detect_model(p[i],identity,sizeof(identity),&cmd_escpos);
                        }
                }
#if 1
                {
                    printf("DEBUG: model:%d\n",model);
                }
#endif

                if (errnum==APS_OK && model>=0) {
                        if (aps_get_port_uri(p[i],uri,sizeof(uri))>=0) {
                                printers->model = model;
                                strcpy(printers->identity,identity);
                                strcpy(printers->uri,uri);
                                printers++;
                                detected++;
                        }
                }
        }
        
        /*close and destroy ports*/
        for (i=0; i<n; i++) {
                aps_flush(p[i]);
                aps_close(p[i]);
                aps_destroy_port(p[i]);
        }

        return detected;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  aps_detect_printers
Purpose   :  Detect printers connected to system
Inputs    :  printers : printers array
             max      : printers array size
Outputs   :  Fills printers array with model and port information
Return    :  number of printers detected
-----------------------------------------------------------------------------*/
int aps_detect_printers(aps_printer_t *printers,int max)
{
        int i;
        int n = 0;
       
        memset(printers,0,max*sizeof(aps_printer_t));

        /*detect serial printers*/
        i = detect_serial(&printers[n],max);
        n += i;
        max -= i;

        /*detect parallel printers*/
        i = detect_parallel(&printers[n],max);
        n += i;
        max -= i;

        /*detect USB printers*/
        i = detect_usb(&printers[n],max);
        n += i;
        max -= i;

        return n;
}

