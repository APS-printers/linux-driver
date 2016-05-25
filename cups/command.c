/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : command.c
* DESCRIPTION   : Command set routines
* CVS           : $Id: command.c,v 1.12 2008/06/09 14:46:45 nicolas Exp $
*******************************************************************************
*   Copyright (C) 2006  APS Engineering
*   
*   This file is part of the APS Linux Driver.
*
*   APS Linux Driver is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   APS Linux Driver is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with APS Linux Driver; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*******************************************************************************
* HISTORY       :
*   15may2006   nico    Initial revision
*   07oct2006   nico    Added get NEOP status command
*   31jan2007   nico    Added adjust FIFO parameter to serial settings command
*   10apr2008   nico    Corrected bug in set max speed command (MRS/HRS/KCP)
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aps/aps.h>

#include <qrencode.h>

#include "command.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/*APS USB vendor requests*/
#define APS_HSP_GET_STATUS      0
#define APS_HRS_GET_STATUS      1
#define APS_KCP_GET_STATUS      1
#define APS_HARD_RESET          2
#define APS_ACK_ERROR           3

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  cmd_reset
Purpose   :  Build 'reset' command based on model type
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_reset(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
        case APS_HSP:
                cmd->size = 2;
                cmd->buf[0] = ESC;
                cmd->buf[1] = '@';
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_enter_full_mrs_mode
Purpose   :  Build 'enter full MRS mode' command based on model type
             This command is only available on CP205MRS printers
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_enter_full_mrs_mode(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
                cmd->size = 2;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'f';
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_get_status
Purpose   :  Build 'get status' command based on model type
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_get_status(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
                cmd->size = 2;
                cmd->answer = 1;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'v';
                break;
        case APS_KCP:
                cmd->size = 2;
                cmd->answer = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'v';
                break;
        case APS_HSP:
                cmd->size = 2;
                cmd->answer = 4;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'v';
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_get_status_neop
Purpose   :  Build 'get near end-of-paper status' command based on model type
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_get_status_neop(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 4;
                cmd->answer = 1;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'n';
                cmd->buf[2] = 's';
                cmd->buf[3] = CAN;      /*clear print buffer for old firmwares*/
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_serial_opt
Purpose   :  Build 'set serial options' command based on model type
Inputs    :  type      : model type
             cmd       : command structure
             baudrate  : serial baudrate
             handshake : serial handshake mode
             adjust    : adjust FIFO thresholds if TRUE (only on MRS printers)
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_serial_opt(int type,command_t *cmd,int baudrate,int handshake,int adjust)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 3;

                cmd->buf[0] = GS;
                cmd->buf[1] = 'B';

                /*convert APS library baudrate to MRS/HRS/KCP*/
                switch (baudrate) {
                case APS_B1200:
                        cmd->buf[2] = 0;
                        break;
                case APS_B2400:
                        cmd->buf[2] = 1;
                        break;
                case APS_B4800:
                        cmd->buf[2] = 2;
                        break;
                case APS_B9600:
                        cmd->buf[2] = 3;
                        break;
                case APS_B19200:
                        cmd->buf[2] = 4;
                        break;
                case APS_B38400:
                        cmd->buf[2] = 5;
                        break;
                case APS_B57600:
                        cmd->buf[2] = 6;
                        break;
                case APS_B115200:
                        cmd->buf[2] = 7;
                        break;
                default:
                        errnum = APS_INVALID_BAUDRATE;
                        break;
                }

                /*convert APS library handshake to MRS/HRS/KCP*/
                switch (handshake) {
                case APS_XONXOFF:
                        cmd->buf[2] |= 0x00;
                        break;
                case APS_RTSCTS:
                        cmd->buf[2] |= 0x80;
                        break;
                default:
                        errnum = APS_INVALID_HANDSHAKE;
                        break;
                }

                /*adjust FIFO thresholds on MRS printers*/
                /*functionality ignored for other printer types*/
                if (type==APS_MRS && adjust) {
                        cmd->buf[2] |= 0x40;
                }
                
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_font
Purpose   :  Build 'set font' command based on model type
Inputs    :  type : model type
             cmd  : command structure
             n    : font number
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_font(int type,command_t *cmd,int n)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = '%';
                cmd->buf[2] = n;
                break;
        case APS_HSP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'M';
                cmd->buf[2] = n;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_dynamic_division
Purpose   :  Build 'set dynamic division' command based on model type
Inputs    :  type : model type
             cmd  : command structure
             n    : number of black bytes (see printer specification)
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_dynamic_division(int type,command_t *cmd,int n)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 3;
                cmd->buf[0] = GS;
                cmd->buf[1] = '/';
                cmd->buf[2] = n;
                break;
        case APS_HSP:
                cmd->size = 4;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'J';
                cmd->buf[2] = '3';
                cmd->buf[3] = n;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_maximum_speed
Purpose   :  Build 'set maximum print speed' command based on model type
Inputs    :  type  : model type
             cmd   : command structure
             speed : maximum print speed in mm/s
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_maximum_speed(int type,command_t *cmd,int speed)
{
        aps_error_t errnum = APS_OK;
        int t;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                t = 125000/speed;
                cmd->size = 4;
                cmd->buf[0] = GS;
                cmd->buf[1] = 's';
                cmd->buf[2] = (t>>8)&255;       /*MSB*/
                cmd->buf[3] = t&255;            /*LSB*/
                break;
        case APS_HSP:
                t = 0.5*speed;
                cmd->size = 4;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'M';
                cmd->buf[2] = '2';
                cmd->buf[3] = t;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_intensity
Purpose   :  Build 'set print intensity' command based on model type
Inputs    :  type    : model type
             cmd     : command structure
             percent : print intensity in percent
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_intensity(int type,command_t *cmd,int percent)
{
        aps_error_t errnum = APS_OK;
        int n;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                n = 128+(127*percent/60);
                cmd->size = 3;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'D';
                cmd->buf[2] = n;
                break;
        case APS_HSP:
                n = 128+(76*percent/60);
                cmd->size = 4;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'J';
                cmd->buf[2] = '2';
                cmd->buf[3] = n;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_compression
Purpose   :  Build 'set compression mode' command based on model type
Inputs    :  type : model type
             cmd  : command structure
             flag : compression flag (1=enable, 0=disable)
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_compression(int type,command_t *cmd,int flag)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_HRS:
        case APS_KCP:
                cmd->size = 4;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'G';
                cmd->buf[2] = flag;
                cmd->buf[3] = CAN;      /*clear print buffer for old firmwares*/
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_char_spacing
Purpose   :  Build 'set char spacing' command based on model type
Inputs    :  type : model type
             cmd  : command structure
             n    : spacing in pixels
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_char_spacing(int type,command_t *cmd,int n)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
        case APS_HSP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = ' ';
                cmd->buf[2] = n;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_set_line_spacing
Purpose   :  Build 'set line spacing' command based on model type
Inputs    :  type : model type
             cmd  : command structure
             n    : spacing in dotlines
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_set_line_spacing(int type,command_t *cmd,int n)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
        case APS_HSP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = '3';
                cmd->buf[2] = n;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}


/* shopov: added for the lpm driver board - to issue a move-to-next ticket command */
int cmd_lpm_end_of_ticket(int type,command_t *cmd)
{
	aps_error_t errnum = APS_OK;

	memset(cmd,0,sizeof(command_t));

	cmd->size = 2;
	cmd->buf[0] = GS;
	cmd->buf[1] = 'T';

	return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_full_cut
Purpose   :  Build 'perform full cut' command based on model type
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_full_cut(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 2;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'i';
                break;
        case APS_HSP:
                cmd->size = 4;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'V';
                cmd->buf[2] = 66;
                cmd->buf[3] = 0;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_partial_cut
Purpose   :  Build 'perform partial cut' command based on model type
Inputs    :  type : model type
             cmd  : command structure
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_partial_cut(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 2;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'm';
                break;
        case APS_HSP:
                cmd->size = 3;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'V';
                cmd->buf[2] = 1;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_feed_forward
Purpose   :  Build 'feed paper forward' command based on model type
Inputs    :  type     : model type
             cmd      : command structure
             dotlines : number of dotlines to feed
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_feed_forward(int type,command_t *cmd,int dotlines)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
        case APS_HSP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'J';
                cmd->buf[2] = dotlines;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_feed_backward
Purpose   :  Build 'feed paper backward' command based on model type
Inputs    :  type     : model type
             cmd      : command structure
             dotlines : number of dotlines to feed
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_feed_backward(int type,command_t *cmd,int dotlines)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 3;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'j';
                cmd->buf[2] = dotlines;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_shift_dotline
Purpose   :  Build 'shift dotline' command header based on model type
Inputs    :  type   : model type
             cmd    : command structure
             nbytes : right shift amount in bytes
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_shift_dotline(int type,command_t *cmd,int nbytes)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 4;
                cmd->buf[0] = ESC;
                cmd->buf[1] = '$';
                cmd->buf[2] = nbytes&255;
                cmd->buf[3] = (nbytes>>8)&255;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_print_dotline
Purpose   :  Build 'print dotline' command header based on model type
Inputs    :  type   : model type
             cmd    : command structure
             nbytes : width of dotline in bytes
Outputs   :  Fills command structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_print_dotline(int type,command_t *cmd,int nbytes)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
        case APS_MRS:
        case APS_HRS:
        case APS_KCP:
                cmd->size = 5;
                cmd->buf[0] = ESC;
                cmd->buf[1] = 'V';
                cmd->buf[2] = 0;
                cmd->buf[3] = nbytes&255;
                cmd->buf[4] = (nbytes>>8)&255;
                break;
        case APS_HSP:
                cmd->size = 8;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'v';
                cmd->buf[2] = '0';
                cmd->buf[3] = 0;
                cmd->buf[4] = nbytes&255;
                cmd->buf[5] = (nbytes>>8)&255;
                cmd->buf[6] = 1;
                cmd->buf[7] = 0;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_usb_get_status
Purpose   :  Build 'get status' USB request based on model type
Inputs    :  type : model type
             ctrl : control request structure
Outputs   :  Fills control request structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_usb_get_status(int type,aps_usb_ctrltransfer_t *ctrl)
{
        aps_error_t errnum = APS_OK;

        memset(ctrl,0,sizeof(aps_usb_ctrltransfer_t));

        switch (type) {
        case APS_HRS:
                ctrl->bRequestType = 0xc1;
                ctrl->bRequest = APS_HRS_GET_STATUS;
                ctrl->wValue = 0;
                ctrl->wIndex = 0;
                ctrl->wLength = 1;
                break;
        case APS_KCP:
                ctrl->bRequestType = 0xc1;
                ctrl->bRequest = APS_KCP_GET_STATUS;
                ctrl->wValue = 0;
                ctrl->wIndex = 0;
                ctrl->wLength = 3;
                break;
        case APS_HSP:
                ctrl->bRequestType = 0xc1;
                ctrl->bRequest = APS_HSP_GET_STATUS;
                ctrl->wValue = 0;
                ctrl->wIndex = 0;
                ctrl->wLength = 4;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}

/*-----------------------------------------------------------------------------
Name      :  cmd_usb_hard_reset
Purpose   :  Build 'hard reset' USB request based on model type
Inputs    :  type : model type
             ctrl : control request structure
Outputs   :  Fills control request structure
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int cmd_usb_hard_reset(int type,aps_usb_ctrltransfer_t *ctrl)
{
        aps_error_t errnum = APS_OK;

        memset(ctrl,0,sizeof(aps_usb_ctrltransfer_t));

        switch (type) {
        case APS_HRS:
        case APS_KCP:
        case APS_HSP:
                ctrl->bRequestType = 0x41;
                ctrl->bRequest = APS_HARD_RESET;
                ctrl->wValue = 0;
                ctrl->wIndex = 0;
                ctrl->wLength = 0;
                break;
        default:
                errnum = APS_INVALID_MODEL_TYPE;
                break;
        }

        return errnum;
}


int cmd_lpm_calibrate(int type,command_t *cmd)
{
        aps_error_t errnum = APS_OK;

        memset(cmd,0,sizeof(command_t));

        switch (type) {
		default:
                cmd->size = 2;
                cmd->buf[0] = GS;
                cmd->buf[1] = 'E';
                break;
        }

        return errnum;
}



/*!
 *	\fn	int cmd_mrs_qrcode(int version, int level, int mode, int casesensitivity, char * data, char ** qr_data_buf, int * qrbuf_len)
 *	\brief	formats a qrcode barcode for mrs printers
 *
 *	\param	version		the version to use for the qr barcode; if 0, then
 *				the minimum suitable version will be selected
 *				automatically
 *	\param	level		the error correction level to use; valid values are:
 *					0 - level L
 *					1 - level M
 *					2 - level Q
 *					3 - level H
 *	\param	mode		the encoding mode to use; valid values are:
 *					0 - NUM
 *					1 - AN
 *					2 - 8bit
 *					3 - kanji
 *	\param	casesensitivity	nonzero if the qr code should be case sensitive
 *	\param	data		the nul terminated string to encode
 *	\param	qr_data_buf	a pointer to where to store the encoded qr code;
 *				if non null on return, the caller *must* free this
 *				buffer by using free()
 *	\param	qrbuf_len	the size of the qrbuf above, in bytes
 *	\param	cmd		command to set
 *	\return		APS_OK on success, APS_NOT_IMPLEMENTED on error
 *	*/
int cmd_mrs_qrcode(int version, int level, int mode, int casesensitivity, char * data, char ** qr_data_buf, int * qrbuf_len, command_t * cmd)
{
/*! \todo	make this a configurable parameter */
enum
{
	QR_SCALE = 5,
};
	aps_error_t errnum = APS_OK;
	memset(cmd,0,sizeof(command_t));

	//QR_APP(unsigned char type,int size,int *data,unsigned char Level)
	{
		int size;
		int Level = level;
		int type = mode;

		size = strlen(data);
		// Data size check:       
		switch (type)
		{
			case '0':     // Numeric                                               

				if (Level==1)
				{
					if(size>2188) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>1588) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>1228) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>2812) { return APS_NOT_IMPLEMENTED;}
				}
				break;
			case '2':          //Data bit       
				if (Level==1) 
				{
					if(size>7312) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>5312) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>4112) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>9392) { return APS_NOT_IMPLEMENTED;}
				}
				break;
			case '3':                 //Kanji
				if (Level==1) 
				{
					if(size>561) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>407) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>315) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>721) { return APS_NOT_IMPLEMENTED;}                                   
				}
				break;
			default:        //Alfanumernic
				if (Level==1) 
				{
					if(size>1326) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>963) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>744) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>1704) { return APS_NOT_IMPLEMENTED;}                                             
				}
				break;
		}   // End
	}   // End



