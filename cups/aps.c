/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : aps.c
 * DESCRIPTION   : CUPS backend for APS printers
 *                 Use APS library to send data to printers
 * CVS           : $Id: aps.c,v 1.28 2008/07/09 14:22:29 pierre Exp $
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
 *   07feb2006   nico    Initial revision
 *   15may2006   nico    Added USB status query
 *   12jun2006   nico    Added cancel support
 *                       Added robust error handling code
 *   16aug2006   nico    Increased serial and parallel status read timeout to 5s
 *   07oct2006   nico    Added NEOP status polling (MRS/HRS/KCP)
 *                       NEOP error is non-fatal (does not prevent printing)
 *   31jan2007   nico    Adjust FIFO thresholds on RS232 MRS printers
 *   18feb2008   nico    Decreased polling period to 100ms in wait state
 *                       Corrected timeout handling in wait state
 *                       Reports printing status via CUPS interface
 *                       Added wait state after ticket data has been sent
 *   25mar2008   nico    Added printer-state-reasons state output
 *   09jun2008   nico    Added wait USB state
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cups/cups.h>

#include <aps/aps.h>

#include "command.h"
#include "options.h"

#undef DEBUG_DUMP
#define DEBUG_DUMP_FILE         "/tmp/aps"


/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static  sig_atomic_t    cancel_flag;

#define PRINTERS_MAX    32

#define PRINT_BUFSIZE   4096    /*bytes*/

static unsigned char    print_buf[PRINT_BUFSIZE];

static void *   port;

static aps_serial_baudrate_t    defbaudrate;
static aps_serial_handshake_t   defhandshake;
static aps_parallel_mode_t      defparmode;

static int      printer_ready;

