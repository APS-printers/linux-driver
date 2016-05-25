/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : options.h
* DESCRIPTION   : CUPS options routines
* CVS           : $Id: options.h,v 1.7 2008/07/09 14:22:29 pierre Exp $
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
*   31jan2006   nico    Initial revision
******************************************************************************/

#ifndef _OPTIONS_H
#define _OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif


/*final cut modes*/
typedef enum {
        FINALCUT_NONE           = 0,
        FINALCUT_PARTIAL        = 1,
        FINALCUT_FULL           = 2
} finalcut_t;

/*printer configuration*/
extern int      printer_model;
extern int      printer_type;
extern int      printer_width;          /*bytes*/

/*common options*/
extern int      prbaudrate;
extern int      prhandshake;
extern int      parmode;
extern int      prtimeout;
extern int      dynadiv;                /*black bytes*/
extern int      maxspeed;               /*mm/s*/
extern int      intensity;              /*%*/
extern int      optprint;
extern int      compress;
extern int      font;
extern int      process;
extern int      finalcut;
extern int      pagemode;
extern int      ticketmode;
extern int      rmtop;                  /*dotlines*/
extern int      fwdfeed;                /*dotlines*/
extern int      backfeed;               /*dotlines*/
extern int      maxlength;              /*dotlines*/
extern int      maxlengthmm;            /*dotlines*/
extern int      checkneop;
extern int      charspacing;            /*pixels*/
extern int      linespacing;            /*dotlines*/
extern char     *font_path; 		/*path of aps font file*/

void    debug(const char *s,void *port);
void    error(const char *s);

void    get_options(const char *opt);
void 	free_options(void);
void    dump_options(void);

#ifdef __cplusplus
}
#endif

#endif /*_OPTIONS_H*/

