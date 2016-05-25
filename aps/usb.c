/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : usb.c
 * DESCRIPTION   : APS library - USB communication routines
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
 *   10mar2006   nico    Initial revision
 *   07nov2006   nico    Added detection of APS USB vendor ID (usb_list_devices)
 *   07nov2006   nico    Added printer selection from vendor and product ID
 *   10apr2008   nico    Added usb_kill() function
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/usbdevice_fs.h>
#include <linux/version.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

#include <libusb-1.0/libusb.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

#define APS_VENDOR_ID0          0x0471
#define APS_VENDOR_ID           0x1AD4

/*default usbfs mount point*/
#define USBFS   "/dev/bus/usb"

#undef DEBUG


/*! \todo	shopov - fix these, move them to a proper place */
static libusb_context * libusb_ctx;
/* a list of all usb devices in the system; null if the list is unavailable */
static libusb_device ** devs;
/* the number of entries in the list above */
static ssize_t devcnt;


static void init_libusb(void)
{
	if (libusb_ctx == 0)
	{
		if (libusb_init(&libusb_ctx) != 0)
		{
			fprintf(stderr, "ERROR: failed to initialize libusb\n");
			/*! \todo	shopov - fix this, provide some means of error handling */
			exit(1);
		}
		if ((devcnt = libusb_get_device_list(libusb_ctx, &devs)) == LIBUSB_ERROR_NO_MEM)
		{
			fprintf(stderr, "ERROR: failed to obtain usb device list\n");
			exit(1);
		}
	}
}


/*-----------------------------------------------------------------------------
 * Name      :  usb_create
 * Purpose   :  Create USB port from device. Initialize settings to defaults
 * Inputs    :  p      : port structure
 *              device : device name
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 *
 * THIS FUNCTION IS DEPRECATED, DO NOT USE IT!!!
 * -----------------------------------------------------------------------------*/
int usb_create(aps_port_t *p,const char *device)
{
#if 0
	aps_error_t errnum;
	int fd;
	int addr[2];

	/*retrieve (busnum,devnum) address from kernel driver*/

	printf("open %s\n",device);

	fd = open(device,O_RDWR);

	if (fd<0) {
		return APS_OPEN_FAILED;
	}

	printf("ioctl %s\n",device);

	if (ioctl(fd,LPIOC_GET_BUS_ADDRESS(sizeof(addr)),addr)<0) {
		return APS_IO_ERROR;
	}

	if (close(fd)<0) {
		return APS_CLOSE_FAILED;
	}

	printf("create %s\n",device);

	/*create port from address*/
	errnum = usb_create_from_address(p,USBFS,addr[0],addr[1]);

	return errnum;
	return APS_OK;
#endif
	return APS_OPEN_FAILED;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_create_from_address
 * Purpose   :  Create USB port from address. Initialize settings to defaults
 * Inputs    :  p      : port structure
 *              usbfs  : usbfs mount point
 *              busnum : bus number
 *              devnum : device number
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * THIS FUNCTION IS DEPRECATED, DO NOT USE IT!!!
 * -----------------------------------------------------------------------------*/
int usb_create_from_address(aps_port_t *p,const char *usbfs,int busnum,int devnum)
{
#if 0
	aps_error_t errnum;
	usb_device_t list[USB_DEVICES_MAX];
	int n;

	/*copy usbfs mount point*/
	if (strlen(usbfs)>sizeof(p->set.usb.usbfs)-1) {
		return APS_NAME_TOO_LONG;
	}

	strcpy(p->set.usb.usbfs,usbfs);

	/*list connected devices*/
	n = usb_list_devices(list,USB_DEVICES_MAX,p->set.usb.usbfs);

	if (n<0) {
		return n;
	}

	/*compute physical connection path of (busnum,devnum) pair*/
	errnum = usb_trace_path(list,n,busnum,devnum,
			p->set.usb.path,sizeof(p->set.usb.path));

	return errnum;
#endif
	return APS_USB_DEVICE_NOT_FOUND;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_create_from_id
 * Purpose   :  Create USB port from VID/PID. Initialize settings to defaults
 * Inputs    :  p     : port structure
 *              usbfs : usbfs mount point
 *              vid   : device vendor ID
 *              pid   : device product ID
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * THIS FUNCTION IS DEPRECATED, DO NOT USE IT!!!
 * -----------------------------------------------------------------------------*/
int usb_create_from_id(aps_port_t *p,const char *usbfs,int vid,int pid)
{
#if 0
	aps_error_t errnum;
	usb_device_t list[USB_DEVICES_MAX];
	usb_device_t *dev;
	int n;

	/*copy usbfs mount point*/
	if (strlen(usbfs)>sizeof(p->set.usb.usbfs)-1) {
		return APS_NAME_TOO_LONG;
	}

	strcpy(p->set.usb.usbfs,usbfs);

	/*list connected devices*/
	n = usb_list_devices(list,USB_DEVICES_MAX,p->set.usb.usbfs);

	if (n<0) {
		return n;
	}

	/*find USB device*/
	dev = usb_find_by_id(list,n,vid,pid);

	if ((aps_error_t)dev<0) {
		return APS_USB_DEVICE_NOT_FOUND;
	}

	/*compute physical connection path of (busnum,devnum) pair*/
	errnum = usb_trace_path(list,n,dev->busnum,dev->devnum,
			p->set.usb.path,sizeof(p->set.usb.path));

	return errnum;
#endif
	return APS_USB_DEVICE_NOT_FOUND;
}



#if 0
static int find_aps_printer(int * busaddr, int * devaddr)
{
	libusb_device * dev;
	libusb_device ** devs;
	libusb_device_handle * h;
	int r;
	ssize_t cnt;
	int i;

	r = libusb_init(NULL);
	if (r < 0)
		return APS_USB_DEVICE_NOT_FOUND;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
	{
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		return APS_USB_DEVICE_NOT_FOUND;
	}

	i = 0;
	while ((dev = devs[i]) != NULL)
	{
		struct libusb_device_descriptor desc;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "ERROR: failed to get device descriptor");
			libusb_free_device_list(devs, 1);
			libusb_exit(NULL);
			return APS_USB_DEVICE_NOT_FOUND;
		}

		if (desc.idVendor == APS_VENDOR_ID || desc.idVendor == APS_VENDOR_ID0)
		{
			fprintf(stderr, "DEBUG: ok, aps printer found...\n");
			break;
		}
		i ++;
	}

	if (dev == 0)
	{
		printf("aps printer not found, exiting\n");
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		return APS_USB_DEVICE_NOT_FOUND;
	}
	printf("printer at bus: %i\n", libusb_get_bus_number(dev));
	printf("printer at address: %i\n", libusb_get_device_address(dev));

	* busaddr = libusb_get_bus_number(dev);
	* devaddr = libusb_get_device_address(dev);

	libusb_free_device_list(devs, 1);
	libusb_exit(NULL);
	return APS_OK;
}
#endif



