int dump;
/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : rastertoaps.c
*
* DESCRIPTION   : CUPS filter for APS printers
*                 Converts CUPS internal RIP format into APS commands
*                 APS command set is selected depending on cupsModelNumber
*                 attribute of the PPD file
*
* CVS           : $Id: rastertoaps.c,v 1.14 2008/07/09 14:22:29 pierre Exp $
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
*   19may2006   nico    Added HRS/KCP bitmap compression support
*   08jun2006   nico    Added blank dotlines compression and ticket/page mode
*   12jun2006   nico    Added cancel support
*   16aug2006   nico    Reset dotline shift amount at end of ticket
*   11oct2006   nico    Added maximum ticket length option
*                       (full cut after maxlength dotlines)
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <cups/cups.h>
#include <cups/raster.h>

#include <aps/aps.h>

#include "command.h"
#include "compress.h"
#include "options.h"
#include "ticket.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static  sig_atomic_t    cancel_flag;

#define BLANK_BUFSIZE   256             /*bytes*/

static unsigned char    blank_buf[BLANK_BUFSIZE];

static  int     dotlines_counter;
static  int     blank_counter;
static  int     shift_amount;

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  cancel_handler
Purpose   :  Cancel signal handler (traps SIGTERM)
Inputs    :  signum : signal number
Outputs   :  Updates global cancel_flag
Return    :  <>
-----------------------------------------------------------------------------*/
static void cancel_handler(int signum)
{
        (void)signum;

        cancel_flag = 1;
}

/*-----------------------------------------------------------------------------
Name      :  next_dotline
Purpose   :  Increment dotlines counter, perform full cut if necessary
Inputs    :  <>
Outputs   :  Updates global dotlines_counter
Return    :  <>
-----------------------------------------------------------------------------*/
static void next_dotline(void)
{
        if (dotlines_counter==maxlength + maxlengthmm) {
                aps_error_t errnum;
                command_t cmd;

                /*reset dotlines counter*/
                dotlines_counter = 0;
		if (ticketmode != 0)
		{
			cmd_lpm_end_of_ticket(printer_type, &cmd);
			write_command(0,&cmd,NULL,0);
		}
		else
		{

			/*perform full cut*/
			errnum = cmd_full_cut(printer_type,&cmd);

			if (errnum<0) {
				error(aps_strerror(errnum));
			}
			else {
				write_command(0,&cmd,NULL,0);
			}
		}
        }
        else {
                dotlines_counter++;
        }
}

/*-----------------------------------------------------------------------------
Name      :  print_blank_normal
Purpose   :  Print blank dotlines using normal print command
Inputs    :  nbytes : width of dotline in bytes
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void print_blank_normal(int nbytes)
{
        aps_error_t errnum;
        command_t cmd;

        errnum = cmd_print_dotline(printer_type,&cmd,nbytes);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(0,&cmd,blank_buf,nbytes);
        }

        fflush(stdout);
}

/*-----------------------------------------------------------------------------
Name      :  print_blank_opt
Purpose   :  Print blank dotlines using optimized technique
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void print_blank_opt(void)
{
        aps_error_t errnum;
        command_t cmd;

        errnum = cmd_print_dotline(printer_type,&cmd,1);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(0,&cmd,blank_buf,1);
        }

        fflush(stdout);
}

/*-----------------------------------------------------------------------------
Name      :  print_blank
Purpose   :  Write APS commands to print blank dotlines
Inputs    :  n      : number of dotlines to print
             nbytes : width of dotline in bytes
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void print_blank(int n,int nbytes)
{
        while (n--) {
                next_dotline();

                if (optprint) {
                        print_blank_opt();
                }
                else {
                        print_blank_normal(nbytes);
                }
        }
}

/*-----------------------------------------------------------------------------
Name      :  print_dotline
Purpose   :  Write APS command to print dotline
Inputs    :  buf    : dotline buffer
             nbytes : width of dotline in bytes
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void print_dotline(const unsigned char *buf,int nbytes)
{
        aps_error_t errnum;
        command_t cmd;

        next_dotline();

        errnum = cmd_print_dotline(printer_type,&cmd,nbytes);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(0,&cmd,buf,nbytes);
        }

        fflush(stdout);
}

/*-----------------------------------------------------------------------------
Name      :  shift_dotline
Purpose   :  Write APS command to shift dotlines
Inputs    :  nbytes : right shift amount in bytes
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void shift_dotline(int nbytes)
{
        aps_error_t errnum;
        command_t cmd;

        errnum = cmd_shift_dotline(printer_type,&cmd,nbytes);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(0,&cmd,NULL,0);
        }

        fflush(stdout);
}

/*-----------------------------------------------------------------------------
Name      :  count_leading_blank
Purpose   :  Count number of leading blank bytes in dotline
Inputs    :  buf    : dotline buffer
             nbytes : width of dotline in bytes
Outputs   :  <>
Return    :  Number of leading blank bytes
-----------------------------------------------------------------------------*/
static int count_leading_blank(const unsigned char *buf,int nbytes)
{
        int n = 0;

        while (n<nbytes && buf[n]==0) {
                n++;
        }

        return n;
}

