/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : aps.c
 * DESCRIPTION   : APS library - exported functions
 * CVS           : $Id: aps.c,v 1.19 2009/01/16 16:14:34 pierre Exp $
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
 *   12apr2006   nico    Added parallel port specific functions
 *   10apr2008   nico    Added aps_usb_kill() function
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  build_port
Purpose   :  Build port structure
Inputs    :  <>
Outputs   :  <>
Return    :  port structure or NULL on error
-----------------------------------------------------------------------------*/
static aps_class_t *build_port(void)
{
    aps_class_t *p;

    /*allocate memory*/
    p = malloc(sizeof(aps_class_t));
    if (p==NULL)
        return NULL;

    /*zero out structure*/
    memset(p,0,sizeof(aps_class_t));

    return p;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  aps_get_model_name
Purpose   :  Retrieve name of model
Inputs    :  model : model number
Outputs   :  <>
Return    :  printer model name or NULL if model is invalid
-----------------------------------------------------------------------------*/
const char *aps_get_model_name(int model)
{
    const model_t *db;

    db = model_find_by_number(model);

    if (db==NULL) {
        return NULL;
    }
    else {
        return db->name;
    }
}

/*-----------------------------------------------------------------------------
Name      :  aps_get_model_type
Purpose   :  Retrieve type of model
Inputs    :  model : model number
Outputs   :  <>
Return    :  printer model type (aps_model_type_t) or error code
-----------------------------------------------------------------------------*/
int aps_get_model_type(int model)
{
    const model_t *db;

    db = model_find_by_number(model);

    if (db==NULL) {
        return APS_INVALID_MODEL;
    }
    else {
        return db->type;
    }
}

/*-----------------------------------------------------------------------------
Name      :  aps_get_model_width
Purpose   :  Retrieve width of model TPH
Inputs    :  model : model number
Outputs   :  <>
Return    :  TPH width in pixels or error code
-----------------------------------------------------------------------------*/
int aps_get_model_width(int model)
{
    const model_t *db;

    db = model_find_by_number(model);

    if (db==NULL) {
        return APS_INVALID_MODEL;
    }
    else {
        return db->width;
    }
}

/*-----------------------------------------------------------------------------
Name      :  aps_decode_status
Purpose   :  Decode internal printer status into standard aps_status_t
Inputs    :  type   : model type
buf    : status buffer
size   : status buffer size in bytes
status : standard status structure
Outputs   :  Fills status buffer with status information
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_decode_status(int type,const void *buf,int size,aps_status_t *status)
{
    aps_error_t errnum;
    const unsigned char *bytes = buf;

    switch (type) {

        case APS_MRS:
        case APS_HRS:
            if (size==1) {
                memset(status,0,sizeof(aps_status_t));

                status->temp_error              = bytes[0]&0x01?1:0;
                status->head_up                 = bytes[0]&0x02?1:0;
                status->end_of_paper            = bytes[0]&0x04?1:0;
                status->supply_error            = bytes[0]&0x08?1:0;
                status->printing                = bytes[0]&0x10?1:0;
                status->online                  = bytes[0]&0x20?1:0;
                status->mark_error              = bytes[0]&0x40?1:0;
                status->cutter_error            = bytes[0]&0x80?0:1;

                errnum = APS_OK;
            }
            else {
                errnum = APS_INVALID_STATUS;
            }
            break;

        case APS_KCP:
            if (size==3) {
                memset(status,0,sizeof(aps_status_t));

                status->temp_error              = bytes[0]&0x01?1:0;
                status->head_up                 = bytes[0]&0x02?1:0;
                status->end_of_paper            = bytes[0]&0x04?1:0;
                status->supply_error            = bytes[0]&0x08?1:0;
                status->printing                = bytes[0]&0x10?1:0;
                status->online                  = bytes[0]&0x20?1:0;
                status->presenter_error         = bytes[0]&0x80?0:1;

                status->presenter_action        = bytes[1]&0x07;
                status->rcpt_front_exit         = bytes[1]&0x08?1:0;
                status->rcpt_retract_exit       = bytes[1]&0x10?1:0;
                status->rcpt_status             = (bytes[1]>>5)&0x03;

                status->front_exit_jam          = bytes[2]&0x01?1:0;
                status->retract_exit_jam        = bytes[2]&0x02?1:0;
                status->cover_open              = bytes[2]&0x04?1:0;
                status->mechanical_error        = bytes[2]&0x10?1:0;

                errnum = APS_OK;
            }
            else {
                errnum = APS_INVALID_STATUS;
            }
            break;

        case APS_HSP:
            if (size==4) {
                memset(status,0,sizeof(aps_status_t));

                status->printing                = bytes[0]&0x01?1:0;

                status->online                  = bytes[0]&0x40?0:1;

                status->end_of_paper            = bytes[1]&0x01?1:0;
                status->head_up                 = bytes[1]&0x02?1:0;
                status->cover_open              = bytes[1]&0x04?1:0;
                status->near_end_of_paper       = bytes[1]&0x08?1:0;

                status->cutter_error            = bytes[2]&0x01?1:0;
                status->supply_error            = bytes[2]&0x06?1:0;
                status->temp_error              = bytes[2]&0x08?1:0;

                errnum = APS_OK;
            }
            else {
                errnum = APS_INVALID_STATUS;
            }
            break;

        default:
            errnum = APS_INVALID_MODEL_TYPE;
            break;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_create_port
Purpose   :  Create port structure from URI
Inputs    :  uri : uniform resource identifier
Outputs   :  <>
Return    :  port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_port(const char *uri)
{
    aps_class_t *p;
    struct aps_uri su;
    const char *value;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    /*parse URI*/
    if (uri==NULL || uri_split(&su,uri)<0) {
        p->port.errnum = APS_INVALID_URI;
        return p;
    }

    /*get port type*/
    value = uri_get_opt(&su,"type");

    if (value==NULL) {
        p->port.errnum = APS_INVALID_URI;
        return p;
    }

    /*setup port settings according to type*/
    if (strcmp(value,"serial")==0) {
        serial_custom(p);
    }
    else if (strcmp(value,"parallel")==0) {
        par_custom(p);
    }
    else if (strcmp(value,"usb")==0) {
        usb_custom(p);
    }
    else if (strcmp(value,"ethernet")==0) {
        ethernet_custom(p);
    }
    else {
        p->port.errnum = APS_INVALID_PORT_TYPE;
    }
    if (p->create_from_uri != NULL)
        p->port.errnum = p->create_from_uri(&p->port,&su);

    return p;
}

