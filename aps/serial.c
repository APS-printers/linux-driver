/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : serial.c
* DESCRIPTION   : APS library - serial communication routines
* CVS           : $Id: serial.c,v 1.20 2009/01/16 16:14:34 pierre Exp $
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
*   17jul2006   nico    Linux kernel version is now automatic
*   31jan2007   nico    Removed automatic modification of transmit FIFO size
*                       (now  handled directly by CUPS driver)
*   06apr2007   nico    Added support for custom HSP baudrates
*   27feb2008	nico	Added description of required kernel hacks to handle
*   			custom HSP baudrates
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <linux/serial.h>
#include <linux/version.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/*enable custom baudrates for HSP printer*/
#undef CONFIG_CUSTOM_BAUDRATES

/*Note: custom baudrate support requires kernel hacking!
 *
 *The procedure is briefly explained here and is only experimental.
 *If you are feeling adventurous however and would like to try high-speed
 *serial, please contact APS support for detailed instructions.
 *
 *Custom baudrates support requires several modifications:
 *
 * - Add custom baudrates definition in /usr/include/bits/termios.h
 *
 *   #define B125000 0010020
 *   #define B250000 0010021
 *   #define B312500 0010022
 *
 *   (this might need some adjustment - use the first free baudrate code)
 *
 * - Add same custom baudrates definition to the kernel source tree in
 *   linux/include/asm-i386/termbits.h (for i386 architecture)
 *
 * - Add custom baudrates to baud_table[] in
 *   /usr/src/linux/drivers/char/tty_ioctl.c. Append 12500, 250000, 312500
 *   at the end of the table
 *
 * - Add custom baud_bits[] in /usr/src/linux/drivers/char/tty_ioctl.c.
 *   Append B12500, B250000, B312500 at the end of the table
 *
 * - Recompile the kernel to enable support of the new custom baudrates
 */

#define SERIAL_DEFBAUDRATE      APS_B9600
#define SERIAL_DEFHANDSHAKE     APS_RTSCTS

static  void    serial_timer(int);

static  const char *    baudrate_to_string(int);
static  const char *    handshake_to_string(int);

static  int     string_to_baudrate(const char *);
static  int     string_to_handshake(const char *);

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  serial_timer
Purpose   :  Custom signal handler for SIGALRM signals
Inputs    :  signum : signal number
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void serial_timer(int signum)
{
        /* this function traps SIGALRM signals
         * this function does nothing
         */
        (void)signum;
}

/*-----------------------------------------------------------------------------
Name      :  baudrate_to_string
Purpose   :  Convert baudrate parameter to string
Inputs    :  baudrate : baudrate setting
Outputs   :  <>
Return    :  string representation of baudrate or "unknown"
-----------------------------------------------------------------------------*/
static const char *baudrate_to_string(int baudrate)
{
        const char *s;
        
        switch (baudrate) {
        case APS_B1200:
                s = "1200";
                break;
        case APS_B2400:
                s = "2400";
                break;
        case APS_B4800:
                s = "4800";
                break;
        case APS_B9600:
                s = "9600";
                break;
        case APS_B19200:
                s = "19200";
                break;
        case APS_B38400:
                s = "38400";
                break;
        case APS_B57600:
                s = "57600";
                break;
        case APS_B115200:
                s = "115200";
                break;

#ifdef CONFIG_CUSTOM_BAUDRATES
                
        case APS_B125000:
                s = "125000";
                break;
        case APS_B250000:
                s = "250000";
                break;
        case APS_B312500:
                s = "312500";
                break;

#endif /*CONFIG_CUSTOM_BAUDRATES*/
                
        default:
                s = "unknown";
                break;
        }

        return s;
}

