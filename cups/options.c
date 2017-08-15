/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : options.c
* DESCRIPTION   : CUPS options routines
* CVS           : $Id: options.c,v 1.9 2009/01/16 16:14:35 pierre Exp $
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
*   15may2006   nico    Prolog/epilog command generation moved to command.c
*   19may2006   nico    Added compress CUPS option
*   06jun2006   nico    Added pagemode CUPS option
*   11oct2006   nico    Added maxlength CUPS option
*   22fev2008   nico    Added checkneop CUPS option
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>

#include <aps/aps.h>

#include "options.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/*printer configuration*/
int     printer_model;
int     printer_type;
int     printer_width;          /*bytes*/

/*common options*/
int     prbaudrate;
int     prhandshake;
int     parmode;
int     prtimeout;
int     dynadiv;                /*black bytes*/
int     maxspeed;               /*mm/s*/
int     intensity;              /*%*/
int     optprint;
int     compress;
int     font;
int     process;
int     finalcut;
int     pagemode;
int     ticketmode;
int     rmtop;                  /*dotlines*/
int     fwdfeed;                /*dotlines*/
int     backfeed;               /*dotlines*/
int     maxlength;              /*dotlines*/
int     maxlengthmm;              /*dotlines*/
int     checkneop;
int     charspacing;            /*pixels*/
int     linespacing;            /*dotlines*/
char*   font_path; /*path of aps font file*/

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  get_opt_int
Purpose   :  Retrieve option value as an integer
Inputs    :  ppd : PPD file structure
             keyword : option name
Outputs   :  <>
Return    :  option value or -1 if option is not found
-----------------------------------------------------------------------------*/
static int get_opt_int(ppd_file_t *ppd,const char *keyword)
{
        ppd_choice_t *choice;

        choice = ppdFindMarkedChoice(ppd,keyword);

        if (choice!=NULL)
                return atoi(choice->choice);
        else
                return -1;
}

/*-----------------------------------------------------------------------------
Name      :  get_opt_bool
Purpose   :  Retrieve option value as a boolean
Inputs    :  ppd : PPD file structure
             keyword : option name
Outputs   :  <>
Return    :  1 if value is true, 0 if value is false or -1 if error
-----------------------------------------------------------------------------*/
static int get_opt_bool(ppd_file_t *ppd,const char *keyword)
{
        ppd_choice_t *choice;

        choice = ppdFindMarkedChoice(ppd,keyword);

        if (choice!=NULL) {
                if (strcmp(choice->choice,"False")==0)
                        return 0;
                else
                        return 1;
        }
        else
                return -1;
}
 
/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  debug
Purpose   :  Log debug message in /var/log/cups/error_log
Inputs    :  s    : custom debug string
             port : APS port structure
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void debug(const char *s,void *port)
{
        if (port==NULL) {
                fprintf(stderr,"DEBUG: %s (cups:%s)\n",s,
                                ippErrorString(cupsLastError()));
        }
        else {
        	fprintf(stderr,"DEBUG: %s (cups:%s,aps:%s)\n",s,
                                ippErrorString(cupsLastError()),
                                aps_get_strerror_full(aps_get_error(port),port));
        }
}