/*-----------------------------------------------------------------------------
Name      :  aps_create_serial_port
Purpose   :  Create serial port structure
Inputs    :  device : device name
Outputs   :  <>
Return    :  serial port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_serial_port(const char *device)
{
    aps_class_t *p;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    /*setup port settings*/
    serial_custom(p);
    p->port.errnum = p->create(&p->port,device);

    return p;
}

/*-----------------------------------------------------------------------------
Name      :  aps_create_parallel_port
Purpose   :  Create parallel port structure
Inputs    :  device : device name
Outputs   :  <>
Return    :  parallel port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_parallel_port(const char *device)
{
    aps_class_t *p;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    

    /*setup port settings*/
    par_custom(p);
    p->port.errnum = p->create(&p->port,device);

    return p;
}

/*-----------------------------------------------------------------------------
Name      :  aps_create_ethernet_port
Purpose   :  Create ETHERNET port structure
Inputs    :  device : device name
Outputs   :  <>
Return    :  ETHERNET port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_ethernet_port(const char *device)
{
    aps_class_t *p;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    /*setup port settings*/
    ethernet_custom(p);
    p->port.errnum = p->create(&p->port,device);

    return p;
}
/*-----------------------------------------------------------------------------
Name      :  aps_create_usb_port
Purpose   :  Create USB port structure
Inputs    :  device : device name
Outputs   :  <>
Return    :  USB port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_usb_port(const char *device)
{
    aps_class_t *p;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    /*setup port settings*/
    usb_custom(p);
    p->port.errnum = p->create(&p->port,device);

    return p;
}

