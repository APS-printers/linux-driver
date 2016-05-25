/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : ethernet.c
 * DESCRIPTION   : APS library - ethernet communication routines
 * CVS           : $Id: usb.c,v 1.17 2008/04/17 16:05:27 nicolas Exp $
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
 *   23mars2009  pierre    Initial revision from a copy of usb.c
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <sys/stat.h>
//#include <sys/time.h>
#include <sys/types.h>
//#include <linux/version.h>
#include <netdb.h>
#include <sys/socket.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

#define ETHERNET_DEFSERVICE     "9100"


/* PRIVATE FUNCTIONS --------------------------------------------------------*/



/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

void ethernet_custom(aps_class_t *p)
{
    if (p == NULL)
        return;

    p->port.type=APS_ETHERNET;

    p->get_uri = ethernet_get_uri;

    p->create = ethernet_create;
    p->create_from_uri = ethernet_create_from_uri;

    p->open_ = ethernet_open;
    p->close = ethernet_close;

    p->write = ethernet_write;
    p->read = ethernet_read;

    p->sync = ethernet_sync;
    p->flush = ethernet_flush;

}
/*-----------------------------------------------------------------------------
 * Name      :  usb_get_uri
 * Purpose   :  Get URI string defining USB port
 * Inputs    :  p    : port structure
 *              uri  : URI string buffer
 *              size : URI string buffer size (includes trailing zero)
 * Outputs   :  Fills URI string buffer
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_get_uri(aps_port_t *p,char *uri,int size)
{
    aps_error_t errnum;
    int len;

    return APS_INVALID_URI;


    len = snprintf(uri,size,"aps:%s?type=ethernet+port=%s",
                   p->set.ethernet.node,
                   p->set.ethernet.service);

    if (len>=size) {
        errnum = APS_INVALID_URI;
    }
    else {
        errnum = APS_OK;
    }

    return errnum;
}

/*-----------------------------------------------------------------------------
 * Name      :  ethernet_create
 * Purpose   :  Create ETHERNET port from device. Initialize settings to defaults
 * Inputs    :  p      : port structure
 *              device : device name
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_create(aps_port_t *p,const char *device)
{
    /*copy device name*/
    if (strlen(device)>sizeof(p->set.ethernet.node)-1) {
        return APS_NAME_TOO_LONG;
    }

    strcpy(p->set.ethernet.node,device);

    /*initialize default settings*/
    strcpy(p->set.ethernet.service,ETHERNET_DEFSERVICE);

    return APS_OK;
}


/*-----------------------------------------------------------------------------
 * Name      :  ethernet_create_from_uri
 * Purpose   :  Create ETHERNET port from URI string
 * Inputs    :  p   : port structure
 *              uri : URI string
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_create_from_uri(aps_port_t *p,struct aps_uri *su)
{
    aps_error_t errnum;
    const char *node;
    const char *service;

    /*get device name*/
    node = uri_get_device(su);

    if (node==NULL) {
        return APS_INVALID_URI;
    }

    printf("Device node:%s\n",node);

    if ((errnum = ethernet_create(p,node))<0) {
        return errnum;
    }

    /*set baudrate parameter (if it exists)*/
    service = uri_get_opt(su,"port");

    if (service != NULL)
    {
        /*copy device name*/
        if (strlen(service)>sizeof(p->set.ethernet.service)-1) {
            return APS_PORT_NAME_TOO_LONG;
        }

        strcpy(p->set.ethernet.service,service);
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  ethernet_open
 * Purpose   :  Open ETHERNET port
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code and p->sub_errnum
 * -----------------------------------------------------------------------------*/
int ethernet_open(aps_port_t *p)
{
	int sockfd;
	struct addrinfo hints, *res;
	int eai;
	int errnum;

	errnum = APS_OK;

	memset(&hints,0,sizeof(hints));

	hints.ai_family = AF_UNSPEC; // use IPV4 or IPV6
	hints.ai_socktype = SOCK_STREAM;

	eai = getaddrinfo(p->set.ethernet.node,p->set.ethernet.service,&hints,&res);

	if (eai)
	{
		p->sub_errnum = eai;
		return APS_ETHERNET_EAI_ERROR;
	}

	if (errnum == APS_OK)
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0 )
		{
			errnum = APS_ETHERNET_SOCKET_ERROR;
		}
	}

	/*open device
	 * O_RDWR       open for read and write operations
	 * O_NOCTTY     program does not want to be controlling terminal
	 * O_NONBLOCK   use non-blocking system calls
	 * O_EXCL       open in exclusive mode
	 */

	if (errnum == APS_OK)
	{
		if (fcntl(sockfd,F_SETFL,/*O_RDWR|O_NOCTTY|*/O_NONBLOCK/*|O_EXCL*/) < 0)
		{
			freeaddrinfo(res);
			errno = APS_ETHERNET_FCNTL_ERROR;
		}
	}

	if (errnum == APS_OK)
	{
        int n;
		n = connect(sockfd, res->ai_addr, res->ai_addrlen);
        if (n < 0)
		{
			if (errno == EINPROGRESS)
            {
                fd_set fds;

                /*reset file descriptors set at each iteration*/
                /*this is required because select() modifies it*/
                /*Erik de Castro Lopo (bCODE)*/
                FD_ZERO(&fds);
                FD_SET(sockfd,&fds);

                if (p->write_timeout==0) {
                    p->write_timeout = 30000;
                }
                /*
                   if (p->write_timeout==0) {
                   n = select(sockfd+1,NULL,&fds,NULL,NULL);
                   }
                   else
                   */
                {
                    struct timeval tv;

                    tv.tv_sec = p->write_timeout/1000;
                    tv.tv_usec = (p->write_timeout%1000)*1000;

                    n = select(sockfd+1,NULL,&fds,NULL,&tv);
                }

                if (n < 0) {
                    errnum = APS_OPEN_FAILED;
                }
                else if (n == 0) {
                    errnum = APS_OPEN_TIMEOUT;
                }
            }
			else { 
				errnum = APS_ETHERNET_CONNECT_ERROR;
			} 
		}
	}

	if (errnum == APS_OK)
	{
		p->set.ethernet.sockfd = sockfd;
	}
	else
	{
		p->set.ethernet.sockfd = 0;
		p->sub_errnum = errno;
	}

	freeaddrinfo(res);
	return errnum;
}