#if 0
	switch (type) {
		case APS_MRS:
			break;
		case APS_HRS:
		case APS_KCP:
		case APS_HSP:
		default:
			return errnum = APS_INVALID_MODEL_TYPE;
	}
#endif

	* qr_data_buf = 0;
	* qrbuf_len = 0;

	{
		int i, j, x, bcnt, len;
		char * qrbuf;
		int idx;
		QRcode *qrcode;
		unsigned char * p, cb;
		qrbuf = data;
		idx = strlen(qrbuf);
#if 1
		qrcode = QRcode_encodeString(qrbuf, version, level, mode, casesensitivity);
		if (!qrcode)
		{
			return APS_NOT_IMPLEMENTED;
		}

		//p = malloc(len = ((qrcode->width + 7) / 8) * 8);
		len = (qrcode->width * QR_SCALE + 7) / 8;
		p = malloc(len * qrcode->width * QR_SCALE);
		memset(p, 0x5a, len * qrcode->width * QR_SCALE);
		* qr_data_buf = p;
		* qrbuf_len = len * qrcode->width * QR_SCALE;

		for (idx = x = i = 0; i < qrcode->width; i ++)
		{
			int xx;
			int ii;

			xx = x;
			for (ii = 0; ii < QR_SCALE; ii ++)
			{
				x = xx;
				for (bcnt = cb = j = 0; j < qrcode->width; j ++, x ++)
				{
					int sc;
					for (sc = 0; sc < QR_SCALE; sc ++)
					{
						cb <<= 1;
						if (qrcode->data[x] & 1)
							cb |= 1;
						if (++ bcnt == 8)
						{
							p[idx ++] = cb;
							cb = 0;
							bcnt = 0;
						}
					}
				}
				if (bcnt)
				{
					cb <<= 8 - bcnt;
					p[idx ++] = cb;
				}
			}
		}

		cmd->size = 8;
		cmd->buf[0] = ESC;
		cmd->buf[1] = '*';
		cmd->buf[2] = qrbuf_len[0];
		cmd->buf[3] = qrbuf_len[0] >> 8;
		cmd->buf[4] = qrbuf_len[0] >> 16;
		cmd->buf[5] = 0;
		x = (640 - len) / 16;
		if (x >= 0)
			cmd->buf[6] = x;
		else
			cmd->buf[6] = 0;
		cmd->buf[7] = len;
#endif
		//write_command(0, &cmd, p, idx);
		QRcode_free(qrcode);

	}

	return errnum;
}