/*-----------------------------------------------------------------------------
Name      :  aps_create_usb_port_from_address
Purpose   :  Create USB port structure from device address
Inputs    :  usbfs  : usbfs mount point
busnum : bus number
devnum : device number
Outputs   :  <>
Return    :  USB port structure or NULL on error (check port error code)
-----------------------------------------------------------------------------*/
void *aps_create_usb_port_from_address(const char *usbfs,int busnum,int devnum)
{
    aps_class_t *p;

    /*build port structure*/
    p = build_port();

    if (p==NULL) {
        return NULL;
    }

    /*setup port settings*/
    usb_custom(p);
    p->port.errnum = usb_create_from_address(&p->port,usbfs,busnum,devnum);

    return p;
}

/*-----------------------------------------------------------------------------
Name      :  aps_destroy_port
Purpose   :  Destroy port structure
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_destroy_port(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.is_open) {
            errnum = aps_close(p);
        }
        else {
            errnum = APS_OK;
        }

        if (errnum==APS_OK) {
            free(p);
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_get_port_type
Purpose   :  Retrieve type of port
Inputs    :  port : port structure
Outputs   :  <>
Return    :  printer port type (aps_port_type_t) or error code
-----------------------------------------------------------------------------*/
int aps_get_port_type(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        errnum = p->port.type;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_get_port_uri
Purpose   :  Retrieve URI string of port
Inputs    :  port : port structure
uri  : URI string buffer
size : URI string buffer size (includes trailing zero)
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_get_port_uri(void *port,char *uri,int size)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if  (p->get_uri != NULL) {
            p->port.errnum = p->get_uri(&p->port,uri,size);
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_open
Purpose   :  Open port
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_open(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.is_open) {
            errnum = APS_PORT_ALREADY_OPEN;
        }
        else {
            if  (p->open_ != NULL) {
                errnum = p->open_(&p->port);
            }
            if (errnum==APS_OK) {
                p->port.is_open = 1;
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_close
Purpose   :  Close port
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_close(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->close != NULL) {
                errnum = p->close(&p->port);
            }

            if (errnum==APS_OK) {
                p->port.is_open = 0;
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_write
Purpose   :  Write data buffer to port
Inputs    :  port : port structure
buf  : data buffer
size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_write(void *port,const void *buf,int size)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->write != NULL) {
                errnum = p->write(&p->port,buf,size);
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_write_rt
Purpose   :  Write data buffer to port in real-time (ignore handshake signals)
Inputs    :  port : port structure

buf  : data buffer
size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_write_rt(void *port,const void *buf,int size)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->write_rt != NULL) {
                errnum = p->write_rt(&p->port,buf,size);
            }
            else if (p->write != NULL) {
                errnum = p->write(&p->port,buf,size);
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_read
Purpose   :  Read data buffer from port
Inputs    :  port : port structure
buf  : data buffer
size : data buffer size in bytes
Outputs   :  Fills data buffer
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_read(void *port,void *buf,int size)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->read != NULL) {
                errnum = p->read(&p->port,buf,size);
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_sync
Purpose   :  Block until all data pending in output buffer are transmitted
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_sync(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->sync != NULL) {
                errnum = p->sync(&p->port);
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_flush
Purpose   :  Clear input and output buffers
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_flush(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else {
            if  (p->flush != NULL) {
                errnum = p->flush(&p->port);
            }
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_gets
Purpose   :  Read a null-terminated string
Inputs    :  port : port structure
s    : string buffer
size : string buffer size (includes trailing zero)
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_gets(void *port,void *s,int size)
{
    aps_error_t errnum;
    unsigned char *buf = s;

    if (size==0) {
        errnum = APS_OK;
    }
    else {
        errnum = APS_OK;

        while (size>1) {
            if ((errnum = aps_read(port,buf,1))<0) {
                break;
            }
            else if (*buf==0) {
                break;
            }
            else {
                buf++;
                size--;
            }
        }

        if (errnum==APS_OK) {
            *buf = 0;
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_serial_set_baudrate
Purpose   :  Set port baudrate. Only available on serial type ports
Inputs    :  port     : port structure
baudrate : baudrate setting
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_serial_set_baudrate(void *port,int baudrate)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else if (p->port.type!=APS_SERIAL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = serial_set_baudrate(&p->port,baudrate);
        }

        p->port.errnum = errnum;
    }


    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_serial_set_handshake
Purpose   :  Set handshake mode. Only available on serial type ports
Inputs    :  port      : port structure
handshake : handshake mode
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_serial_set_handshake(void *port,int handshake)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else if (p->port.type!=APS_SERIAL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = serial_set_handshake(&p->port,handshake);
            
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_serial_get_baudrate
Purpose   :  Get port baudrate. Only available on serial type ports
Inputs    :  port : port structure
Outputs   :  <>
Return    :  baudrate (aps_baudrate_t) or error code
-----------------------------------------------------------------------------*/
int aps_serial_get_baudrate(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.type!=APS_SERIAL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = p->port.set.serial.baudrate;
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_serial_get_handshake
Purpose   :  Get handshake mode. Only available on serial type ports
Inputs    :  port : port structure
Outputs   :  <>
Return    :  handshake (aps_handshake_t) or error code
-----------------------------------------------------------------------------*/
int aps_serial_get_handshake(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.type!=APS_SERIAL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = p->port.set.serial.handshake;
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_parallel_reset
Purpose   :  Hardware reset printer. Only available on parallel type ports
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_parallel_reset(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else if (p->port.type!=APS_PARALLEL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = par_reset(&p->port);
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_parallel_set_mode
Purpose   :  Set parallel port mode. Only available on parallel type ports
Inputs    :  port : port structure
mode : mode setting
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_parallel_set_mode(void *port,int mode)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.type!=APS_PARALLEL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = par_set_mode(&p->port,mode);
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_parallel_get_mode
Purpose   :  Get parallel port mode. Only available on parallel type ports
Inputs    :  port : port structure
Outputs   :  <>
Return    :  mode (aps_parallel_mode_t) or error code
-----------------------------------------------------------------------------*/
int aps_parallel_get_mode(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (p->port.type!=APS_PARALLEL) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = p->port.set.par.mode;
        }
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_usb_control
Purpose   :  Perform a USB control request on port
Inputs    :  port : port structure
ctrl : control request
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_usb_control(void *port,aps_usb_ctrltransfer_t *ctrl)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else if (p->port.type!=APS_USB) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = usb_control(&p->port,ctrl);
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_usb_kill
Purpose   :  Kill USB printer port (force close without uninitialization)
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_usb_kill(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (p==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (!p->port.is_open) {
            errnum = APS_PORT_NOT_OPEN;
        }
        else if (p->port.type!=APS_USB) {
            errnum = APS_INVALID_PORT_TYPE;
        }
        else {
            errnum = usb_kill(&p->port);
        }

        if (errnum==APS_OK) {
            p->port.is_open = 0;
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_set_write_timeout
Purpose   :  Set write timeout in milliseconds
Inputs    :  port : port structure
ms   : timeout in milliseconds
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_set_write_timeout(void *port,int ms)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (port==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (ms<0) {
            errnum = APS_INVALID_TIMEOUT;
        }
        else {
            p->port.write_timeout = ms;
            errnum = APS_OK;
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_set_read_timeout
Purpose   :  Set read timeout in milliseconds
Inputs    :  port : port structure
ms   : timeout in milliseconds
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int aps_set_read_timeout(void *port,int ms)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (port==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        if (ms<0) {
            errnum = APS_INVALID_TIMEOUT;
        }
        else {
            p->port.read_timeout = ms;
            errnum = APS_OK;
        }

        p->port.errnum = errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_get_sub_error
Purpose   :  Get last sub error on port
Inputs    :  port : port structure
Outputs   :  <>
Return    :  error code of last error
-----------------------------------------------------------------------------*/
int     aps_get_sub_error(void *port)
{
    int sub_errnum;
    aps_class_t *p = port;

    if (port==NULL) {
        sub_errnum = 0;
    }
    else {
        sub_errnum = p->port.sub_errnum;
    }

    return sub_errnum;
}
/*-----------------------------------------------------------------------------
Name      :  aps_get_error
Purpose   :  Get last error on port
Inputs    :  port : port structure
Outputs   :  <>
Return    :  error code of last error
-----------------------------------------------------------------------------*/
int aps_get_error(void *port)
{
    aps_error_t errnum;
    aps_class_t *p = port;

    if (port==NULL) {
        errnum = APS_INVALID_PORT;
    }
    else {
        errnum = p->port.errnum;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  aps_strerror
Purpose   :  Convert error code in printable string
Inputs    :  errnum : error code
Outputs   :  <>
Return    :  printable string
-----------------------------------------------------------------------------*/
const char *aps_strerror(int errnum)
{
    return aps_get_strerror_full(errnum,NULL);
}


/*-----------------------------------------------------------------------------
Name      :  aps_get_strerror_full
Purpose   :  Convert error code in printable string
             and get the extended error if port is not null
Inputs    :  errnum : error code
             port   : port structure can be null if not created
Outputs   :  <>
Return    :  printable string
-----------------------------------------------------------------------------*/
const char *aps_get_strerror_full(int errnum,void* port)
{
    const char *s;
    int sub_errnum = 0;
    
    if (port != NULL)
    {
        sub_errnum = ((aps_class_t*)port)->port.sub_errnum;
    }

    switch (errnum) {
        case APS_OK:
            s = "Success";
            break;
        case APS_NOT_IMPLEMENTED:
            s = "Function not implemented";
            break;
        case APS_IO_ERROR:
            s = "Low-level input/output error";
            break;
        case APS_INVALID_MODEL:
            s = "Invalid model parameter";
            break;
        case APS_INVALID_MODEL_TYPE:
            s = "Invalid model type parameter";
            break;
        case APS_INVALID_PORT:
            s = "Invalid port parameter";
            break;
        case APS_INVALID_PORT_TYPE:
            s = "Invalid port type parameter";
            break;
        case APS_PORT_NAME_TOO_LONG:
            s = "Port name too long";
            break;
        case APS_NAME_TOO_LONG:
            s = "Name or string too long";
            break;
        case APS_INVALID_URI:
            s = "Invalid URI parameter";
            break;
        case APS_INVALID_BAUDRATE:
            s = "Invalid baudrate parameter";
            break;
        case APS_INVALID_HANDSHAKE:
            s = "Invalid handshake parameter";
            break;
        case APS_INVALID_TIMEOUT:
            s = "Invalid timeout parameter";
            break;
        case APS_OPEN_FAILED:
            s = "Open operation failed";
            break;
        case APS_CLOSE_FAILED:
            s = "Close operation failed";
            break;
        case APS_WRITE_FAILED:
            s = "Write operation failed";
            break;
        case APS_WRITE_TIMEOUT:
            s = "Write operation timed out";
            break;
        case APS_READ_FAILED:
            s = "Read operation failed";
            break;
        case APS_READ_TIMEOUT:
            s = "Read operation timed out";
            break;
        case APS_SYNC_FAILED:
            s = "Sync operation failed";
            break;
        case APS_FLUSH_FAILED:
            s = "Flush operation failed";
            break;
        case APS_INVALID_STATUS:
            s = "Invalid status buffer";
            break;
        case APS_PORT_NOT_OPEN:
            s = "Port is not open";
            break;
        case APS_PORT_ALREADY_OPEN:
            s = "Port is already open";
            break;
        case APS_INVALID_PARALLEL_MODE:
            s = "Invalid parallel mode parameter";
            break;
        case APS_INVALID_USB_PATH:
            s = "Invalid USB path";
            break;
        case APS_USB_DEVICE_NOT_FOUND:
            s = "USB device not found";
            break;
        case APS_USB_DEVICE_BUSY:
            s = "USB device busy (cannot unregister current driver)";
            break;
        case APS_ETHERNET_EAI_ERROR:
            if (sub_errnum)
                s = gai_strerror(sub_errnum);
            else
                s = "Internal Ethernet error";
        case APS_ETHERNET_SOCKET_ERROR:
            switch(sub_errnum)
            {
                case EACCES:
                    s = "Ethernet socket: Permission to create a socket of the specified type and/or protocol is denied.";
                    break;
                case EAFNOSUPPORT: 
                    s = "Ethernet socket: The implementation does not support the specified address family.";
                    break;
                case EINVAL:
                    s = "Ethernet socket: Unknown protocol, or protocol family not available.";
                    break;
                case EMFILE:
                    s = "Ethernet socket: Process file table overflow.";
                    break;
                case ENFILE:
                    s = "Ethernet socket: The system limit on the total number of open files has been reached.";
                    break;
                case ENOBUFS:
                case ENOMEM:
                    s = "Ethernet socket: Insufficient memory is available. The socket cannot be created until sufficient resources are freed.";
                    break;
                case EPROTONOSUPPORT:
                    s = "Ethernet socket: The protocol type or the specified protocol is not supported within this domain.";
                    break;
                default:
                    s = "Ethernet socket: Unknown error";
                    break;
            }
            break;
        case APS_ETHERNET_FCNTL_ERROR:
            s = "fcntl.";
            break;
        case APS_ETHERNET_CONNECT_ERROR:
            switch(sub_errnum)
            {
                case EACCES:
                    s = "Ethernet connect: For Unix domain sockets, which are identified by pathname: Write permission is denied on the socket file, or search permission is denied for one of the directories in the path prefix. (See also path_resolution(2).)";
                    break; 
                case EPERM:
                    s = "Ethernet connect: The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the connection request failed because of a local firewall rule.";
                    break; 
                case EADDRINUSE:
                    s = "Ethernet connect: Local address is already in use.";
                    break; 
                case EAFNOSUPPORT:
                    s = "Ethernet connect: The passed address didn't have the correct address family in its sa_family field.";
                    break; 
                case EAGAIN:
                    s = "Ethernet connect: No more free local ports or insufficient entries in the routing cache. For PF_INET see the net.ipv4.ip_local_port_range sysctl in ip(7) on how to increase the number of local ports.";
                    break; 
                case EALREADY:
                    s = "Ethernet connect: The socket is non-blocking and a previous connection attempt has not yet been completed.";
                    break; 
                case EBADF:
                    s = "Ethernet connect: The file descriptor is not a valid index in the descriptor table.";
                    break; 
                case ECONNREFUSED:
                    s = "Ethernet connect: No one listening on the remote address.";
                    break; 
                case EFAULT:
                    s = "Ethernet connect: The socket structure address is outside the user's address space.";
                    break; 
                case EINPROGRESS:
                    s = "Ethernet connect: The socket is non-blocking and the connection cannot be completed immediately. It is possible to select(2) or poll(2) for completion by selecting the socket for writing. After select(2) indicates writability, use getsockopt(2) to read the SO_ERROR option at level SOL_SOCKET to determine whether connect() completed successfully (SO_ERROR is zero) or unsuccessfully (SO_ERROR is one of the usual error codes listed here, explaining the reason for the failure).";
                    break; 
                case EINTR:
                    s = "Ethernet connect: The system call was interrupted by a signal that was caught.";
                    break; 
                case EISCONN:
                    s = "Ethernet connect: The socket is already connected.";
                    break; 
                case ENETUNREACH:
                    s = "Ethernet connect: Network is unreachable.";
                    break; 
                case ENOTSOCK:
                    s = "Ethernet connect: The file descriptor is not associated with a socket.";
                    break; 
                case ETIMEDOUT:
                    s = "Ethernet connect: Timeout while attempting connection. The server may be too busy to accept new connections. Note that for IP sockets the timeout may be very long when syncookies are enabled on the server";
                    break;
                default:
                        s = "Ethernet connect: Unknown error";
                        break;
            }
            break;

        default:
            s = "Unknown error";
            break;
    }

    return s;
}