/*-----------------------------------------------------------------------------
Name      :  error
Purpose   :  Exit program with error code
             Log error in /var/log/cups/error_log before exit
Inputs    :  s : custom error string
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void error(const char *s)
{
	
	fprintf(stderr,"ERROR: APS backend => %s (cups:%s)\n",s,
                        ippErrorString(cupsLastError()));

	fprintf(stderr,"ERROR: APS backend => errno: %s \n",strerror(errno));
		

        exit(1);
}

/*-----------------------------------------------------------------------------
Name      :  get_options

Purpose   :  Retrieve current printing options
             Retrieve marked options from PPD file
             Override options with command-line string

Inputs    :  opt : command-line options
             
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void get_options(const char *opt)
{
        const char *ppd_name;
        ppd_file_t *ppd;
        int num_options;
        cups_option_t *options;
        int errnum;

        /*open printer PPD file and mark options*/
        ppd_name = getenv("PPD");
        if (ppd_name == NULL)
                error("PPD environment variable not set");

        ppd = ppdOpenFile(ppd_name);
        if (ppd==NULL)
                error("ppdOpenFile failed");

        ppdMarkDefaults(ppd);

        options = NULL;
        num_options = cupsParseOptions(opt,0,&options);

        if (options!=NULL && num_options!=0) {

            const char *p =cupsGetOption("font_path",num_options,options);
            font_path = NULL;
            if (p!= NULL)
            {
                font_path = malloc(strlen(p));
                strcpy(font_path,p);
            }
            cupsMarkOptions(ppd,num_options,options);
            cupsFreeOptions(num_options,options);
        }

        /*retrieve printer configuration*/
        printer_model = ppd->model_number;
        
        errnum = aps_get_model_type(printer_model);
        if (errnum<0)
                error(aps_strerror(errnum));
        else
                printer_type = errnum;

        errnum = aps_get_model_width(printer_model);
        if (errnum<0)
                error(aps_strerror(errnum));
        else
                printer_width = errnum/8;
        
        /*retrieve common options*/
        prbaudrate      = get_opt_int(ppd,"prbaudrate");
        prhandshake     = get_opt_int(ppd,"prhandshake");
        parmode         = get_opt_int(ppd,"parmode");
        prtimeout       = get_opt_int(ppd,"prtimeout");
        dynadiv         = get_opt_int(ppd,"dynadiv");
        maxspeed        = get_opt_int(ppd,"maxspeed");
        intensity       = get_opt_int(ppd,"intensity");
        optprint        = get_opt_bool(ppd,"optprint");
        //compress        = get_opt_bool(ppd,"compress");
        finalcut        = get_opt_int(ppd,"finalcut");
        font            = get_opt_int(ppd,"APS_font");
        process         = get_opt_bool(ppd,"process");
        rmtop           = get_opt_bool(ppd,"rmtop");
        pagemode        = get_opt_bool(ppd,"pagemode");
        ticketmode      = get_opt_bool(ppd,"ticketmode");
        fwdfeed         = get_opt_int(ppd,"fwdfeed");
        backfeed        = get_opt_int(ppd,"backfeed");
        maxlength       = get_opt_int(ppd,"maxlength");
        maxlengthmm     = get_opt_int(ppd,"maxlengthmm");
        checkneop       = get_opt_bool(ppd,"checkneop");
        charspacing     = get_opt_int(ppd,"charspacing");
        linespacing     = get_opt_int(ppd,"linespacing");

        /*retrieve printer-specific options*/
        /*TODO: not implemented!*/
        
        ppdClose(ppd);

}

/*-----------------------------------------------------------------------------
Name      :  free_options
Purpose   :  free memory use by options
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void 	free_options(void)
{
	if (font_path != NULL)
	{
		free(font_path);
	}
}

/*-----------------------------------------------------------------------------
Name      :  dump_options
Purpose   :  Dump print options as debug lines in CUPS error log
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
void dump_options(void)
{
        fprintf(stderr,"DEBUG: CUPS printing options\n");
        fprintf(stderr,"DEBUG: prbaudrate   = %d\n",prbaudrate);
        fprintf(stderr,"DEBUG: prhandshake  = %d\n",prhandshake);
        fprintf(stderr,"DEBUG: parmode      = %d\n",parmode);
        fprintf(stderr,"DEBUG: rmtop        = %d\n",rmtop);
        fprintf(stderr,"DEBUG: prtimeout    = %d\n",prtimeout);
        fprintf(stderr,"DEBUG: dynadiv      = %d\n",dynadiv);
        fprintf(stderr,"DEBUG: maxspeed     = %d\n",maxspeed);
        fprintf(stderr,"DEBUG: intensity    = %d\n",intensity);
        fprintf(stderr,"DEBUG: optprint     = %d\n",optprint);
        //fprintf(stderr,"DEBUG: compress   = %d\n",compress);
        fprintf(stderr,"DEBUG: font         = %d\n",font);
        fprintf(stderr,"DEBUG: process      = %d\n",process);
        fprintf(stderr,"DEBUG: finalcut     = %d\n",finalcut);
        fprintf(stderr,"DEBUG: pagemode     = %d\n",pagemode);
        fprintf(stderr,"DEBUG: fwdfeed      = %d\n",fwdfeed);
        fprintf(stderr,"DEBUG: backfeed     = %d\n",backfeed);
        fprintf(stderr,"DEBUG: maxlength    = %d\n",maxlength);
        fprintf(stderr,"DEBUG: maxlengthmm  = %d\n",maxlengthmm);
        fprintf(stderr,"DEBUG: checkneop    = %d\n",checkneop);
        fprintf(stderr,"DEBUG: charspacing  = %d\n",charspacing);
        fprintf(stderr,"DEBUG: linespacing  = %d\n",linespacing);

        fprintf(stderr,"DEBUG: printer_width= %d bytes\n",printer_width);
        if (font_path != NULL)
            fprintf(stderr,"DEBUG: font_path    = %s\n",font_path);
        else
            fprintf(stderr,"DEBUG: font_path    = NONE\n");

}

