/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : command.h
* DESCRIPTION   : Command set routines
* CVS           : $Id: command.h,v 1.8 2008/06/09 14:46:45 nicolas Exp $
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
******************************************************************************/

#ifndef _COMMAND_H
#define _COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

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
} control_code_t;

#define CMD_BUFSIZE     8       /*bytes*/

typedef struct {
        unsigned char   buf[CMD_BUFSIZE];
        int             size;
        int             answer;
} command_t;

int     cmd_reset(int type,command_t *cmd);

int     cmd_enter_full_mrs_mode(int type,command_t *cmd);

int     cmd_get_status(int type,command_t *cmd);
int     cmd_get_status_neop(int type,command_t *cmd);

int     cmd_set_serial_opt(int type,command_t *cmd,int baudrate,int handshake,int adjust);

int     cmd_set_font(int type,command_t *cmd,int n);
int     cmd_set_dynamic_division(int type,command_t *cmd,int n);
int     cmd_set_maximum_speed(int type,command_t *cmd,int speed);
int     cmd_set_intensity(int type,command_t *cmd,int percent);
int     cmd_set_compression(int type,command_t *cmd,int flag);
int     cmd_set_char_spacing(int type,command_t *cmd,int n);
int     cmd_set_line_spacing(int type,command_t *cmd,int n);

int cmd_lpm_end_of_ticket(int type,command_t *cmd);
int     cmd_full_cut(int type,command_t *cmd);
int     cmd_partial_cut(int type,command_t *cmd);

int     cmd_feed_forward(int type,command_t *cmd,int dotlines);
int     cmd_feed_backward(int type,command_t *cmd,int dotlines);

int     cmd_shift_dotline(int type,command_t *cmd,int nbytes);
int     cmd_print_dotline(int type,command_t *cmd,int nbytes);

int cmd_lpm_calibrate(int type,command_t *cmd);

/*USB control requests*/
int     cmd_usb_get_status(int type,aps_usb_ctrltransfer_t *ctrl);
int     cmd_usb_hard_reset(int type,aps_usb_ctrltransfer_t *ctrl);
int cmd_mrs_qrcode(int version, int level, int mode, int casesensitivity, char * data, char ** qr_data_buf, int * qrbuf_len, command_t * cmd);

int cmd_mrs_qrcode_bindata(int version, int level, int mode, char * data, int dlen, char ** qr_data_buf, int * qrbuf_len, command_t * cmd, int * qr_width);

#ifdef __cplusplus
}
#endif

#endif /*_COMMAND_H*/

