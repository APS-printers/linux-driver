/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : aps-private.h
 * DESCRIPTION   : APS library - private definitions
 * CVS           : $Id: aps-private.h,v 1.17 2009/01/16 16:14:34 pierre Exp $
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

#ifndef _APS_PRIVATE_H
#define _APS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libusb-1.0/libusb.h>

#define APS_DATA_LOG "/tmp/aps_log.bin"

	/*ASCII control codes*/
	typedef enum {
		NUL     = 0,
		SOH     = 1,
		STX     = 2,
		ETX     = 3,
		EOT     = 4,
		ENQ     = 5,
		ACK     = 6,
		BEL     = 7,
		BS      = 8,
		TAB     = 9,
		LF      = 10,
		VT      = 11,
		FF      = 12,
		CR      = 13,
		SO      = 14,
		SI      = 15,
		DLE     = 16,
		DC1     = 17,
		DC2     = 18,
		DC3     = 19,
		DC4     = 20,
		NAK     = 21,
		SYN     = 22,
		ETB     = 23,
		CAN     = 24,
		EM      = 25,
		SUB     = 26,
		ESC     = 27,
		FS      = 28,
		GS      = 29,
		RS      = 30,
		US      = 31
	} aps_control_code_t;

	/* URI routines -------------------------------------------------------------*/

#define URI_MAX         255
#define OPTS_MAX        8

	struct aps_uri {
		char    buf[URI_MAX+1];
		int     bufpos;
		char *  device;
		int     nopts;
		struct {
			char *  key;
			char *  value;
		} opts[OPTS_MAX];
	};

	int     uri_split(struct aps_uri *su,const char *uri);
	int     uri_join(struct aps_uri *su,char *uri,int size);

	void    uri_reset(struct aps_uri *su);
	int     uri_add_device(struct aps_uri *su,const char *device);
	int     uri_add_opt(struct aps_uri *su,const char *key,const char *value);

	const char *    uri_get_device(struct aps_uri *su);
	const char *    uri_get_opt(struct aps_uri *su,const char *key);

	/* Port definition ----------------------------------------------------------*/

#define DEVICE_MAX              255     /*characters*/

#define NODE_MAX                255     /*characters*/
#define SERVICE_MAX             255     /*characters*/

#define USBFS_MAX               255     /*characters*/
#define USBPATH_MAX             31      /*characters*/

	/*in case these were already defined*/
#undef USB_DIR_OUT
#undef USB_DIR_IN

#define USB_DIR_OUT		0x00	/*to device*/
#define USB_DIR_IN		0x80	/*to host*/

#define USB_BULK_EP             2       /*bidirectionnal*/

#define USB_BULK_OUT_EP_SIZE    64      /*characters*/
#define USB_BULK_IN_EP_SIZE     64      /*characters*/