/*-----------------------------------------------------------------------------
 * Name      :  usb_create_from_uri
 * Purpose   :  Create USB port from URI string
 * Inputs    :  p   : port structure
 *              uri : URI string
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_create_from_uri(aps_port_t *p,struct aps_uri *su)
{
	libusb_device * dev;
	libusb_device_handle * h;
	int r;
	ssize_t cnt;
	int i;
	const char *vidstr;
	const char *pidstr;
	int vid, pid;

	p->set.usb.busnum = 0;
	p->set.usb.devaddr = 0;

	vidstr = uri_get_opt(su,"vid");
	pidstr = uri_get_opt(su,"pid");

#ifdef DEBUG
	fprintf(stderr, "DEBUG: in %s()\n", __func__);
#endif
	if (devcnt == 0)
	{
		return APS_USB_DEVICE_NOT_FOUND;
	}

	if (vidstr == 0 || pidstr == 0)
	{
		return APS_INVALID_URI;
	}

	if (sscanf(vidstr,"%i",&vid)!=1) {
		return APS_INVALID_URI;
	}
	if (sscanf(pidstr,"%i",&pid)!=1) {
		return APS_INVALID_URI;
	}

	i = 0;
	while ((dev = devs[i]) != NULL)
	{
		struct libusb_device_descriptor desc;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "ERROR: failed to get device descriptor");
			libusb_free_device_list(devs, 1);
			libusb_exit(NULL);
			return APS_USB_DEVICE_NOT_FOUND;
		}

		//if (desc.idVendor == APS_VENDOR_ID || desc.idVendor == APS_VENDOR_ID0)
		if (desc.idVendor == vid && desc.idProduct == pid)
		{
#ifdef DEBUG
			fprintf(stderr, "DEBUG: ok, aps printer found...\n");
#endif
			break;
		}
		i ++;
	}

	if (dev == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
#endif
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_USB_DEVICE_NOT_FOUND;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: printer at bus: %i\n", libusb_get_bus_number(dev));
	fprintf(stderr, "DEBUG: printer at address: %i\n", libusb_get_device_address(dev));
#endif
	/* shopov(04072011) - added */
	p->set.usb.busnum = libusb_get_bus_number(dev);
	p->set.usb.devaddr = libusb_get_device_address(dev);
	return APS_OK;
	/* shopov(04072011) - end added */
