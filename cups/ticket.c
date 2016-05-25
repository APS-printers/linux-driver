/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : ticket.c
* DESCRIPTION   : Ticket formatting routines
* CVS           : $Id: ticket.c,v 1.5 2008/06/09 14:46:45 nicolas Exp $
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
*   08jun2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aps/aps.h>

#include "command.h"
#include "options.h"
#include "ticket.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  enter_raw_mode
Purpose   :  Signal backend that ticket data will be sent in raw mode
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
extern int dump;
void enter_raw_mode(void)
{
        int n = -1;

        fwrite(&n,sizeof(int),1,stdout);
}

/*-----------------------------------------------------------------------------
Name      :  write_command
Purpose   :  Write command to stdout
Inputs    :  raw  : issue raw command if true
             cmd  : command structure
             buf  : command data buffer
             size : command data buffer size in bytes
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void write_command(int raw,const command_t *cmd,const void *buf,int size)
{
        /*send block header in non-raw mode*/
        if (!raw) {
                int n = cmd->size+size;
                fwrite(&n,sizeof(int),1,stdout);
        }

        /*send command header*/
        fwrite(cmd->buf,cmd->size,1,stdout);

        /*send command data only if buffer is specified*/
        if (buf!=NULL) {
                fwrite(buf,size,1,stdout);
        }
}

/*-----------------------------------------------------------------------------
Name      :  write_prolog
Purpose   :  Write ticket prolog
Inputs    :  raw : issue raw commands if true
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void write_prolog(int raw)
{
        command_t cmd;

        /*backward feed in tiquet mode */
        if (ticketmode!=0) {
            if (backfeed!=0) {
                cmd_feed_backward(printer_type,&cmd,backfeed);
                write_command(raw,&cmd,NULL,0);
            }
        }

        if (printer_model==MODEL_CP205MRS) {
                cmd_enter_full_mrs_mode(printer_type,&cmd);
                write_command(raw,&cmd,NULL,0);
        }
        
        if (font!=-1) {
                cmd_set_font(printer_type,&cmd,font);
                write_command(raw,&cmd,NULL,0);
        }
        if (dynadiv!=-1) {
                cmd_set_dynamic_division(printer_type,&cmd,dynadiv);
                write_command(raw,&cmd,NULL,0);
        }
        if (maxspeed!=-1) {
                cmd_set_maximum_speed(printer_type,&cmd,maxspeed);
                write_command(raw,&cmd,NULL,0);
        }
        if (intensity!=-1) {
                cmd_set_intensity(printer_type,&cmd,intensity);
                write_command(raw,&cmd,NULL,0);
        }
        /*if (compress!=-1) {
                cmd_set_compression(printer_type,&cmd,compress);
                write_command(raw,&cmd,NULL,0);
        }*/
        if (charspacing!=-1) {
                cmd_set_char_spacing(printer_type,&cmd,charspacing);
                write_command(raw,&cmd,NULL,0);
        }
        if (linespacing!=-1) {
                cmd_set_line_spacing(printer_type,&cmd,linespacing);
                write_command(raw,&cmd,NULL,0);
        }

        fflush(stdout);
}

/*-----------------------------------------------------------------------------
Name      :  write_epilog
Purpose   :  Write ticket epilog
Inputs    :  raw : issue raw commands if true
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void write_epilog(int raw)
{
    command_t cmd;

    if (ticketmode!=0) {
        cmd_lpm_end_of_ticket(printer_type,&cmd);
        write_command(raw,&cmd,NULL,0);
    }

    /*forward feed before cut*/
    if (fwdfeed!=0) {
            cmd_feed_forward(printer_type,&cmd,fwdfeed);
            write_command(raw,&cmd,NULL,0);
    }

    if (ticketmode==0) {
        switch (finalcut) {
        case FINALCUT_NONE:
                break;
        case FINALCUT_PARTIAL:
                cmd_partial_cut(printer_type,&cmd);
                write_command(raw,&cmd,NULL,0);
                break;
        case FINALCUT_FULL:
                cmd_full_cut(printer_type,&cmd);
                write_command(raw,&cmd,NULL,0);
                break;
        }

        /*backward feed after cut*/
        if (backfeed!=0) {
                cmd_feed_backward(printer_type,&cmd,backfeed);
                write_command(raw,&cmd,NULL,0);
        }
    }

    fflush(stdout);
}