#define USB_READ_BUFSIZE        USB_BULK_IN_EP_SIZE



	typedef struct {
		char    device[DEVICE_MAX+1];
		int     fd;
		int     baudrate;
		int     handshake;
	} aps_setting_serial_t;

	typedef struct {
		char    device[DEVICE_MAX+1];
		int     fd;
		int     mode;
		int     irq_left;
	} aps_setting_par_t;

	typedef struct {
		libusb_device * pdev;
		/* if hdev is null, the device is invalid (not opened) */
		libusb_device_handle * hdev;
		/* a zero value for this is invalid */
		int	busnum;
		/* a zero value for this is invalid */
		int	devaddr;
		/* denotes if a kernel driver was attached prior to opening the device */
		int	was_kernel_driver_attached;
		/*user mode read buffer*/
		unsigned char   read_buf[USB_READ_BUFSIZE];
		int             read_pos;
		int             read_len;
	} aps_setting_usb_t;

	typedef struct {
		char    node[NODE_MAX+1];
		char    service[SERVICE_MAX+1];
		int     sockfd;
	} aps_setting_ethernet_t;

	typedef union {
		aps_setting_serial_t    serial;
		aps_setting_par_t       par;
		aps_setting_usb_t       usb;
		aps_setting_ethernet_t  ethernet;
	} aps_settings_t;

	typedef struct {
		int             type;
		int             errnum;
		int             sub_errnum;
		int             is_open;
		int             write_timeout;          /*milliseconds*/
		int             read_timeout;           /*milliseconds*/
		aps_settings_t  set;
#if defined(APS_DATA_LOG)
		FILE*           data_log_fd;
#endif
	} aps_port_t;


	/* Serial port routines -----------------------------------------------------*/
	int     serial_get_uri(aps_port_t *p,char *uri,int size);
	int     serial_create(aps_port_t *p,const char *device);
	int     serial_create_from_uri(aps_port_t *p,struct aps_uri *su);

	int     serial_open(aps_port_t *p);
	int     serial_close(aps_port_t *p);

	int     serial_write(aps_port_t *p,const void *buf,int size);
	int     serial_write_rt(aps_port_t *p,const void *buf,int size);
	int     serial_read(aps_port_t *p,void *buf,int size);

	int     serial_sync(aps_port_t *p);
	int     serial_flush(aps_port_t *p);
	/* custom for serial */
	int     serial_set_baudrate(aps_port_t *p,int baudrate);
	int     serial_set_handshake(aps_port_t *p,int handshake);


	/* Parallel port routines ---------------------------------------------------*/

	int     par_get_uri(aps_port_t *p,char *uri,int size);
	int     par_create(aps_port_t *p,const char *device);
	int     par_create_from_uri(aps_port_t *p,struct aps_uri *su);

	int     par_open(aps_port_t *p);
	int     par_close(aps_port_t *p);

	int     par_write(aps_port_t *p,const void *buf,int size);
	int     par_write_rt(aps_port_t *p,const void *buf,int size);
	int     par_read(aps_port_t *p,void *buf,int size);

	int     par_sync(aps_port_t *p);
	int     par_flush(aps_port_t *p);
	/* custom for parallel */
	int     par_reset(aps_port_t *p);
	int     par_set_mode(aps_port_t *p,int mode);

	/* USB port routines --------------------------------------------------------*/

	int     usb_get_uri(aps_port_t *p,char *uri,int size);
	int     usb_create(aps_port_t *p,const char *device);
	int     usb_create_from_uri(aps_port_t *p,struct aps_uri *su);

	int     usb_open(aps_port_t *p);
	int     usb_close(aps_port_t *p);

	int     usb_write(aps_port_t *p,const void *buf,int size);
	int     usb_read(aps_port_t *p,void *buf,int size);

	int     usb_sync(aps_port_t *p);
	int     usb_flush(aps_port_t *p);

	/* custom for usb */
	int     usb_list_ports(aps_port_t **p,int max);
	int     usb_create_from_id(aps_port_t *p,const char *usbfs,int vid,int pid);
	int     usb_create_from_address(aps_port_t *p,const char *usbfs,int busnum,int devnum);
	int     usb_control(aps_port_t *p,aps_usb_ctrltransfer_t *ctrl);
	int     usb_kill(aps_port_t *p);

	/* ethernet port routines -----------------------------------------------------*/
	int     ethernet_get_uri(aps_port_t *p,char *uri,int size);
	int     ethernet_create(aps_port_t *p,const char *device);
	int     ethernet_create_from_uri(aps_port_t *p,struct aps_uri *su);

	int     ethernet_open(aps_port_t *p);
	int     ethernet_close(aps_port_t *p);

	int     ethernet_write(aps_port_t *p,const void *buf,int size);
	int     ethernet_read(aps_port_t *p,void *buf,int size);

	int     ethernet_sync(aps_port_t *p);
	int     ethernet_flush(aps_port_t *p);
	/* custom for ethernet */
	/* no specific at this moment */

	typedef struct {
		aps_port_t port;
		/* fonction customisation */
		int     (*get_uri)(aps_port_t *p,char *uri,int size);

		int     (*create)(aps_port_t *p,const char *device);
		int     (*create_from_uri)(aps_port_t *p,struct aps_uri *su);

		/* _ is to avoid confusion with old open boolean menber */
		int     (*open_)(aps_port_t *p);
		int     (*close)(aps_port_t *p);

		int     (*write)(aps_port_t *p,const void *buf,int size);
		int     (*write_rt)(aps_port_t *p,const void *buf,int size);
		int     (*read)(aps_port_t *p,void *buf,int size);

		int     (*sync)(aps_port_t *p);
		int     (*flush)(aps_port_t *p);

	} aps_class_t;

	/* customisation of class */
	void serial_custom(aps_class_t *p);
	void par_custom(aps_class_t *p);
	void usb_custom(aps_class_t *p);
	void ethernet_custom(aps_class_t *p);

	/* data log routines -----------------------------------------------------*/

#if defined(APS_DATA_LOG)
	int     log_open(aps_port_t *p);
	int     log_close(aps_port_t *p);
	int     log_write(aps_port_t *p,const void *buf,int size);
	int     log_read(aps_port_t *p,void *buf,int size);
	int     log_flush(aps_port_t *p);
#else
#   define log_open(p)           (APS_OK);
#   define log_close(p)          (APS_OK);
#   define log_write(p,buf,size) (APS_OK);
#   define log_read(p,buf,size)  (APS_OK);
#   define log_flush(p)          (APS_OK);
#endif

	/* Serial port specific routines --------------------------------------------*/


	/* Parallel port specific routines ------------------------------------------*/


	/* Usb port specific routines -----------------------------------------------*/


	/* port common routines -----------------------------------------------------*/



	/* Models database ----------------------------------------------------------*/

	typedef struct {
		int             model;
		int             type;
		int             width;
		const char *    name;
		const char *    id;
	} model_t;

	const model_t * model_find_by_number(int model);
	const model_t * model_find_by_id(const char *s);

#ifdef __cplusplus
}
#endif

#endif /*_APS_PRIVATE_H*/