#ifdef DEBUG_DUMP
static int dump;
#endif /*DEBUG_DUMP*/

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
Name      :  reset_printer
Purpose   :  Reset printer to default state, regardless of current state
Printer port is closed on exit
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int reset_printer(void)
{
    aps_error_t errnum;
    int type;
    command_t cmd;
    aps_usb_ctrltransfer_t ctrl;

    if ((type = aps_get_port_type(port))<0) {
        return type;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }

    /*perform reset action*/
    switch (type) {
        case APS_SERIAL:
            if ((errnum = cmd_reset(printer_type,&cmd))<0) {
                return errnum;
            }
            if ((errnum = aps_write_rt(port,cmd.buf,cmd.size))<0) {
                return errnum;
            }
            break;

        case APS_PARALLEL:
            if ((errnum = aps_parallel_reset(port))<0) {
                return errnum;
            }
            break;

        case APS_USB:
            if ((errnum = cmd_usb_hard_reset(printer_type,&ctrl))<0) {
                return errnum;
            }

            /*this USB vendor requests executes correctly on HRS printers
             *but for some reason is reported as failed
             *we ignore the error code of this code for the moment - the
             *aps_open() call below will confirm whether the printer has
             *reset correctly or not
             */

            /*
               if ((errnum = aps_usb_control(port,&ctrl))<0) {
               return errnum;
               }
               */

            aps_usb_control(port,&ctrl);
            break;
    }

    /*close printer port*/
    /*
       if (type==APS_USB) {
       if ((errnum = aps_usb_kill(port))<0) {
       return errnum;
       }
       }
       else {
       if ((errnum = aps_close(port))<0) {
       return errnum;
       }
       }
       */

    if ((errnum = aps_close(port))<0) {
        return errnum;
    }

    /*allow some time for printer to reset*/
    sleep(2); /*s*/

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  setup_printer
Purpose   :  Save current port settings (defaults) and setup port for printing
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int setup_printer(void)
{
    aps_error_t errnum;
    int type;
    command_t cmd;

    if ((type = aps_get_port_type(port))<0) {
        return type;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }

    switch (type) {
        case APS_SERIAL:
            /*save current settings*/
            if ((int)(defbaudrate = aps_serial_get_baudrate(port))<0) {
                return defbaudrate;
            }
            if ((int)(defhandshake = aps_serial_get_handshake(port))<0) {
                return defhandshake;
            }

            /*setup printing settings as defaults if required*/
            if (prbaudrate==-1) {
                prbaudrate = defbaudrate;
            }
            if (prhandshake==-1) {
                prhandshake = defhandshake;
            }

            /*build set serial settings command*/
            if ((errnum = cmd_set_serial_opt(printer_type,&cmd,prbaudrate,prhandshake,1))<0) {
                return errnum;
            }

            /*update serial settings*/
            if ((errnum = aps_write(port,cmd.buf,cmd.size))<0) {
                return errnum;
            }
            if ((errnum = aps_sync(port))<0) {
                return errnum;
            }

            if ((errnum = aps_serial_set_baudrate(port,prbaudrate))<0) {
                return errnum;
            }
            if ((errnum = aps_serial_set_handshake(port,prhandshake))<0) {
                return errnum;
            }

            break;

        case APS_PARALLEL:
            /*save current settings*/
            if ((int)(defparmode = aps_parallel_get_mode(port))<0) {
                return defparmode;
            }

            /*setup printing settings as defaults if required*/
            if (parmode==-1) {
                parmode = defparmode;
            }

            /*update parallel settings*/
            if ((errnum = aps_parallel_set_mode(port,parmode))<0) {
                return errnum;
            }

            break;

        case APS_USB:
            /*nothing to set*/
            break;
    }        

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  setup_defaults
Purpose   :  Revert to default port settings
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int setup_defaults(void)
{
    aps_error_t errnum;
    int type;
    command_t cmd;

    if ((type = aps_get_port_type(port))<0) {
        return type;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }

    switch (type) {
        case APS_SERIAL:
            /*build set serial settings command*/
            if ((errnum = cmd_set_serial_opt(printer_type,&cmd,defbaudrate,defhandshake,0))<0) {
                return errnum;
            }

            /*revert to default serial settings*/
            if ((errnum = aps_write(port,cmd.buf,cmd.size))<0) {
                return errnum;
            }
            if ((errnum = aps_sync(port))<0) {
                return errnum;
            }

            if ((errnum = aps_serial_set_baudrate(port,defbaudrate))<0) {
                return errnum;
            }
            if ((errnum = aps_serial_set_handshake(port,defhandshake))<0) {
                return errnum;
            }

            break;

        case APS_PARALLEL:
            /*revert to default parallel settings*/
            if ((errnum = aps_parallel_set_mode(port,defparmode))<0) {
                return errnum;
            }

            break;

        case APS_USB:
            /*nothing to set*/
            break;
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  list_devices
Purpose   :  List available printers on console
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void list_devices(void)
{
    aps_printer_t printers[PRINTERS_MAX];
    int n;
    int i;

    fprintf(stderr,"DEBUG: aps detecting printers\n");
    //n = aps_detect_printers(printers,PRINTERS_MAX);
    n = 0;

    if (n==0) {
        /*allow manual configuration*/
	    fprintf(stderr,"DEBUG: aps allowing manual\n");
        printf("direct aps \"unknown\" \"APS printer\"\n");
    }
    else {
        for (i=0; i<n; i++) {
            /*device class|device URI|model|description*/
	    fprintf(stderr,"DEBUG: aps detected printer %s\n", printers[i].uri);
            printf("direct %s \"%s\" \"%s\"\n",
                   printers[i].uri,
                   aps_get_model_name(printers[i].model),
                   printers[i].identity);
        }
    }
}

/*-----------------------------------------------------------------------------
Name      :  report_printer_state
Purpose   :  Report status by updating CUPS printer state message
Inputs    :  status : status buffer
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void report_printer_state(aps_status_t *status)
{
    int ok = 1;

    /*format printer-state-message*/

    fprintf(stderr,"INFO:");

    if (!status->online) {
        ok = 0;
        fprintf(stderr," Printer offline.");
    }
    if (status->printing) {
        ok = 0;
        fprintf(stderr," Printing.");
    }
    if (status->end_of_paper) {
        ok = 0;
        fprintf(stderr," End of paper.");
    }
    if (status->near_end_of_paper) {
        ok = 0;
        fprintf(stderr," Near end of paper.");
    }
    if (status->head_up) {
        ok = 0;
        fprintf(stderr," Printer head up.");
    }
    if (status->cover_open) {
        ok = 0;
        fprintf(stderr," Printer cover open.");
    }
    if (status->temp_error) {
        ok = 0;
        fprintf(stderr," Printer temperature error.");
    }
    if (status->supply_error) {
        ok = 0;
        fprintf(stderr," Printer supply error.");
    }
    if (status->mark_error) {
        ok = 0;
        fprintf(stderr," Mark detection error.");
    }
    if (status->cutter_error) {
        ok = 0;
        fprintf(stderr," Cutter error.");
    }
    if (status->mechanical_error) {
        ok = 0;
        fprintf(stderr," Mechanical error.");
    }
    if (status->presenter_error) {
        ok = 0;
        fprintf(stderr," Presenter error.");
    }
    if (status->front_exit_jam) {
        ok = 0;
        fprintf(stderr," Jam at front exit.");
    }
    if (status->retract_exit_jam) {
        ok = 0;
        fprintf(stderr," Jam at retract exit.");
    }

    if (ok) {
        fprintf(stderr," ok.");
    }

    fputc('\n',stderr);

    /*update printer-state-reasons*/
    /*we use best fit for standard keywords defined in RFC 2911*/
    if (status->online) {
        fprintf(stderr,"STATE: - offline\n");
    }
    else {
        fprintf(stderr,"STATE: + offline\n");
    }
    if (status->end_of_paper) {
        fprintf(stderr,"STATE: + media-empty\n");
    }
    else {
        fprintf(stderr,"STATE: - media-empty\n");
    }
    if (status->near_end_of_paper) {
        fprintf(stderr,"STATE: + media-low\n");
    }
    else {
        fprintf(stderr,"STATE: - media-low\n");
    }
    if (status->head_up || status->cover_open) {
        fprintf(stderr,"STATE: + cover-open\n");
    }
    else {
        fprintf(stderr,"STATE: - cover-open\n");
    }
    if (status->temp_error)
    {
        fprintf(stderr,"STATE: + fuser-over-temp\n");
    }
    else {
        fprintf(stderr,"STATE: - fuser-over-temp\n");
    }
    if (
        status->cutter_error            ||
        status->mechanical_error        ||
        status->presenter_error         ||
        status->front_exit_jam          ||
        status->retract_exit_jam
       )
    {
        fprintf(stderr,"STATE: + media-jam\n");
    }
    else {
        fprintf(stderr,"STATE: - media-jam\n");
    }
}

/*-----------------------------------------------------------------------------
Name      :  poll_serial_parallel_status
Purpose   :  Poll serial and parallel printer status using real-time command
Inputs    :  buf  : status buffer
max  : status buffer size in bytes
Outputs   :  Status buffer and size are modified
Return    :  Number of status bytes read or negative error code
-----------------------------------------------------------------------------*/
static int poll_serial_parallel_status(unsigned char *buf,int max)
{
    aps_error_t errnum;
    command_t cmd;

    /*build 'get status' command*/
    if ((errnum = cmd_get_status(printer_type,&cmd))<0) {
        return errnum;
    }

    if (cmd.answer>max) {
        return APS_IO_ERROR;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }
    if ((errnum = aps_set_read_timeout(port,5000))<0) {
        return errnum;
    }

    /*issue 'get status' command and retrieve printer status*/
    if ((errnum = aps_write_rt(port,cmd.buf,cmd.size))<0) {
        return errnum;
    }
    if ((errnum = aps_read(port,buf,cmd.answer))<0) {
        return errnum;
    }

    return cmd.answer;
}

/*-----------------------------------------------------------------------------
Name      :  poll_usb_status
Purpose   :  Poll USB printer status using USB control pipe
Inputs    :  buf  : status buffer
max  : status buffer size in bytes
Outputs   :  Status buffer and size are modified
Return    :  Number of status bytes read or negative error code
-----------------------------------------------------------------------------*/
static int poll_usb_status(unsigned char *buf,int max)
{
    aps_error_t errnum;
    aps_usb_ctrltransfer_t ctrl;

    /*build 'get status' USB request*/
    if ((errnum = cmd_usb_get_status(printer_type,&ctrl))<0) {
        return errnum;
    }

    if ((int)ctrl.wLength>max) {
        return APS_IO_ERROR;
    }

    ctrl.data = buf;

    /*setup timeout*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }

    /*issue 'get status' USB request*/
    if ((errnum = aps_usb_control(port,&ctrl))<0) {
        return errnum;
    }

    return ctrl.wLength;
}

/*-----------------------------------------------------------------------------
Name      :  poll_neop_status
Purpose   :  Poll near end-of-paper status
Inputs    :  <>
Outputs   :  <>
Return    :  1 if paper is low, 0 if ok or negative error code
-----------------------------------------------------------------------------*/
static int poll_neop_status(void)
{
    aps_error_t errnum;
    command_t cmd;
    unsigned char status;

    /*build 'get NEOP status' command*/
    if ((errnum = cmd_get_status_neop(printer_type,&cmd))<0) {
        return errnum;
    }

    if (cmd.answer>(int)sizeof(status)) {
        return APS_IO_ERROR;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }
    if ((errnum = aps_set_read_timeout(port,1000))<0) {
        return errnum;
    }

    /*issue 'get NEOP status' command and retrieve status*/
    if ((errnum = aps_write(port,cmd.buf,cmd.size))<0) {
        return errnum;
    }
    if ((errnum = aps_read(port,&status,cmd.answer))<0) {
        return errnum;
    }

    if (status==0) {
        return 0;       /*paper roll is ok*/
    }
    else if (status==1) {
        return 1;       /*paper roll is low*/
    }
    else {
        return APS_IO_ERROR;
    }
}

/*-----------------------------------------------------------------------------
Name      :  poll_status
Purpose   :  Poll printer status and update CUPS printer state message
Inputs    :  <>
Outputs   :  Updates global printer_ready flag
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int poll_status(void)
{
    aps_error_t errnum;
    aps_status_t status;
    unsigned char buf[4];
    int size;
    int type;

    /*printer is not ready if any kind of error occurs*/
    /*printer is not ready if currently printing*/
    printer_ready = 0;

    if ((type = aps_get_port_type(port))<0) {
        return type;
    }

    /*query printer status*/
    switch (type) {
        case APS_ETHERNET:
        case APS_SERIAL:
        case APS_PARALLEL:
            size = poll_serial_parallel_status(buf,sizeof(buf));
            break;
        case APS_USB:
            size = poll_usb_status(buf,sizeof(buf));
            break;
        default:
            size = APS_INVALID_PORT_TYPE;
            break;
    }

    if (size<0) {
        return size;
    }

    /*decode status information*/
    if ((errnum = aps_decode_status(printer_type,buf,size,&status))<0) {
        return errnum;
    }

    /*retrieve NEOP status (if enabled)*/
    if (checkneop) {
        if ((errnum = poll_neop_status())<0) {
            /* This error is not fatal: some printers don't
             * support NEOP, some printers have NEOP status
             * integrated in main status
             */
        }
        else {
            status.near_end_of_paper = errnum;
        }
    }

    report_printer_state(&status);

    /*compute ready/busy status*/
    printer_ready =
        status.online                   &&
        !status.printing                &&
        !status.end_of_paper            &&
        !status.head_up                 &&
        !status.cover_open              &&
        !status.temp_error              &&
        !status.supply_error            &&
        !status.mark_error              &&
        !status.cutter_error            &&
        !status.mechanical_error        &&
        !status.presenter_error         &&
        !status.front_exit_jam          &&
        !status.retract_exit_jam;

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  state_setup
Purpose   :  Setup port settings for printing
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int state_setup(void)
{
    aps_error_t errnum;

    debug("Entering setup state",port);

    if ((errnum = setup_printer())<0) {
        return errnum;
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  state_wait
Purpose   :  Wait until printer is ready to print
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int state_wait(void)
{
    aps_error_t errnum;
    time_t now;
    time_t timeout;

    debug("Entering wait state",port);

    /*compute timeout from current time (minimum 1s)*/
    now = time(NULL);

    if (prtimeout<1000) {
        timeout = now+1;
    }
    else {
        timeout = now+prtimeout/1000;
    }

    printer_ready = 0;

    while (!printer_ready) {
        if ((errnum = poll_status())<0) {
            return errnum;
        }

        /*wait some time before polling again*/
        if (!printer_ready) {
            struct timespec ts;

            ts.tv_sec = 0;
            ts.tv_nsec = 100000000; /*100ms*/

            nanosleep(&ts,NULL);
        }

        /*refresh current time*/
        now = time(NULL);

        if (prtimeout!=0 && now>timeout) {
            return APS_READ_TIMEOUT;
        }
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  state_wait_usb
Purpose   :  Wait until USB communication buffers are empty
Send a get status command through the data pipe and wait
got the answer. This indicates that the printer has processed all
incoming data and can respond to status queries, and thus means
the all communication buffes are clear.
Inputs    :  <>
Outputs   :  <>
Return    :  0 if communication buffers are empty (close() is ok), -1 if
timeout
-----------------------------------------------------------------------------*/
static int state_wait_usb(void)
{
    aps_error_t errnum;
    command_t cmd;
    int timeout;
    unsigned char buf[10];

    /*ignore that step if port type is not USB*/
    if (aps_get_port_type(port)!=APS_USB) {
        return 0;
    }

    debug("Entering wait state (flushing USB buffers)",port);

    /*build 'get status' command*/
    if ((errnum = cmd_get_status(printer_type,&cmd))<0) {
        return errnum;
    }
    if (cmd.answer>(int)sizeof(buf)) {
        return APS_IO_ERROR;
    }

    /*compute read timeout in milliseconds (minimum 1s)*/
    if (prtimeout<1000) {
        timeout = 1000;
    }
    else {
        timeout = prtimeout;
    }

    /*setup timeouts*/
    if ((errnum = aps_set_write_timeout(port,1000))<0) {
        return errnum;
    }
    if ((errnum = aps_set_read_timeout(port,timeout))<0) {
        return errnum;
    }

    /*issue 'get status' command*/
    if ((errnum = aps_write(port,cmd.buf,cmd.size))<0) {
        return errnum;
    }

    /*wait for answer to the 'get status' command*/
    /*read timeout has already been set above*/
    if ((errnum = aps_read(port,buf,cmd.answer))<0) {
        return errnum;
    }

    /*got the answer to the command*/
    /*communication buffers are empty now*/
    return 0;
}

/*-----------------------------------------------------------------------------
Name      :  write_raw
Purpose   :  Write raw data to printer
Inputs    :  fd : input file descriptor
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int write_raw(int fd)
{
	aps_error_t errnum;
	int n;

	/*write until end of input pipe*/
	while ((n = read(fd,print_buf,sizeof(print_buf)))>0) {
		if ((errnum = aps_write(port,print_buf,n))<0) {
			return errnum;
		}

#ifdef DEBUG_DUMP
		if (dump>0) {
			write(dump,print_buf,n);
		}
#endif /*DEBUG_DUMP*/
	}

	return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  write_block
Purpose   :  Write data block to printer
Inputs    :  fd   : input file descriptor
size : data block size in bytes
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int write_block(int fd,int size)
{
	aps_error_t errnum;

	while (size) {
		int max;
		int n;

		/*read data from input pipe*/
		if (size>(int)sizeof(print_buf)) {
			max = sizeof(print_buf);
		}
		else {
			max = size;
		}

		if ((n = read(fd,print_buf,max))<=0) {
			return APS_IO_ERROR;
		}

		/*write data to printer*/
		if ((errnum = aps_write(port,print_buf,n))<0) {
			return errnum;
		}

#ifdef DEBUG_DUMP
		if (dump>0) {
			write(dump,print_buf,n);
		}
#endif /*DEBUG_DUMP*/

		/*update block size counter*/
		size -= n;
	}

	return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  state_print
Purpose   :  Print data
Inputs    :  fd : input file descriptor
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int state_print(int fd)
{
	unsigned char * xbuf, * p;
	aps_error_t errnum;
	int size, total, cur;

	debug("Entering print state",port);

	/*setup printing timeout*/
	if ((errnum = aps_set_write_timeout(port,prtimeout))<0) {
		return errnum;
	}

	/*read first block size*/
	if (read(fd,&size,sizeof(int))<(int)sizeof(int)) {
		return APS_IO_ERROR;
	}

	if (size==-1) {
		/*write raw data*/
		if ((errnum = write_raw(fd))<0) {
			return errnum;
		}
	}
	else {
		total = 0;
		p = xbuf = malloc(4096 * 1024);
		if (!p)
			return APS_IO_ERROR;
		while (!cancel_flag) {
			if (read(fd,p,size) < size) {
				return APS_IO_ERROR;
			}
			total += size;
			p += size;
			/*read next block size, exit if end of file*/
			if (read(fd,&size,sizeof(int))<(int)sizeof(int)) {
				break;
			}
		}
		/* flush data to printer */
		if ((errnum = aps_write(port,xbuf,total))<0) {
			return errnum;
		}
		free(xbuf);
	}

	/*wait until all data has actually been sent*/
	if ((errnum = aps_sync(port))<0) {
		return errnum;
	}

	return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  state_finish
Purpose   :  Finish printing, revert printer state to default
Inputs    :  <>
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
static int state_finish(void)
{
	aps_error_t errnum;

	debug("Entering finish state",port);

	/*update status*/
	if ((errnum = poll_status())<0) {
		return errnum;
	}

	/*revert port settings to defaults*/
	if ((errnum = setup_defaults())<0) {
		return errnum;
	}

	return APS_OK;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/
#ifdef DEBUG_DUMP
#	warning "DEBUG_DUMP enabled !!!"
#endif
#ifdef DEBUG_DUMP_FILE
#	warning "DEBUG_DUMP_FILE enabled !!!"
#endif

/*-----------------------------------------------------------------------------
Name      :  main
Purpose   :  Program main function
Inputs    :  argc : number of command-line arguments (including program name)
argv : array of command-line arguments
Outputs   :  <>
Return    :  0 if successful, 1 if program failed
-----------------------------------------------------------------------------*/
int main(int argc,char** argv)
{
    struct sigaction sa;
    aps_error_t errnum;
    int fd;

#ifdef DEBUG_DUMP
    dump = open(DEBUG_DUMP_FILE,O_CREAT|O_WRONLY);

    if (dump>0) {
        fchmod(dump,0777);
    }
#endif /*DEBUG_DUMP*/

    setbuf(stderr,NULL);

    debug("aps backend started",NULL);

    /*record version information*/
    fprintf(stderr,"DEBUG: aps backend compiled with APS library version %d.%d.%d\n",
            APS_MAJOR,
            APS_MINOR,
            APS_BUGFIX);

    /*check arguments*/
    if (argc==1) {
	    fprintf(stderr,"DEBUG: aps listing devices\n");
        list_devices();
        return 0;
    }
    else if (argc<6 || argc>7) {
        fputs("ERROR: apsd job-id user title copies options [file]\n",stderr);
        return 1;
    }

    /*retrieve options*/
    get_options(argv[5]);

    /*print real and effective user ID*/
    fprintf(stderr, "DEBUG: Real uid = %d\n", getuid());
    fprintf(stderr, "DEBUG: Effective uid = %d\n", geteuid());

    /*print real and effective group ID*/
    fprintf(stderr, "DEBUG: Real gid = %d\n", getgid());
    fprintf(stderr, "DEBUG: Effective gid = %d\n", getegid());

    /*dump options to error log file*/
    dump_options();

    /*open input file*/
    if (argc==7) {
        if ((fd = open(argv[6],O_RDONLY))==-1) {
            error("Unable to open input file");
        }
    }
    else
    {
        fd = 0; /*stdin*/
    }

    /*ignore SIGPIPE signals*/
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE,&sa,NULL);

    /*install SIGTERM handler*/
    cancel_flag = 0;

    memset(&sa,0,sizeof(sa));
    sa.sa_handler = cancel_handler;
    sigaction(SIGTERM,&sa,NULL);

    /*create printer port*/
    fprintf(stderr, "DEBUG: Create port :%s\n", getenv("DEVICE_URI"));
    port = aps_create_port(getenv("DEVICE_URI"));

    if (port==NULL) {
        error("error creating port");
    }

    debug("get error...",port);
    if ((errnum = aps_get_error(port))<0) {
        error(aps_get_strerror_full(errnum,port));
    }

    if (errnum == APS_OK) {
        /*open port*/
        debug("Open port...",port);
        if ((errnum = aps_open(port))<0) {
            error(aps_get_strerror_full(errnum,port));
        }
    }

    if (errnum==APS_OK) {
        debug("Setup Printer...",port);
        errnum = state_setup();
    }

    if (errnum==APS_OK) {
        debug("Wait Printer...",port);
        errnum = state_wait();
    }

    if (errnum==APS_OK) {
        debug("Print in Printer...",port);
        errnum = state_print(fd);
    }

    if (errnum==APS_OK) {
        debug("Wait Printer...",port);
        errnum = state_wait();
    }

    if (errnum==APS_OK) {
        debug("Wait usb ...",port);
        errnum = state_wait_usb();
    }

    if (errnum==APS_OK) {
        debug("finish ...",port);
        errnum = state_finish();
    }

    /*reset printer in case of timeout*/
    if (errnum==APS_WRITE_TIMEOUT || errnum==APS_READ_TIMEOUT) {
        debug("aps backend failed, resetting printer",port);

        /*try to update printer status message*/
        if (poll_status()<0) {
            fprintf(stderr,"INFO: %s.\n",aps_get_strerror_full(errnum,port));
        }

        if ((errnum = reset_printer())<0) {
            error(aps_get_strerror_full(errnum,port));
        }

        /*printer port is closed*/
    }
    else if (errnum<0) {
        debug("aps backend failed",port);

        fprintf(stderr,"INFO: %s.\n",aps_get_strerror_full(errnum,port));
    }

    debug("Close Port ...",port);
    /*close port*/
    /*ignore errors (in case of port already closed, for example)*/
    aps_close(port);

    debug("Destroy Port ...",port);
    /*destroy printer port*/
    if ((errnum = aps_destroy_port(port))<0) {
        error(aps_strerror(errnum));
    }

    port = NULL;

    /*restore default SIGPIPE handler*/
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGPIPE,&sa,NULL);

    /*restore default SIGTERM handler*/
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGTERM,&sa,NULL);

    /*close input file*/
    if (fd!=0) {
        close(fd);
    }

    debug("aps backend finished",NULL);

#ifdef DEBUG_DUMP
    if (dump>0) {
        close(dump);
    }
#endif /*DEBUG_DUMP*/

    return 0;
}

