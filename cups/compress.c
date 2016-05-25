/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : compress.c
* DESCRIPTION   : Bitmap compression routines
* CVS           : $Id: compress.c,v 1.1 2006/06/13 13:55:10 nicolas Exp $
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
*   19may2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/*this is the worst compression factor we can get*/
/*this is used to compute size of the compression buffer*/

#define WORST_COMPRESSION_FACTOR        2.0

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  emit_raw
Purpose   :  Emit raw bytes to compression buffer
Inputs    :  buf  : compression buffer
             max  : compression buffer size in bytes
             raw  : raw bytes buffer
             size : raw bytes buffer size in bytes
Outputs   :  Compression buffer is modified
Return    :  Number of bytes written or -1 if error
-----------------------------------------------------------------------------*/
static int emit_raw(unsigned char *buf,int max,unsigned char *raw,int size)
{
        if (max<2+size) {
                return -1;
        }
        else {
                int i;

                buf[0] = 0;
                buf[1] = size;

                for (i=0; i<size; i++) {
                        buf[2+i] = raw[i];
                }

                return 2+size;
        }
}

/*-----------------------------------------------------------------------------
Name      :  emit_compressed
Purpose   :  Emit compressed bytes to compression buffer
Inputs    :  buf   : compression buffer
             max   : compression buffer size in bytes
             byte  : byte to repeat
             count : number of times to repeat
Outputs   :  Compression buffer is modified
Return    :  Number of bytes written or -1 if error
-----------------------------------------------------------------------------*/
static int emit_compressed(unsigned char *buf,int max,int byte,int count)
{
        if (max<2) {
                return -1;
        }
        else {
                buf[0] = count;
                buf[1] = byte;
                return 2;
        }
}

/*-----------------------------------------------------------------------------
Name      :  compress_dotline
Purpose   :  Compress dotline data using HRS/KCP compression algorithm
Inputs    :  dotline   : dotline buffer
             num_bytes : width of dotline in bytes
             buf       : compression buffer
             max       : compression buffer size in bytes
Outputs   :  Fills buf with compressed dotline
Return    :  Number of compressed bytes
-----------------------------------------------------------------------------*/
static int compress_dotline(unsigned char *dotline,int num_bytes,unsigned char *buf,int max)
{
        int i;
        int size;
        int rawsize;
        unsigned char rawbuf[256];

        size = 0;
        rawsize = 0;

        for (i=0; i<num_bytes; ) {
                int repbyte;
                int repcount;

                /*get number of repetitions of current byte*/
                repbyte = dotline[i];
                repcount = 1;

                while (repcount<256 && i+repcount<num_bytes) {
                        if (repbyte==dotline[i+repcount]) {
                                repcount++;
                        }
                        else {
                                break;
                        }
                }

                /*decide whether to emit compressed or raw bytes*/
                if (repcount<3) {
                        if (rawsize==255) {
                                int n;
                               
                                if ((n=emit_raw(buf,max,rawbuf,rawsize))<0) {
                                        return -1;
                                }

                                size += n;
                                buf += n;
                                max -= n;
                                
                                rawsize = 0;
                        }
                        rawbuf[rawsize] = dotline[i];
                        rawsize++;
                        i++;
                }
                else {
                        int n;

                        if ((n=emit_compressed(buf,max,repbyte,repcount))<0) {
                                return -1;
                        }

                        size += n;
                        buf += n;
                        max -= n;

                        i += repcount;
                }
        }

        /*emit remaining raw bytes*/
        if (rawsize!=0) {
                int n;
                
                if ((n=emit_raw(buf,max,rawbuf,rawsize))<0) {
                        return -1;
                }

                size += n;
        }

        return size;
}