#if 0	

	if (libusb_open(dev, &h))
	{
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_USB_DEVICE_NOT_FOUND;
	}
	r = libusb_kernel_driver_active(h, 0);
	if (r != 0 && r != 1)
	{
close_and_return:		
		libusb_close(h);
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_IO_ERROR;
	}
	p->set.usb.was_kernel_driver_attached = r;
	if (r)
	{
		r = libusb_detach_kernel_driver(h, 0);
		if (r != 0)
			goto close_and_return;
	}
	r = libusb_claim_interface(h, 0);
	if (r != 0)
		goto close_and_return;

	p->set.usb.pdev = dev;
	p->set.usb.hdev = h;

	return APS_OK;
#endif


#if 0
	struct stat st;
	const char *device;
	const char *usbfs;
	const char *path;
	const char *vidstr;
	const char *pidstr;

	/*get device name*/
	device = uri_get_device(su);

	printf("Device:%s\n",device);

	if (stat(device,&st)<0) {
		return APS_IO_ERROR;
	}

	printf("stats ok!:\n");

	if (S_ISCHR(st.st_mode)) {
		printf("S_ISCHR =============ok!:\n");
		/*build connection path from device name*/
		return usb_create(p,device);
	}
	else if (S_ISDIR(st.st_mode)) {
		printf("S_ISDIR ok!:\n");
		usbfs = device;
	}
	else {
		printf("Ke dall !  ok!:\n");
		return APS_INVALID_URI;
	}

	/*get path, vendor ID and product ID parameters*/
	path = uri_get_opt(su,"path");
	vidstr = uri_get_opt(su,"vid");
	pidstr = uri_get_opt(su,"pid");

	if (path!=NULL) {
		/*create USB port from connection path*/
		/*just copy path into port structure*/

		/*set usbfs parameter*/
		if (strlen(usbfs)>sizeof(p->set.usb.usbfs)-1) {
			return APS_NAME_TOO_LONG;
		}
		else {
			strcpy(p->set.usb.usbfs,usbfs);
		}

		/*copy connection path*/
		if (strlen(path)>sizeof(p->set.usb.path)-1) {
			return APS_NAME_TOO_LONG;
		}
		else {
			strcpy(p->set.usb.path,path);
		}
	}
	else if (vidstr!=NULL && pidstr!=NULL) {
		/*create USB port from (vendor ID,product ID) pair*/
		/*build connection path*/

		int vid;
		int pid;

		if (sscanf(vidstr,"%i",&vid)!=1) {
			return APS_INVALID_URI;
		}
		if (sscanf(pidstr,"%i",&pid)!=1) {
			return APS_INVALID_URI;
		}

		return usb_create_from_id(p,usbfs,vid,pid);
	}
	else {
		return APS_INVALID_URI;
	}

	return APS_OK;
#endif
	return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_list_ports
 * Purpose   :  List available USB ports
 * Inputs    :  p   : array of port structures
 *              max : maximum number of ports in list
 * Outputs   :  <>
 * Return    :  Number of ports or error code
 * -----------------------------------------------------------------------------*/
int usb_list_ports(aps_port_t **p,int max)
{
	const char *usbfs = USBFS;
	int n;
	int i;
	int total;
	libusb_device * dev;


	if (libusb_ctx == 0)
	{
		init_libusb();
	}

	/*list connected devices*/
	n = devcnt;

	if (n <= 0) {
		return 0;
	}

	/*build ports from APS devices*/
	total = 0;

	i = 0;
	while (total < max && (dev = devs[i]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int r;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "ERROR: failed to get device descriptor");
			libusb_free_device_list(devs, 1);
			libusb_exit(NULL);
			devs = 0;
			devcnt = 0;
			libusb_ctx = 0;
			return APS_USB_DEVICE_NOT_FOUND;
		}

		if (desc.idVendor == APS_VENDOR_ID || desc.idVendor == APS_VENDOR_ID0)
		{
#ifdef DEBUG
			fprintf(stderr, "DEBUG: ok, aps printer found...\n");
#endif
			*p = calloc(1, sizeof(aps_class_t));
			usb_custom(*p);
			(*p)->set.usb.pdev = dev;
			p ++;
			total ++;
		}
		i ++;
	}

#if 0
	for (i=0; i<n && total<max; i++) {
		libusb_device *dev = devs + i;

		if (dev->vendor_id==APS_VENDOR_ID || dev->vendor_id==APS_VENDOR_ID0) {
			*p = aps_create_usb_port_from_address(usbfs,dev->busnum,dev->devnum);

			if (*p!=NULL) {
				p++;
				total++;
			}
		}
	}
