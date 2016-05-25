/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : parallel.c
* DESCRIPTION   : APS library - parallel communication routines
* CVS           : $Id: parallel.c,v 1.12 2007/12/10 15:14:35 nicolas Exp $
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
*   10mar2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/*timeout flag*/
static  sig_atomic_t    par_timeout;

/*old SIGALRM signal handler*/
static  struct sigaction        par_old_sa;

static  void    par_timer(int);
static  void    par_start_timer(int);
static  void    par_stop_timer(void);

static  int     par_clear_irq(aps_port_t *);
static  int     par_wait_irq(aps_port_t *);

static  int     par_control_idle(aps_port_t *);
static  int     par_control_set(aps_port_t *,unsigned char);
static  int     par_control_clear(aps_port_t *,unsigned char);
static  int     par_get_status(aps_port_t *,unsigned char *);

static  int     par_write_byte(aps_port_t *,const void *);
static  int     par_read_byte(aps_port_t *,void *);

static  const char *    mode_to_string(int);

static  int     string_to_mode(const char *);

/* PARPORT_STATUS_BUSY is active low
 * PARPORT_STATUS_ACK is active low
 * PARPORT_CONTROL_STROBE is active high
 * PARPORT_CONTROL_AUTOFD is active high
 * PARPORT_CONTROL_INIT is active low
 */

#define ASSERT_STROBE(p)        par_control_set(p,PARPORT_CONTROL_STROBE)
#define DEASSERT_STROBE(p)      par_control_clear(p,PARPORT_CONTROL_STROBE)
#define ASSERT_AUTOFD(p)        par_control_set(p,PARPORT_CONTROL_AUTOFD)
#define DEASSERT_AUTOFD(p)      par_control_clear(p,PARPORT_CONTROL_AUTOFD)
#define ASSERT_INIT(p)          par_control_clear(p,PARPORT_CONTROL_INIT)
#define DEASSERT_INIT(p)        par_control_set(p,PARPORT_CONTROL_INIT)

#define IS_BUSY_ASSERTED(s)     (((s)&PARPORT_STATUS_BUSY)==0)
#define IS_BUSY_DEASSERTED(s)   (((s)&PARPORT_STATUS_BUSY)!=0)
#define IS_ACK_ASSERTED(s)      (((s)&PARPORT_STATUS_ACK)==0)
#define IS_ACK_DEASSERTED(s)    (((s)&PARPORT_STATUS_ACK)!=0)

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  par_timer
Purpose   :  Custom signal handler for SIGALRM signals
Inputs    :  signum : signal number
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void par_timer(int signum)
{
        (void)signum;

        par_timeout = 1;
}

/*-----------------------------------------------------------------------------
Name      :  par_start_timer
Purpose   :  Start operation timer
Inputs    :  timeout : timeout in milliseconds
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void par_start_timer(int timeout)
{
        struct sigaction sa;
        struct itimerval timer;
        
        /*clear timeout flag*/
        par_timeout = 0;

        /*install signal handler*/
        memset(&sa,0,sizeof(sa));
        sa.sa_handler = &par_timer;
        sigaction(SIGALRM,&sa,&par_old_sa);

        /*start timer*/
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        timer.it_value.tv_sec = timeout/1000;
        timer.it_value.tv_usec = (timeout%1000)*1000;

        setitimer(ITIMER_REAL,&timer,NULL);
}

/*-----------------------------------------------------------------------------
Name      :  par_stop_timer
Purpose   :  Stop operation timer
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void par_stop_timer(void)
{
        struct itimerval timer;

        /*stop timer*/
        memset(&timer,0,sizeof(timer));
        setitimer(ITIMER_REAL,&timer,NULL);

        /*uninstall signal handler*/
        sigaction(SIGALRM,&par_old_sa,NULL);
}

