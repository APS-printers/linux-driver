/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : serial.c
 * DESCRIPTION   : APS library - serial communication routines
 * CVS           : $Id: serial.c,v 1.19 2008/07/09 14:22:29 pierre Exp $
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
 *   05dec2008   pnb     Initial revision
 ******************************************************************************
 */

#if defined(APS_DATA_LOG)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <linux/version.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*
 * -----------------------------------------------------------------------------
 * Name      :  log_open
 * Purpose   :  Open data log file
 * Inputs    :  p : port structure
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------
 */
int log_open(aps_port_t *p)
{
    aps_error_t errnum;

    if ((p->data_log_fd = fopen(APS_DATA_LOG,"a")) == NULL) {
        return APS_OPEN_FAILED;
    }

    return APS_OK;
}

/*-----------------------------------------------------------------------------
Name      :  log_close
Purpose   :  Close data log file
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int log_close(aps_port_t *p)
{
    aps_error_t errnum;

    if (p->data_log_fd == NULL) return APS_CLOSE_FAILED;

    /*close device*/
    if (fclose(p->data_log_fd) != 0) {
        errnum = APS_CLOSE_FAILED;
    }
    else {
        errnum = APS_OK;
    }

    return errnum;
}



/*
 * -----------------------------------------------------------------------------
 * Name      :  log_write
 * Purpose   :  Write data buffer to log file
 * Inputs    :  p    : port structure
 *              buf  : data buffer
 *              size : data buffer size in bytes
 * Outputs   :  <>
 * Return    :  APS_OK or error code
 * -----------------------------------------------------------------------------
 */
int log_write(aps_port_t *p,const void *buf,int size)
{
    aps_error_t errnum = APS_OK;

    if (p->data_log_fd == NULL) return APS_WRITE_FAILED;

    n = fwrite(buf,1,size,p->data_log_fd);

    if (n != size) {
        errnum = APS_WRITE_FAILED;
        break;
    }

    return errnum;
}


/*-----------------------------------------------------------------------------
Name      :  log_flush
Purpose   :  Clear input and output buffers
Inputs    :  p : port structure
Outputs   :  <>
Return    :  APS_OK or error code
-----------------------------------------------------------------------------*/
int log_flush(aps_port_t *p)
{
    aps_error_t errnum;

    if (p->data_log_fd == NULL) return APS_FLUSH_FAILED;

    if (fflush(p->data_log_fd) != 0) {
        errnum = APS_FLUSH_FAILED;
    }
    else {
        errnum = APS_OK;
    }

    return errnum;
}

#endif