#endif

	return total;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_open
 * Purpose   :  Open USB port
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_open(aps_port_t *p)
{
	libusb_device * dev;
	libusb_device_handle * h;
	int r;
	ssize_t cnt;
	int i;
	const char *vidstr;
	const char *pidstr;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: in %s()\n", __func__);
#endif
	if (devcnt == 0)
	{
		return APS_USB_DEVICE_NOT_FOUND;
	}

	i = 0;
	while ((dev = devs[i]) != NULL)
	{
		struct libusb_device_descriptor desc;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "ERROR: failed to get device descriptor");
			libusb_free_device_list(devs, 1);
			libusb_exit(NULL);
			return APS_USB_DEVICE_NOT_FOUND;
		}

		/* shopov(04072011) - modified */
		//if (desc.idVendor == APS_VENDOR_ID || desc.idVendor == APS_VENDOR_ID0)
		if (libusb_get_bus_number(dev) == p->set.usb.busnum
				&& p->set.usb.devaddr == libusb_get_device_address(dev))
		{
#ifdef DEBUG
			fprintf(stderr, "DEBUG: ok, printer found...\n");
#endif
			break;
		}
		/* shopov(04072011) - end modified */
		i ++;
	}

	if (dev == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
#endif
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_USB_DEVICE_NOT_FOUND;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: printer at bus: %i\n", libusb_get_bus_number(dev));
	fprintf(stderr, "DEBUG: printer at address: %i\n", libusb_get_device_address(dev));
#endif
	if (libusb_open(dev, &h))
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
#endif
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_USB_DEVICE_NOT_FOUND;
	}
	r = libusb_kernel_driver_active(h, 0);
	if (r != 0 && r != 1)
	{
close_and_return:		
		libusb_close(h);
#ifdef DEBUG
		fprintf(stderr, "DEBUG: aps printer not found, exiting\n");
#endif
		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		return APS_IO_ERROR;
	}
	p->set.usb.was_kernel_driver_attached = r;
	if (r)
	{
		r = libusb_detach_kernel_driver(h, 0);
		if (r != 0)
			goto close_and_return;
	}
	r = libusb_claim_interface(h, 0);
	if (r != 0)
		goto close_and_return;

	p->set.usb.pdev = dev;
	p->set.usb.hdev = h;

	return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_close
 * Purpose   :  Close USB port
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_close(aps_port_t *p)
{
	int n;

	/*release interface zero*/
	n = libusb_release_interface(p->set.usb.hdev, 0);

	if (n != 0) {
		return APS_IO_ERROR;
	}

	/*if a kernel driver was connected, try reconnecting*/
	if (p->set.usb.was_kernel_driver_attached) {
		n = libusb_attach_kernel_driver(p->set.usb.hdev, 0);

		if (n != 0) {
			return APS_IO_ERROR;
		}
	}

	/*close device*/
	libusb_close(p->set.usb.hdev);
	p->set.usb.hdev = 0;

	return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_kill
 * Purpose   :  Kill USB port (force close without uninitialization)
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_kill(aps_port_t *p)
{
	/*close device*/
	libusb_close(p->set.usb.hdev);
	p->set.usb.hdev = 0;

	return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_control
 * Purpose   :  Perform a USB control request on port
 * Inputs    :  p    : port structure
 *              ctrl : control transfer structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_control(aps_port_t *p,aps_usb_ctrltransfer_t *ctrl)
{
	int n;

	n = libusb_control_transfer(p->set.usb.hdev, ctrl->bRequestType,
			ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->data,
			ctrl->wLength, p->write_timeout);

	if (n < 0) {
		return APS_IO_ERROR;
	}
	else {
		return APS_OK;
	}
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_write
 * Purpose   :  Write data buffer to USB port
 * Inputs    :  p    : port structure
 *              buf  : data buffer
 *              size : data buffer size in bytes
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_write(aps_port_t *p,const void *buf,int size)
{
	aps_error_t errnum = APS_OK;

	while (1) {
		int n;
		int xferred;

		n = libusb_bulk_transfer(p->set.usb.hdev, USB_DIR_OUT | USB_BULK_EP, (unsigned char *) buf,
				size, &xferred, p->write_timeout);
		if (n != 0) {
			if (n == LIBUSB_ERROR_TIMEOUT) {
				if (p->write_timeout!=0) {
					errnum = APS_WRITE_TIMEOUT;
					break;
				}
			}
			errnum = APS_WRITE_FAILED;
			break;
		}
		else if (xferred == 0)
		{
			/*we just sent the last packet (zero length)*/
			break;
		}
		else {
			size -= xferred;
			buf += xferred;
		}
	}

	return errnum;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_read
 * Purpose   :  Read data buffer from USB port
 * Inputs    :  p    : port structure
 *              buf  : data buffer
 *              size : data buffer size in bytes
 * Outputs   :  Fills data buffer
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_read(aps_port_t *p,void *buf,int size)
{
	aps_error_t errnum = APS_OK;
	unsigned char *dest = buf;

	while (size) {
		int n;

		/*copy data from read buffer*/
		while (size && p->set.usb.read_pos < p->set.usb.read_len) {
			*dest++ = p->set.usb.read_buf[p->set.usb.read_pos];
			size--;
			p->set.usb.read_pos++;
		}

		/*refill read buffer if there are still data to read*/
		if (size) {
			int xferred;

			n = libusb_bulk_transfer(p->set.usb.hdev, USB_DIR_IN | USB_BULK_EP, p->set.usb.read_buf,
					sizeof p->set.usb.read_buf, &xferred, p->read_timeout);

			if (n != 0) {
				if (n == LIBUSB_ERROR_TIMEOUT) {
					if (p->read_timeout!=0) {
						errnum = APS_READ_TIMEOUT;
						break;
					}
				}
				errnum = APS_READ_FAILED;
				break;
			}
			else {
				p->set.usb.read_pos = 0;
				p->set.usb.read_len = xferred;
			}
		}
	}

	return errnum;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_sync
 * Purpose   :  Block until all data pending in output buffer are transmitted
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_sync(aps_port_t *p)
{
	(void)p;

	/*we are always synchronized since USBDEVFS_BULK is synchronous*/
	return APS_OK;
}

/*-----------------------------------------------------------------------------
 * Name      :  usb_flush
 * Purpose   :  Clear input and output buffers
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------*/
int usb_flush(aps_port_t *p)
{
char xbuf;
int xferred;

	/* bulk write transfers are synchronous, so we know output buffer
	 * is empty
	 */

	/*! \todo	shopov(27092011) - i am not sure how input buffers can
	 * 		be flushed, so i am inserting a dummy read here... */
	while (libusb_bulk_transfer(p->set.usb.hdev, USB_DIR_IN | USB_BULK_EP, &xbuf, 1, &xferred, 100)==0);

	/*clear read buffer*/
	memset(p->set.usb.read_buf,0,sizeof(p->set.usb.read_buf));
	p->set.usb.read_pos = 0;
	p->set.usb.read_len = 0;

	return APS_OK;
}

/*
 *   -----------------------------------------------------------------------------
 *    Name      :  usb_get_uri
 *    Purpose   :  Get URI string defining USB port
 *    Inputs    :  p    : port structure
 *                 uri  : URI string buffer
 *                 size : URI string buffer size (includes trailing zero)
 *    Outputs   :  Fills URI string buffer
 *    Return    :  APS_OK or error code
 *   -----------------------------------------------------------------------------
 */
int usb_get_uri(aps_port_t *p,char *uri,int size)
{
	aps_error_t errnum;
	int len;

	errnum = APS_INVALID_URI;
	if (p->set.usb.pdev)
	{
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(p->set.usb.pdev, &desc))
			return APS_INVALID_URI;

		len = snprintf(uri,size,"aps:/dev/bus/usb?type=usb+vid=%i+pid=%i",
				desc.idVendor,
				desc.idProduct);

		if (len>=size) {
			errnum = APS_INVALID_URI;
		}
		else {
			errnum = APS_OK;
		}
	}
	else
		errnum = APS_INVALID_URI;

	return errnum;
}



/*
 * -----------------------------------------------------------------------------
 * Name      :  usb_custom
 * Purpose   :  set all fonction pointer for usb mode
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */ 
void usb_custom(aps_class_t *p)
{
	if (p == NULL)
		return;

	p->port.type=APS_USB;

	p->get_uri = usb_get_uri;

	p->create = usb_create;
	p->create_from_uri = usb_create_from_uri;

	p->open_ = usb_open;
	p->close = usb_close;

	p->write = usb_write;
	p->read = usb_read;

	p->sync = usb_sync;
	p->flush = usb_flush;

	if (libusb_ctx)
	{
		if (devs) libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
		devs = 0;
		devcnt = 0;
		libusb_ctx = 0;
	}

	if (libusb_ctx == 0)
	{
		init_libusb();
	}
	p->port.set.usb.pdev = 0;

	p->port.set.usb.was_kernel_driver_attached = 0;
	p->port.set.usb.pdev = 0;
	p->port.set.usb.hdev = 0;
}