/*-----------------------------------------------------------------------------
Name      :  par_clear_irq
Purpose   :  Clear parallel port IRQ counter
Inputs    :  p : port structure
Outputs   :  Updates port IRQ counter
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_clear_irq(aps_port_t *p)
{
        int irq_count;

        if (ioctl(p->set.par.fd,PPCLRIRQ,&irq_count)<0) {
                return APS_IO_ERROR;
        }

        p->set.par.irq_left = 0;

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_wait_irq
Purpose   :  Wait until a parallel port IRQ is detected or timeout
             This indicates that some bytes have been correctly transmitted
Inputs    :  p : port structure
Outputs   :  Updates port IRQ counter
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_wait_irq(aps_port_t *p)
{
        fd_set fds;
        int irq_count;

        /*if no IRQ are left, there is nothing to wait for*/
        if (p->set.par.irq_left==0) {
                return APS_OK;
        }

        FD_ZERO(&fds);
        FD_SET(p->set.par.fd,&fds);

        /*wait IRQ*/
        if (select(p->set.par.fd+1,&fds,NULL,NULL,NULL)<0) {
                if (par_timeout) {
                        return APS_WRITE_TIMEOUT;
                }
                else {
                        return APS_IO_ERROR;
                }
        }
        
        /*update IRQ counter*/
        if (ioctl(p->set.par.fd,PPCLRIRQ,&irq_count)<0) {
                return APS_IO_ERROR;
        }

        if (p->set.par.irq_left<irq_count) {
                /*FIXME: this might be an error*/
                p->set.par.irq_left = 0;
        }
        else {
                p->set.par.irq_left -= irq_count;
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_control_set
Purpose   :  Set one or more parallel port control signals
Inputs    :  p    : port structure
             mask : signals mask (PARPORT_CONTROL_xxx)
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_control_set(aps_port_t *p,unsigned char mask)
{
        struct ppdev_frob_struct fs;

        fs.mask = mask;
        fs.val = mask;

        if (ioctl(p->set.par.fd,PPFCONTROL,&fs)<0) {
                return APS_IO_ERROR;
        }
        else {
                return APS_OK;
        }
}

/*-----------------------------------------------------------------------------
Name      :  par_control_clear
Purpose   :  Clear one or more parallel port control signals
Inputs    :  p    : port structure
             mask : signals mask (PARPORT_CONTROL_xxx)
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_control_clear(aps_port_t *p,unsigned char mask)
{
        struct ppdev_frob_struct fs;

        fs.mask = mask;
        fs.val = 0;

        if (ioctl(p->set.par.fd,PPFCONTROL,&fs)<0) {
                return APS_IO_ERROR;
        }
        else {
                return APS_OK;
        }
}

/*-----------------------------------------------------------------------------
Name      :  par_control_idle
Purpose   :  Set parallel port control signals to idle state
Inputs    :  p    : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_control_idle(aps_port_t *p)
{
        aps_error_t errnum;

        if ((errnum = DEASSERT_INIT(p))<0) {
                return errnum;
        }
        if ((errnum = DEASSERT_STROBE(p))<0) {
                return errnum;
        }
        if ((errnum = DEASSERT_AUTOFD(p))<0) {
                return errnum;
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_get_status
Purpose   :  Retrieve parallel port status
Inputs    :  p      : port structure
             status : status byte
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_get_status(aps_port_t *p,unsigned char *status)
{
        if (ioctl(p->set.par.fd,PPRSTATUS,status)<0) {
                return APS_IO_ERROR;
        }
        else {
                return APS_OK;
        }
}

/*-----------------------------------------------------------------------------
Name      :  par_write_byte
Purpose   :  Write one byte to parallel port using APS printers timing
             Parallel port drivers must be set as output
Inputs    :  p   : port structure
             buf : byte buffer
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_write_byte(aps_port_t *p,const void *buf)
{
        aps_error_t errnum;
        unsigned char status;

        /*wait until BUSY is deasserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_BUSY_DEASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_WRITE_TIMEOUT;
                }
        }

        /*write data*/
        if (ioctl(p->set.par.fd,PPWDATA,buf)<0) {
                return APS_IO_ERROR;
        }

        /*assert STROBE*/
        if ((errnum = ASSERT_STROBE(p))<0) {
                return errnum;
        }
        
        /*wait until BUSY is asserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_BUSY_ASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_WRITE_TIMEOUT;
                }
        }

        /*deassert STROBE*/
        if ((errnum = DEASSERT_STROBE(p))<0) {
                return errnum;
        }
        
        /* If the host is slow or if we get preempted during the following
         * busy loop, we might not detect falling edge of the ACK signal.
         * We skip this part because it is non crucial, since the fact
         * that BUSY deasserts also indicates that write operation is over,
         * and detecting this is not time dependent.
         */

#if 0
        /*wait until ACK is asserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_ACK_ASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_WRITE_TIMEOUT;
                }
        }
#endif

        /*wait until ACK and BUSY are deasserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_ACK_DEASSERTED(status) || !IS_BUSY_DEASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_WRITE_TIMEOUT;
                }
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_read_byte
Purpose   :  Read one byte from parallel port using APS printers timing
             Parallel port drivers must be set as input
Inputs    :  p   : port structure
             buf : byte buffer
Outputs   :  Byte read is written in buf
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int par_read_byte(aps_port_t *p,void *buf)
{
        aps_error_t errnum;
        unsigned char status;

        /*wait until BUSY is deasserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_BUSY_DEASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_READ_TIMEOUT;
                }
        }

        /*assert AUTOFEED*/
        if ((errnum = ASSERT_AUTOFD(p))<0) {
                return errnum;
        }

        /*wait until ACK is asserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_ACK_ASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_READ_TIMEOUT;
                }
        }

        /*read data*/
        if (ioctl(p->set.par.fd,PPRDATA,buf)<0) {
                return APS_IO_ERROR;
        }

        /*deassert AUTOFEED*/
        if ((errnum = DEASSERT_AUTOFD(p))<0) {
                return errnum;
        }

        /*assert STROBE*/
        if ((errnum = ASSERT_STROBE(p))<0) {
                return errnum;
        }

        /*wait until ACK is deasserted*/
        if ((errnum = par_get_status(p,&status))<0) {
                return errnum;
        }

        while (!IS_ACK_DEASSERTED(status)) {
                if ((errnum = par_get_status(p,&status))<0) {
                        return errnum;
                }
                if (par_timeout) {
                        return APS_READ_TIMEOUT;
                }
        }

        /*deassert STROBE*/
        if ((errnum = DEASSERT_STROBE(p))<0) {
                return errnum;
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  mode_to_string
Purpose   :  Convert mode parameter to string
Inputs    :  mode : mode setting
Outputs   :  <>
Return    :  string representation of mode or "unknown"
-----------------------------------------------------------------------------*/
static const char *mode_to_string(int mode)
{
        const char *s;

        switch (mode) {
        case APS_POLL:
                s = "poll";
                break;
        case APS_IRQ:
                s = "irq";
                break;
        default:
                s = "unknown";
                break;
        }

        return s;
}

/*-----------------------------------------------------------------------------
Name      :  string_to_mode
Purpose   :  Convert string to mode parameter
Inputs    :  s : string
Outputs   :  <>
Return    :  mode parameter (aps_parallel_mode_t) or error code
-----------------------------------------------------------------------------*/
static int string_to_mode(const char *s)
{
        int mode;

        if (strcmp(s,"poll")==0) {
                mode = APS_POLL;
        }
        else if (strcmp(s,"irq")==0) {
                mode = APS_IRQ;
        }
        else {
                mode = APS_INVALID_PARALLEL_MODE;
        }

        return mode;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

void par_custom(aps_class_t *p)
{
    if (p == NULL)
        return;

    p->port.type = APS_PARALLEL;

    p->get_uri = par_get_uri;
    
    p->create = par_create;
    p->create_from_uri = par_create_from_uri;


    p->open_ = par_open;
    p->close = par_close;

    p->write = par_write;
    p->read = par_read;

    p->sync = par_sync;
    p->flush = par_flush;

}

/*-----------------------------------------------------------------------------
Name      :  par_get_uri
Purpose   :  Get URI string defining parallel port
Inputs    :  p    : port structure
             uri  : URI string buffer
             size : URI string buffer size (includes trailing zero)
Outputs   :  Fills URI string buffer
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_get_uri(aps_port_t *p,char *uri,int size)
{
        aps_error_t errnum;
        int len;

        len = snprintf(uri,size,"aps:%s?type=parallel+mode=%s",
                        p->set.par.device,
                        mode_to_string(p->set.par.mode));

        if (len>=size) {
                errnum = APS_INVALID_URI;
        }
        else {
                errnum = APS_OK;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_create
Purpose   :  Create parallel port from device. Initialize settings to defaults
Inputs    :  p      : port structure
             device : device name
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_create(aps_port_t *p,const char *device)
{
        /*copy device name*/
        if (strlen(device)>sizeof(p->set.par.device)-1) {
                return APS_NAME_TOO_LONG;
        }

        strcpy(p->set.par.device,device);

        /*initialize default settings*/
        p->set.par.mode = APS_POLL;

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_create_from_uri
Purpose   :  Create parallel port from URI string
Inputs    :  p   : port structure
             uri : URI string
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_create_from_uri(aps_port_t *p,struct aps_uri *su)
{
        aps_error_t errnum;
        const char *device;
        const char *value;

        /*set device parameter*/
        device = uri_get_device(su);

        if (device==NULL) {
                return APS_INVALID_URI;
        }
        if ((errnum = par_create(p,device))<0) {
                return errnum;
        }

        /*set poll parameter (if it exists)*/
        value = uri_get_opt(su,"mode");

        if (value!=NULL) {
                if ((errnum = string_to_mode(value))<0) {
                        return errnum;
                }
                else {
                        p->set.par.mode = errnum;
                }
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_open
Purpose   :  Open parallel port
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_open(aps_port_t *p)
{
        aps_error_t errnum;
        int fd;
        int mode;

        /*open device
         * O_RDWR       open for read and write operations
         * O_NONBLOCK   use non-blocking system calls
         * O_EXCL       open in exclusive mode
         */
        if ((fd = open(p->set.par.device,O_RDWR|O_NONBLOCK|O_EXCL))<0) {
                return APS_OPEN_FAILED;
        }

        p->set.par.fd = fd;

        /*register device behind parallel port*/
        if (ioctl(fd,PPCLAIM)<0) {
                return APS_IO_ERROR;
        }

        /*set compatiblity mode*/
        mode = IEEE1284_MODE_COMPAT;

        if (ioctl(fd,PPSETMODE,&mode)<0) {
                return APS_IO_ERROR;
        }

        /*clear interrupt count*/
        errnum = par_clear_irq(p);

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_close
Purpose   :  Close parallel port
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_close(aps_port_t *p)
{
        /*unregister device*/
        if (ioctl(p->set.par.fd,PPRELEASE)<0) {
                return APS_IO_ERROR;
        }

        /*close device*/
        if (close(p->set.par.fd)<0) {
                return APS_CLOSE_FAILED;
        }

        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_reset
Purpose   :  Reset printer
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_reset(aps_port_t *p)
{
        aps_error_t errnum;
        struct timespec ts;

        if ((errnum = par_control_idle(p))<0) {
                return errnum;
        }

        if ((errnum = ASSERT_INIT(p))<0) {
                return errnum;
        }

        ts.tv_sec = 0;
        ts.tv_nsec = 100000000;         /*100ms*/
        nanosleep(&ts,NULL);
        
        if ((errnum = DEASSERT_INIT(p))<0) {
                return errnum;
        }

        ts.tv_sec = 0;
        ts.tv_nsec = 100000000;         /*100ms*/
        nanosleep(&ts,NULL);
        
        return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  par_set_mode
Purpose   :  Set parallel port mode
Inputs    :  p    : port structure
             mode : mode setting
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_set_mode(aps_port_t *p,int mode)
{
        aps_error_t errnum;
        
        if (mode==APS_POLL || mode==APS_IRQ) {
                p->set.par.mode = mode;
                errnum = APS_OK;
        }
        else {
                errnum = APS_INVALID_PARALLEL_MODE;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_write
Purpose   :  Write data buffer to parallel port
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_write(aps_port_t *p,const void *buf,int size)
{
        aps_error_t errnum = APS_OK;

        if (p->write_timeout!=0) {
                par_start_timer(p->write_timeout);
        }
        else {
                par_timeout = 0;
        }

        if (p->set.par.mode==APS_POLL) {
                /*polling mode*/
                while (size) {
                        if ((errnum = par_write_byte(p,buf))<0) {
                                break;
                        }
                        else {
                                buf++;
                                size--;
                        }
                }

                if (errnum!=APS_OK) {
                        par_control_idle(p);
                }
        }
        else {
                /*interrupt mode*/
                while (size) {
                        int n;

                        /*wait for IRQ, freeing some space in kernel buffer*/
                        if ((errnum = par_wait_irq(p))<0) {
                                break;
                        }

                        /*write some bytes*/
                        n = write(p->set.par.fd,buf,size);

                        if (par_timeout) {
                                errnum = APS_WRITE_TIMEOUT;
                                break;
                        }
                        else if (n<0) {
                                errnum = APS_WRITE_FAILED;
                                break;
                        }
                        else {
                                buf += n;
                                size -= n;
                                p->set.par.irq_left += n;
                        }
                }
        }

        if (p->write_timeout!=0) {
                par_stop_timer();
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_write_rt
Purpose   :  Write data buffer to parallel port with handshaking disabled
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_write_rt(aps_port_t *p,const void *buf,int size)
{
        /*TODO: write true real-time write sequence*/
        return par_write(p,buf,size);
}

/*-----------------------------------------------------------------------------
Name      :  par_read
Purpose   :  Read data buffer from parallel port
Inputs    :  p    : port structure
             buf  : data buffer
             size : data buffer size in bytes
Outputs   :  Fills data buffer
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_read(aps_port_t *p,void *buf,int size)
{
        aps_error_t errnum = APS_OK;
        int dir;

        /*make sure that all data has been transferred before reading*/
        if ((errnum = par_sync(p))<0) {
                return errnum;
        }

        /*set parallel port drivers as input*/
        dir = 1;

        if (ioctl(p->set.par.fd,PPDATADIR,&dir)<0) {
                return APS_IO_ERROR;
        }

        if (p->read_timeout!=0) {
                par_start_timer(p->read_timeout);
        }
        else {
                par_timeout = 0;
        }
        
        while (size) {
                if ((errnum = par_read_byte(p,buf))<0) {
                        break;
                }
                else {
                        buf++;
                        size--;
                }
        }

        if (errnum!=APS_OK) {
                par_control_idle(p);
        }

        if (p->read_timeout!=0) {
                par_stop_timer();
        }
        
        /*set parallel port drivers as output*/
        dir = 0;

        if (ioctl(p->set.par.fd,PPDATADIR,&dir)<0) {
                return APS_IO_ERROR;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_sync
Purpose   :  Block until all data pending in output buffer are transmitted
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_sync(aps_port_t *p)
{
        aps_error_t errnum = APS_OK;

        if (p->set.par.mode==APS_POLL) {
                /*output buffer is always empty in polling mode*/
        }
        else {
                if (p->write_timeout!=0) {
                        par_start_timer(p->write_timeout);
                }
                else {
                        par_timeout = 0;
                }
                
                while (p->set.par.irq_left) {
                        if ((errnum = par_wait_irq(p))<0) {
                                break;
                        }
                }

                if (p->write_timeout!=0) {
                        par_stop_timer();
                }
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  par_flush
Purpose   :  Clear input and output buffers
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int par_flush(aps_port_t *p)
{
        aps_error_t errnum;

        if ((errnum = par_sync(p))<0) {
                return errnum;
        }

        if ((errnum = par_clear_irq(p))<0) {
                return errnum;
        }

        return APS_OK;
}

