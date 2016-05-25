/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : ticket.h
* DESCRIPTION   : Ticket formatting routines
* CVS           : $Id: ticket.h,v 1.2 2006/06/16 15:24:06 nicolas Exp $
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

#ifndef _TICKET_H
#define _TICKET_H

#ifdef __cplusplus
extern "C" {
#endif

void    enter_raw_mode(void);

void    write_command(int raw,const command_t *cmd,const void *buf,int size);

void    write_prolog(int raw);
void    write_epilog(int raw);

#ifdef __cplusplus
}
#endif

#endif /*_TICKET_H*/