/*!
 *	\n	int cmd_mrs_qrcode_bindata(int version, int level, int mode, char * data, int dlen, char ** qr_data_buf, int * qrbuf_len, command_t * cmd, int * qr_width)
 *	\brief	formats a qrcode barcode for mrs printers
 *
 *	\param	version		the version to use for the qr barcode; if 0, then
 *				the minimum suitable version will be selected
 *				automatically
 *	\param	level		the error correction level to use; valid values are:
 *					0 - level L
 *					1 - level M
 *					2 - level Q
 *					3 - level H
 *	\param	mode		the encoding mode to use; valid values are:
 *					0 - NUM
 *					1 - AN
 *					2 - 8bit
 *					3 - kanji
 *	\param	casesensitivity	nonzero if the qr code should be case sensitive
 *	\param	data		a pointer to the buffer containing the data to encode
 *	\param	dlen		the size of the dlen buffer above, in bytes
 *	\param	qr_data_buf	a pointer to where to store the encoded qr code;
 *				if non null on return, the caller *must* free this
 *				buffer by using free()
 *	\param	qrbuf_len	the size of the qr_data_buf above, in bytes
 *	\param	cmd		command to set
 *	\param	qr_width	will be set to the width of the qr code
 *	\return		APS_OK on success, APS_NOT_IMPLEMENTED on error
 *	*/