/*-----------------------------------------------------------------------------
 * Name      :  ethernet_close
 * Purpose   :  Close ETHERNET port
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code and p->sub_errnum
 * -----------------------------------------------------------------------------*/
int ethernet_close(aps_port_t *p)
{
    aps_error_t errnum;

    /*close device*/
    if (close(p->set.ethernet.sockfd)<0) {
        errnum = APS_CLOSE_FAILED;
    }
    else {
        errnum = APS_OK;
    }

    return errnum;
}



/*-----------------------------------------------------------------------------
 * Name      :  ethernet_write
 * Purpose   :  Write data buffer to ETHERNET port
 * Inputs    :  p    : port structure
 *              buf  : data buffer
 *              size : data buffer size in bytes
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_write(aps_port_t *p,const void *buf,int size)
{
    aps_error_t errnum = APS_OK;

    while (size) {
        fd_set fds;
        int n;

        /*reset file descriptors set at each iteration*/
        /*this is required because select() modifies it*/
        /*Erik de Castro Lopo (bCODE)*/
        FD_ZERO(&fds);
        FD_SET(p->set.ethernet.sockfd,&fds);

        /*wait until some room is available in kernel write buffer*/
        if (p->write_timeout==0) {
            n = select(p->set.ethernet.sockfd+1,NULL,&fds,NULL,NULL);
        }
        else {
            struct timeval tv;

            tv.tv_sec = p->write_timeout/1000;
            tv.tv_usec = (p->write_timeout%1000)*1000;

            n = select(p->set.ethernet.sockfd+1,NULL,&fds,NULL,&tv);
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
        n = write(p->set.ethernet.sockfd,buf,size);

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
 * Name      :  ethernet_read
 * Purpose   :  Read data buffer from ETHERNET port
 * Inputs    :  p    : port structure
 *              buf  : data buffer
 *              size : data buffer size in bytes
 * Outputs   :  Fills data buffer
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_read(aps_port_t *p,void *buf,int size)
{
    aps_error_t errnum = APS_OK;

    while (size) {
        fd_set fds;
        int n;

        /*reset file descriptors set at each iteration*/
        /*this is required because select() modifies it*/
        /*Erik de Castro Lopo (bCODE)*/
        FD_ZERO(&fds);
        FD_SET(p->set.ethernet.sockfd,&fds);

        /*wait until some bytes are available in kernel read buffer*/
        if (p->read_timeout==0) {
            n = select(p->set.ethernet.sockfd+1,&fds,NULL,NULL,NULL);
        }
        else {
            struct timeval tv;

            tv.tv_sec = p->read_timeout/1000;
            tv.tv_usec = (p->read_timeout%1000)*1000;

            n = select(p->set.ethernet.sockfd+1,&fds,NULL,NULL,&tv);
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
        n = read(p->set.ethernet.sockfd,buf,size);

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
 * Name      :  ethernet_sync
 * Purpose   :  Block until all data pending in output buffer are transmitted
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_sync(aps_port_t *p)
{
    (void)p;

    /*we are always synchronized since USBDEVFS_BULK is synchronous*/
    return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  ethernet_flush
 * Purpose   :  Clear input and output buffers
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int ethernet_flush(aps_port_t *p)
{
    /* bulk write transfers are synchronous, so we know output buffer
     * is empty
     */
    (void)p;

    return APS_OK;
}