/*-----------------------------------------------------------------------------
Name      :  string_to_baudrate
Purpose   :  Convert string to baudrate parameter
Inputs    :  s : string
Outputs   :  <>
Return    :  baudrate parameter (aps_baudrate_t) or error code
-----------------------------------------------------------------------------*/
static int string_to_baudrate(const char *s)
{
        int baudrate;

        if (strcmp(s,"1200")==0) {
                baudrate = APS_B1200;
        }
        else if (strcmp(s,"2400")==0) {
                baudrate = APS_B2400;
        }
        else if (strcmp(s,"4800")==0) {
                baudrate = APS_B4800;
        }
        else if (strcmp(s,"9600")==0) {
                baudrate = APS_B9600;
        }
        else if (strcmp(s,"19200")==0) {
                baudrate = APS_B19200;
        }
        else if (strcmp(s,"38400")==0) {
                baudrate = APS_B38400;
        }
        else if (strcmp(s,"57600")==0) {
                baudrate = APS_B57600;
        }
        else if (strcmp(s,"115200")==0) {
                baudrate = APS_B115200;
        }

#ifdef CONFIG_CUSTOM_BAUDRATES

        else if (strcmp(s,"125000")==0) {
                baudrate = APS_B125000;
        }
        else if (strcmp(s,"250000")==0) {
                baudrate = APS_B250000;
        }
        else if (strcmp(s,"312500")==0) {
                baudrate = APS_B312500;
        }

#endif /*CONFIG_CUSTOM_BAUDRATES*/

        else {
                baudrate = APS_INVALID_BAUDRATE;
        }
        
        return baudrate;
}

/*-----------------------------------------------------------------------------
Name      :  handshake_to_string
Purpose   :  Convert handshake parameter to string
Inputs    :  handshake : handshake setting
Outputs   :  <>
Return    :  string representation of handshake or "unknown"
-----------------------------------------------------------------------------*/
static const char *handshake_to_string(int handshake)
{
        const char *s;

        switch (handshake) {
        case APS_NONE:
                s = "none";
                break;
        case APS_RTSCTS:
                s = "rtscts";
                break;
        case APS_XONXOFF:
                s = "xonxoff";
                break;
        default:
                s = "unknown";
                break;
        }

        return s;
}