int cmd_mrs_qrcode_bindata(int version, int level, int mode, char * data, int dlen, char ** qr_data_buf, int * qrbuf_len, command_t * cmd, int * qr_width)
{
	QRinput * stream;
	aps_error_t errnum = APS_OK;
	memset(cmd,0,sizeof(command_t));

	//QR_APP(unsigned char type,int size,int *data,unsigned char Level)
	{
		int size;
		int Level = level;
		int type = mode;

		size = dlen;
		// Data size check:       
		switch (type)
		{
			case '0':     // Numeric                                               

				if (Level==1)
				{
					if(size>2188) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>1588) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>1228) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>2812) { return APS_NOT_IMPLEMENTED;}
				}
				break;
			case '2':          //Data bit       
				if (Level==1) 
				{
					if(size>7312) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>5312) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>4112) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>9392) { return APS_NOT_IMPLEMENTED;}
				}
				break;
			case '3':                 //Kanji
				if (Level==1) 
				{
					if(size>561) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>407) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>315) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>721) { return APS_NOT_IMPLEMENTED;}                                   
				}
				break;
			default:        //Alfanumernic
				if (Level==1) 
				{
					if(size>1326) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==2) 
				{
					if(size>963) { return APS_NOT_IMPLEMENTED;}
				}
				else if (Level==3) 
				{
					if(size>744) { return APS_NOT_IMPLEMENTED;}   
				}
				else if (Level==0) 
				{
					if(size>1704) { return APS_NOT_IMPLEMENTED;}                                             
				}
				break;
		}   // End
	}   // End