/*-----------------------------------------------------------------------------
Name      :  count_trailing_blank
Purpose   :  Count number of trailing blank bytes in dotline
Inputs    :  buf    : dotline buffer
             nbytes : width of dotline in bytes
Outputs   :  <>
Return    :  Number of trailing blank bytes
-----------------------------------------------------------------------------*/
static int count_trailing_blank(const unsigned char *buf,int nbytes)
{
        int n = 0;

        while (n<nbytes && buf[nbytes-n-1]==0) {
                n++;
        }

        return n;
}

/*-----------------------------------------------------------------------------
Name      :  process_page
Purpose   :  Process one CUPS page
Inputs    :  ras    : CUPS raster structure
             header : CUPS page header structure
Outputs   :  Updates global blank_counter
Return    :  <>
-----------------------------------------------------------------------------*/
static void process_page(cups_raster_t *ras,cups_page_header_t *header)
{
        int nbytes;
        int y;
        unsigned char *buf;
	int rmtop_once;
	rmtop_once = rmtop;

        /*compute printer dotline size*/
        if ((int)header->cupsBytesPerLine>printer_width)
                nbytes = printer_width;
        else
                nbytes = header->cupsBytesPerLine;

        /*allocate CUPS dotline buffer*/
        buf = malloc(10 * header->cupsBytesPerLine);
        if (buf==NULL) {
                error("Cannot allocate dotline buffer");
        }

        /*read dotlines and print APS commands to stdout*/
        for (y=0; y / 10 <(int)header->cupsHeight / 10 && !cancel_flag; y += 10) {
                int n;
                int n1;
                int n2;
		int i;
                
                n = cupsRasterReadPixels(ras,buf, 10 * header->cupsBytesPerLine);
               
                if (n!=(int)10 * header->cupsBytesPerLine) {
                        error("cupsRasterReadPixels did not read enough data");
                }

		for (i = 0; i < 10; i ++)
		{

                n1 = count_leading_blank(buf + i * header->cupsBytesPerLine,nbytes);

                if (n1==nbytes) {
                        blank_counter++;
                }
                else {
                        /*print queued blank dotlines*/
                        if (blank_counter && !rmtop_once) {
                                print_blank(blank_counter,nbytes);
                                blank_counter = 0;
                        }
			rmtop_once = 0;

                        /*print dotline*/
                        if (optprint) {
                                switch (printer_type) {
                                case APS_MRS:
                                case APS_HRS:
                                case APS_KCP:
                                        if (shift_amount!=n1) {
                                                shift_dotline(n1);
                                                shift_amount = n1;
                                        }
                                        break;
                                default:
                                        n1 = 0;
                                        break;
                                }

                                n2 = count_trailing_blank(buf + i * header->cupsBytesPerLine,nbytes);

                                print_dotline(&(buf + i * header->cupsBytesPerLine)[n1],nbytes-n1-n2);
                        }
                        else {
                                print_dotline(buf + i * header->cupsBytesPerLine,nbytes);
                        }
                }
		}
        }



        /*read dotlines and print APS commands to stdout*/
        for (; y<(int)header->cupsHeight && !cancel_flag; y++) {
                int n;
                int n1;
                int n2;
                
                n = cupsRasterReadPixels(ras,buf,header->cupsBytesPerLine);
               
                if (n!=(int)header->cupsBytesPerLine) {
                        error("cupsRasterReadPixels did not read enough data");
                }

                n1 = count_leading_blank(buf,nbytes);

                if (n1==nbytes) {
                        blank_counter++;
                }
                else {
                        /*print queued blank dotlines*/
                        if (blank_counter && !rmtop_once) {
                                print_blank(blank_counter,nbytes);
                                blank_counter = 0;
                        }
			rmtop_once = 0;

                        /*print dotline*/
                        if (optprint) {
                                switch (printer_type) {
                                case APS_MRS:
                                case APS_HRS:
                                case APS_KCP:
                                        if (shift_amount!=n1) {
                                                shift_dotline(n1);
                                                shift_amount = n1;
                                        }
                                        break;
                                default:
                                        n1 = 0;
                                        break;
                                }

                                n2 = count_trailing_blank(buf,nbytes);

                                print_dotline(&buf[n1],nbytes-n1-n2);
                        }
                        else {
                                print_dotline(buf,nbytes);
                        }
                }
        }



        /*free CUPS dotline buffer*/
        free(buf);
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  main
Purpose   :  Program main function
Inputs    :  argc : number of command-line arguments (including program name)
             argv : array of command-line arguments
Outputs   :  <>
Return    :  0 if successful, 1 if program failed
-----------------------------------------------------------------------------*/
#define DEBUG_DUMP_FILE_1	"/tmp/aps-raster"
int main(int argc,char** argv)
{
    int fd;
    struct sigaction sa;
    cups_raster_t *ras;
    cups_page_header_t header;
    int page;

#ifdef DEBUG_DUMP
    dump = open(DEBUG_DUMP_FILE_1,O_CREAT|O_WRONLY);

    if (dump>0) {
        fchmod(dump,0777);
    }
#endif /*DEBUG_DUMP*/


    setbuf(stderr,NULL);

    debug("rastertoaps filter started",NULL);

    /*record version information*/
    fprintf(stderr,"DEBUG: rastertoaps compiled with APS library version %d.%d.%d\n",
            APS_MAJOR,
            APS_MINOR,
            APS_BUGFIX);

    /*check arguments*/
    if (argc<6 || argc>7) {
        fputs("ERROR: rastertoaps job-id user title copies options [file]\n",stderr);
        return 1;
    }

    /*retrieve options*/
    get_options(argv[5]);

    /*open page stream*/
    if (argc==7) {
        if ((fd = open(argv[6],O_RDONLY))==-1) {
            error("Unable to open raster file");
        }
    }
    else
        fd = 0; /*stdin*/

    /*install cancel handler*/
    cancel_flag = 0;

    memset(&sa,0,sizeof(sa));
    sa.sa_handler = &cancel_handler;
    sigaction(SIGTERM,&sa,NULL);

    /*open CUPS raster stream*/
    ras = cupsRasterOpen(fd,CUPS_RASTER_READ);
    if (ras==NULL) {
        error("cupsRasterOpen failed");
    }

    /*write ticket prolog*/
    write_prolog(0);

    /*read and process pages*/
    memset(blank_buf,0,sizeof(blank_buf));

    dotlines_counter = 0;
    blank_counter = 0;
    shift_amount = -1;

    page = 0;

    while (cupsRasterReadHeader(ras,&header) && !cancel_flag) {

        /*do page accounting*/
        page++;
        fprintf(stderr,"PAGE: %d 1\n",page);

        /*process page*/
        process_page(ras,&header);
    }

    /*reset dotline shift amount*/
    if (optprint && shift_amount>0) {
        switch (printer_type) {
            case APS_MRS:
            case APS_HRS:
            case APS_KCP:
                shift_dotline(0);
                break;
            default:
                break;
        }
    }

    if (cancel_flag) {
        debug("Print job was cancelled",NULL);
    }
    else {
        /*print trailing blank dotlines in page mode*/
        if (blank_counter && pagemode) {
            print_blank(blank_counter,printer_width);
        }

        /*write ticket epilog*/
        write_epilog(0);
    }

    /*close CUPS raster stream*/
    cupsRasterClose(ras);

    /*uninstall cancel handler*/
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGTERM,&sa,NULL);

    /*close input file*/
    if (fd!=0) {
        close(fd);
    }

    free_options();

    debug("rastertoaps filter finished",NULL);
#ifdef DEBUG_DUMP
    if (dump>0) {
	    close(dump);
    }
#endif /*DEBUG_DUMP*/
    return 0;
}