/*-----------------------------------------------------------------------------
Name      :  string_to_handshake
Purpose   :  Convert string to handshake parameter
Inputs    :  s : string
Outputs   :  <>
Return    :  handshake parameter (aps_handshake_t) or error code
-----------------------------------------------------------------------------*/
static int string_to_handshake(const char *s)
{
        int handshake;

        if (strcmp(s,"none")==0) {
                handshake = APS_NONE;
        }
        else if (strcmp(s,"rtscts")==0) {
                handshake = APS_RTSCTS;
        }
        else if (strcmp(s,"xonxoff")==0) {
                handshake = APS_XONXOFF;
        }
        else {
                handshake = APS_INVALID_HANDSHAKE;
        }
        
        return handshake;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

void serial_custom(aps_class_t *p)
{
    if (p == NULL)
        return;

    p->port.type=APS_SERIAL;

    p->get_uri = serial_get_uri;
    p->create = serial_create;
    p->create_from_uri = serial_create_from_uri;
    

    p->open_ = serial_open;
    p->close = serial_close;

    p->write = serial_write;
    p->write_rt = serial_write_rt;
    p->read = serial_read;

    p->sync = serial_sync;
    p->flush = serial_flush;


}



/*-----------------------------------------------------------------------------
Name      :  serial_get_uri
Purpose   :  Get URI string defining serial port
Inputs    :  p    : port structure
             uri  : URI string buffer
             size : URI string buffer size (includes trailing zero)
Outputs   :  Fills URI string buffer
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_get_uri(aps_port_t *p,char *uri,int size)
{
        aps_error_t errnum;
        int len;

        len = snprintf(uri,size,"aps:%s?type=serial+baudrate=%s+handshake=%s",
                        p->set.serial.device,
                        baudrate_to_string(p->set.serial.baudrate),
                        handshake_to_string(p->set.serial.handshake));

        if (len>=size) {
                errnum = APS_INVALID_URI;
        }
        else {
                errnum = APS_OK;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  serial_create
Purpose   :  Create serial port from device. Initialize settings to defaults
Inputs    :  p      : port structure
             device : device name
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_create(aps_port_t *p,const char *device)
{
        /*copy device name*/
        if (strlen(device)>sizeof(p->set.serial.device)-1) {
                return APS_NAME_TOO_LONG;
        }

        strcpy(p->set.serial.device,device);

        /*initialize default settings*/
        p->set.serial.baudrate = SERIAL_DEFBAUDRATE;
        p->set.serial.handshake = SERIAL_DEFHANDSHAKE;

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  serial_create_from_uri
Purpose   :  Create serial port from URI string
Inputs    :  p   : port structure
             uri : URI string
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_create_from_uri(aps_port_t *p,struct aps_uri *su)
{
        aps_error_t errnum;
        const char *device;
        const char *value;

        /*set device parameter*/
        device = uri_get_device(su);

        if (device==NULL) {
                return APS_INVALID_URI;
        }
        if ((errnum = serial_create(p,device))<0) {
                return errnum;
        }

        /*set baudrate parameter (if it exists)*/
        value = uri_get_opt(su,"baudrate");

        if (value!=NULL) {
                if ((errnum = string_to_baudrate(value))<0) {
                        return errnum;
                }
                else {
                        p->set.serial.baudrate = errnum;
                }
        }
        
        /*set handshake parameter (if it exists)*/
        value = uri_get_opt(su,"handshake");

        if (value!=NULL) {
                if ((errnum = string_to_handshake(value))<0) {
                        return errnum;
                }
                else {
                        p->set.serial.handshake = errnum;
                }
        }

        return APS_OK;
}


/*-----------------------------------------------------------------------------
Name      :  serial_open
Purpose   :  Open serial port
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_open(aps_port_t *p)
{
    int fd;
    aps_error_t errnum;
    struct termios set;
    struct utsname buf;
    int a,b,c;

    /*retrieve Linux kernel version*/
    memset(&buf,0,sizeof(buf));
    uname(&buf);

    if (sscanf(buf.release,"%d.%d.%d",&a,&b,&c)!=3) {
        return APS_IO_ERROR;
    }

    /*open device
     * O_RDWR       open for read and write operations
     * O_NOCTTY     program does not want to be controlling terminal
     * O_NONBLOCK   use non-blocking system calls
     * O_EXCL       open in exclusive mode
     */

    if ((fd = open(p->set.serial.device,O_RDWR|O_NOCTTY|O_NONBLOCK|O_EXCL))<0) {
        return APS_OPEN_FAILED;
    }

    p->set.serial.fd = fd;

    /*setup default serial settings*/
    if (tcgetattr(fd,&set)<0) {
        return APS_IO_ERROR;
    }

    cfmakeraw(&set);

    /*set 8 bits, no parity, enable receiver*/
    set.c_cflag &= ~PARENB;
    set.c_cflag &= ~CSTOPB;
    set.c_cflag &= ~CSIZE;
    set.c_cflag |= CS8;
    set.c_cflag |= CLOCAL|CREAD;

    if (tcsetattr(fd,TCSANOW,&set)<0) {
        return APS_IO_ERROR;
    }

    if ((errnum = serial_set_baudrate(p,p->set.serial.baudrate))<0) {
        return errnum;
    }

    if ((errnum = serial_set_handshake(p,p->set.serial.handshake))<0) {
        return errnum;
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  serial_close
Purpose   :  Close serial port
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_close(aps_port_t *p)
{
        aps_error_t errnum;

        /*close device*/
        if (close(p->set.serial.fd)<0) {
                errnum = APS_CLOSE_FAILED;
        }
        else {
                errnum = APS_OK;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  serial_set_baudrate
Purpose   :  Set serial port baudrate. Port structure is not updated
Inputs    :  p        : port structure
             baudrate : baudrate setting
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_set_baudrate(aps_port_t *p,int baudrate)
{
        struct termios set;

        if (tcgetattr(p->set.serial.fd,&set)<0) {
                return APS_IO_ERROR;
        }

        switch (baudrate) {
        case APS_B1200:
                cfsetispeed(&set,B1200);
                cfsetospeed(&set,B1200);
                break;
        case APS_B2400:
                cfsetispeed(&set,B2400);
                cfsetospeed(&set,B2400);
                break;
        case APS_B4800:
                cfsetispeed(&set,B4800);
                cfsetospeed(&set,B4800);
                break;
        case APS_B9600:
                cfsetispeed(&set,B9600);
                cfsetospeed(&set,B9600);
                break;
        case APS_B19200:
                cfsetispeed(&set,B19200);
                cfsetospeed(&set,B19200);
                break;
        case APS_B38400:
                cfsetispeed(&set,B38400);
                cfsetospeed(&set,B38400);
                break;
        case APS_B57600:
                cfsetispeed(&set,B57600);
                cfsetospeed(&set,B57600);
                break;
        case APS_B115200:
                cfsetispeed(&set,B115200);
                cfsetospeed(&set,B115200);
                break;

#ifdef CONFIG_CUSTOM_BAUDRATES

        case APS_B125000:
                cfsetispeed(&set,B125000);
                cfsetospeed(&set,B125000);
                break;
        case APS_B250000:
                cfsetispeed(&set,B250000);
                cfsetospeed(&set,B250000);
                break;
        case APS_B312500:
                cfsetispeed(&set,B312500);
                cfsetospeed(&set,B312500);
                break;

#endif /*CONFIG_CUSTOM_BAUDRATES*/
                
        default:
                return APS_INVALID_BAUDRATE;
        }
        
        if (tcsetattr(p->set.serial.fd,TCSANOW,&set)<0) {
                return APS_IO_ERROR;
        }

        p->set.serial.baudrate = baudrate;
        return APS_OK;
 }

/*-----------------------------------------------------------------------------
Name      :  serial_set_handshake
Purpose   :  Set serial handshake mode. Port structure is not updated
Inputs    :  p         : port structure
             handshake : handshake mode
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_set_handshake(aps_port_t *p,int handshake)
{
        struct termios set;

        if (tcgetattr(p->set.serial.fd,&set)<0) {
                return APS_IO_ERROR;
        }

        switch (handshake) {
        case APS_NONE:
                /*disable flow control*/
                set.c_cflag &= ~CRTSCTS;
                set.c_iflag &= ~(IXON|IXOFF|IXANY);
                break;
        case APS_XONXOFF:
                /*software flow control*/
                set.c_cflag &= ~CRTSCTS;
                set.c_iflag |= IXON|IXOFF|IXANY;
                break;
        case APS_RTSCTS:
                /*hardware flow control*/
                set.c_cflag |= CRTSCTS;
                set.c_iflag &= ~(IXON|IXOFF|IXANY);
                break;
        default:
                return APS_INVALID_HANDSHAKE;
        }

        if (tcsetattr(p->set.serial.fd,TCSANOW,&set)<0) {
                return APS_IO_ERROR;
        }

        p->set.serial.handshake = handshake;

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  serial_write
Purpose   :  Write data buffer to serial port
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_write(aps_port_t *p,const void *buf,int size)
{
        aps_error_t errnum = APS_OK;

        while (size) {
                fd_set fds;
                int n;

                /*reset file descriptors set at each iteration*/
                /*this is required because select() modifies it*/
                /*Erik de Castro Lopo (bCODE)*/
                FD_ZERO(&fds);
                FD_SET(p->set.serial.fd,&fds);

                /*wait until some room is available in kernel write buffer*/
                if (p->write_timeout==0) {
                        n = select(p->set.serial.fd+1,NULL,&fds,NULL,NULL);
                }
                else {
                        struct timeval tv;

                        tv.tv_sec = p->write_timeout/1000;
                        tv.tv_usec = (p->write_timeout%1000)*1000;

                        n = select(p->set.serial.fd+1,NULL,&fds,NULL,&tv);
                }

                if (n<0) {
                        errnum = APS_WRITE_FAILED;
                        break;
                }
                else if (n==0) {
                        errnum = APS_WRITE_TIMEOUT;
                        break;
                }

                /*write some bytes*/
                n = write(p->set.serial.fd,buf,size);

                if (n<0) {
                        errnum = APS_WRITE_FAILED;
                        break;
                }
                else {
                        buf += n;
                        size -= n;
                }
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  serial_write_rt
Purpose   :  Write data buffer to serial port with handshaking disabled
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_write_rt(aps_port_t *p,const void *buf,int size)
{
/* shopov - 22042010 - add saving/restoring of the handshake type,
 * without this the first time this routine is executed, the
 * handshake is being set to APS_NONE and the printing gets broken */
int saved_handshake;

        aps_error_t errnum;
        
        /*flush pending input and output data*/
        if ((errnum = serial_flush(p))<0) {
                return errnum;
        }

	saved_handshake = p->set.serial.handshake;

        /*disable handshaking*/
        if ((errnum = serial_set_handshake(p,APS_NONE))<0) {
		p->set.serial.handshake = saved_handshake;
                return errnum;
        }
	p->set.serial.handshake = saved_handshake;

        /*write data*/
        /*transmission starts immediatly because we flushed output buffer*/
        if ((errnum = serial_write(p,buf,size))<0) {
                return errnum;
        }

        /*wait until data is completely transmitted*/
        if ((errnum = serial_sync(p))<0) {
                return errnum;
        }

        /*enable handshaking*/
        if ((errnum = serial_set_handshake(p,p->set.serial.handshake))<0) {
                return errnum;
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  serial_read
Purpose   :  Read data buffer from serial port
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  Fills data buffer
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_read(aps_port_t *p,void *buf,int size)
{
        aps_error_t errnum = APS_OK;

        while (size) {
                fd_set fds;
                int n;

                /*reset file descriptors set at each iteration*/
                /*this is required because select() modifies it*/
                /*Erik de Castro Lopo (bCODE)*/
                FD_ZERO(&fds);
                FD_SET(p->set.serial.fd,&fds);

                /*wait until some bytes are available in kernel read buffer*/
                if (p->read_timeout==0) {
                        n = select(p->set.serial.fd+1,&fds,NULL,NULL,NULL);
                }
                else {
                        struct timeval tv;

                        tv.tv_sec = p->read_timeout/1000;
                        tv.tv_usec = (p->read_timeout%1000)*1000;

                        n = select(p->set.serial.fd+1,&fds,NULL,NULL,&tv);
                }

                if (n<0) {
                        errnum = APS_READ_FAILED;
                        break;
                }
                else if (n==0) {
                        errnum = APS_READ_TIMEOUT;
                        break;
                }

                /*read some bytes*/
                n = read(p->set.serial.fd,buf,size);

                if (n<0) {
                        errnum = APS_READ_FAILED;
                        break;
                }
                else {
                        buf += n;
                        size -= n;
                }
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  serial_sync
Purpose   :  Block until all data pending in output buffer are transmitted
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_sync(aps_port_t *p)
{
        aps_error_t errnum;
        struct sigaction sa,old_sa;
        struct itimerval timer;

        if (p->write_timeout!=0) {
                /*install signal handler*/
                memset(&sa,0,sizeof(sa));
                sa.sa_handler = &serial_timer;
                sigaction(SIGALRM,&sa,&old_sa);

                /*start timer*/
                timer.it_interval.tv_sec = 0;
                timer.it_interval.tv_usec = 0;
                timer.it_value.tv_sec = p->write_timeout/1000;
                timer.it_value.tv_usec = (p->write_timeout%1000)*1000;

                setitimer(ITIMER_REAL,&timer,NULL);
        }

        if (tcdrain(p->set.serial.fd)<0) {
                if (errno==EINTR) {
                        errnum = APS_WRITE_TIMEOUT;
                }
                else {
                        errnum = APS_SYNC_FAILED;
                }
        }
        else {
                errnum = APS_OK;
        }

        if (p->write_timeout!=0) {
                /*stop timer*/
                memset(&timer,0,sizeof(timer));
                setitimer(ITIMER_REAL,&timer,NULL);

                /*uninstall signal handler*/
                sigaction(SIGALRM,&old_sa,NULL);
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  serial_flush
Purpose   :  Clear input and output buffers
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int serial_flush(aps_port_t *p)
{
        aps_error_t errnum;

        if (tcflush(p->set.serial.fd,TCIOFLUSH)<0) {
                errnum = APS_FLUSH_FAILED;
        }
        else {
                errnum = APS_OK;
        }

        return errnum;
}