#if 0
	switch (type) {
		case APS_MRS:
			break;
		case APS_HRS:
		case APS_KCP:
		case APS_HSP:
		default:
			return errnum = APS_INVALID_MODEL_TYPE;
	}
#endif

	* qr_data_buf = 0;
	* qrbuf_len = 0;
	* qr_width = 0;

	{
		int i, j, x, bcnt, len;
		char * qrbuf;
		int idx;
		QRcode *qrcode;
		unsigned char * p, cb;
		qrbuf = data;
		idx = strlen(qrbuf);
#if 1
		stream = QRinput_new();
		if (!stream)
		{
			return APS_NOT_IMPLEMENTED;
		}

		QRinput_setVersion(stream, version);
		QRinput_setErrorCorrectionLevel(stream, level);
		QRinput_append(stream, mode, dlen, data);

		qrcode = QRcode_encodeInput(stream);

		if (!qrcode)
		{
			QRinput_free(stream);
			return APS_NOT_IMPLEMENTED;
		}
		* qr_width = qrcode->width;

		//p = malloc(len = ((qrcode->width + 7) / 8) * 8);
		len = (qrcode->width + 7) / 8;
		p = malloc(len * qrcode->width);
		memset(p, 0x5a, len * qrcode->width);
		* qr_data_buf = p;
		* qrbuf_len = len * qrcode->width;

		for (idx = x = i = 0; i < qrcode->width; i ++)
		{
			for (bcnt = cb = j = 0; j < qrcode->width; j ++, x ++)
			{
				cb <<= 1;
				if (qrcode->data[x] & 1)
					cb |= 0x80 >> 7;
				if (++ bcnt == 8)
				{
					p[idx ++] = cb;
					cb = 0;
					bcnt = 0;
				}
			}
			if (bcnt)
			{
				cb <<= 8 - bcnt;
				p[idx ++] = cb;
			}
		}

		cmd->size = 4;
		cmd->buf[0] = GS;
		cmd->buf[1] = 'k';
		cmd->buf[2] = 9;
		cmd->buf[3] = qrcode->width;
#endif
		//write_command(0, &cmd, p, idx);
		QRcode_free(qrcode);
		QRinput_free(stream);

	}

	return errnum;
}

