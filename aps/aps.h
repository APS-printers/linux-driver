/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : aps.h
* DESCRIPTION   : APS library header file
* CVS           : $Id: aps.h,v 1.12 2008/04/10 12:53:04 nicolas Exp $
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
*   07feb2006   nico    Initial revision
******************************************************************************/

#ifndef _APS_H
#define _APS_H

#include <aps/version.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <aps/models.def>

typedef enum {
        APS_OK                          = 0,
        APS_NOT_IMPLEMENTED             = -1,
        APS_IO_ERROR                    = -2,
        APS_INVALID_MODEL               = -3,
        APS_INVALID_MODEL_TYPE          = -4,
        APS_INVALID_PORT                = -5,
        APS_INVALID_PORT_TYPE           = -6,
        APS_NAME_TOO_LONG               = -7,
        APS_INVALID_URI                 = -8,
        APS_INVALID_BAUDRATE            = -9,
        APS_INVALID_HANDSHAKE           = -10,
        APS_INVALID_TIMEOUT             = -11,
        APS_OPEN_FAILED                 = -12,
        APS_CLOSE_FAILED                = -13,
        APS_WRITE_FAILED                = -14,
        APS_WRITE_TIMEOUT               = -15,
        APS_READ_FAILED                 = -16,
        APS_READ_TIMEOUT                = -17,
        APS_SYNC_FAILED                 = -18,
        APS_FLUSH_FAILED                = -19,
        APS_INVALID_STATUS              = -20,
        APS_PORT_NOT_OPEN               = -21,
        APS_PORT_ALREADY_OPEN           = -22,
        APS_INVALID_PARALLEL_MODE       = -23,
        APS_INVALID_USB_PATH            = -24,
        APS_USB_DEVICE_NOT_FOUND        = -25,
        APS_USB_DEVICE_BUSY             = -26,
        APS_PORT_NAME_TOO_LONG          = -27,
        APS_ETHERNET_EAI_ERROR          = -28,
        APS_ETHERNET_SOCKET_ERROR       = -29,
        APS_ETHERNET_FCNTL_ERROR        = -30,
        APS_ETHERNET_CONNECT_ERROR      = -31,
        APS_OPEN_TIMEOUT                = -32


} aps_error_t;

typedef struct {
        char    printing;
        char    online;
        char    end_of_paper;
        char    near_end_of_paper;
        char    head_up;
        char    cover_open;
        char    temp_error;
        char    supply_error;
        char    mark_error;
        char    cutter_error;
        char    mechanical_error;
        char    presenter_error;
        char    presenter_action;
        char    rcpt_front_exit;
        char    rcpt_retract_exit;
        char    rcpt_status;
        char    front_exit_jam;
        char    retract_exit_jam;
} aps_status_t;

typedef enum {
        APS_PRESENTER_IDLE              = 0,
        APS_PRESENTER_PRESENTING        = 1,
        APS_PRESENTER_CUTTING           = 2,
        APS_PRESENTER_RETRACTING        = 3,
        APS_PRESENTER_REPRESENTING      = 4,
        APS_PRESENTER_JAM_CLEARING      = 5
} aps_presenter_action_t;

typedef enum {
        APS_RCPT_NOT_COLLECTED          = 0,
        APS_RCPT_COLLECTED              = 1,
        APS_RCPT_RETRACT_COMPLETE       = 2,
        APS_RCPT_RETRACT_PARTIAL        = 3
} aps_rcpt_status_t;

typedef enum {
        APS_UNKNOWN     = 0,
        APS_MRS         = 1,
        APS_HRS         = 2,
        APS_HSP         = 3,
        APS_KCP         = 4
} aps_model_type_t;

typedef enum {
        APS_SERIAL      = 0,
        APS_PARALLEL    = 1,
        APS_USB         = 2,
        APS_ETHERNET    = 3
} aps_port_type_t;

#define APS_IDENTITY_MAX        31      /*characters*/
#define APS_URI_MAX             255     /*characters*/

typedef struct {
        int     model;
        char    identity[APS_IDENTITY_MAX+1];
        char    uri[APS_URI_MAX+1];
} aps_printer_t;

typedef enum {
        APS_B1200       = 0,
        APS_B2400       = 1,
        APS_B4800       = 2,
        APS_B9600       = 3,
        APS_B19200      = 4,
        APS_B38400      = 5,
        APS_B57600      = 6,
        APS_B115200     = 7,
        APS_B125000     = 8,    /*custom baudrate - requires kernel hacking*/
        APS_B250000     = 9,    /*custom baudrate - requires kernel hacking*/
        APS_B312500     = 10    /*custom baudrate - requires kernel hacking*/
} aps_serial_baudrate_t;

typedef enum {
        APS_NONE        = 0,
        APS_XONXOFF     = 1,
        APS_RTSCTS      = 2
} aps_serial_handshake_t;

typedef enum {
        APS_POLL        = 0,
        APS_IRQ         = 1
} aps_parallel_mode_t;

typedef struct {
        unsigned char   bRequestType;
        unsigned char   bRequest;
        unsigned int    wValue;
        unsigned int    wIndex;
        unsigned int    wLength;
        void *          data;
} aps_usb_ctrltransfer_t;

const char *    aps_get_model_name(int model);

int     aps_get_model_type(int model);
int     aps_get_model_width(int model);

int     aps_decode_status(int type,const void *buf,int size,aps_status_t *status);

int     aps_detect_printers(aps_printer_t *printers,int max);

void *  aps_create_port(const char *uri);
void *  aps_create_serial_port(const char *device);
void *  aps_create_ethernet_port(const char *device);
void *  aps_create_parallel_port(const char *device);
void *  aps_create_usb_port(const char *device);
void *  aps_create_usb_port_from_address(const char *usbfs,int busnum,int devnum);

int     aps_destroy_port(void *port);

int     aps_get_port_type(void *port);
int     aps_get_port_uri(void *port,char *uri,int size);
        
int     aps_open(void *port);
int     aps_close(void *port);
int     aps_write(void *port,const void *buf,int size);
int     aps_write_rt(void *port,const void *buf,int size);
int     aps_read(void *port,void *buf,int size);
int     aps_sync(void *port);
int     aps_flush(void *port);

int     aps_gets(void *port,void *s,int size);

int     aps_serial_set_baudrate(void *port,int baudrate);
int     aps_serial_set_handshake(void *port,int handshake);
int     aps_serial_get_baudrate(void *port);
int     aps_serial_get_handshake(void *port);

int     aps_parallel_reset(void *port);
int     aps_parallel_set_mode(void *port,int mode);
int     aps_parallel_get_mode(void *port);

int     aps_usb_control(void *port,aps_usb_ctrltransfer_t *ctrl);
int     aps_usb_kill(void *port);

int     aps_set_write_timeout(void *port,int ms);
int     aps_set_read_timeout(void *port,int ms);

int     aps_get_error(void *port);
int     aps_get_sub_error(void *port);

const char *aps_strerror(int errnum);
const char *aps_get_strerror_full(int errnum,void *port);

#ifdef __cplusplus
}
#endif

#endif /*_APS_H*/

