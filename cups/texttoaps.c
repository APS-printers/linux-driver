/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : texttoaps.c
 *
 * DESCRIPTION   : CUPS filter for APS printers
 *                 Converts plain text into APS commands
 *                 APS command set is selected depending on cupsModelNumber
 *                 attribute of the PPD file
 *
 * CVS           : $Id: texttoaps.c,v 1.12 2008/07/09 14:22:29 pierre Exp $
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
 *   12jun2006   nico    Added cancel support
 *   03mar2008   nico    Modified text processing state machine ('<<LF>' bug)
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cups/cups.h>
#include <cups/raster.h>

#include <aps/aps.h>

#include "command.h"
#include "options.h"
#include "ticket.h"
#include "utf8.h"
#include "text.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static  sig_atomic_t    cancel_flag;

#define BUFSIZE 4096    /*bytes*/

static enum {
    PROCESSING_IDLE = 0,
	PROCESSING_TAG,
	QRCODE_READING_VER0,
	QRCODE_READING_VER1,
	QRCODE_READING_LEVEL,
	QRCODE_READING_MODE,
	QRCODE_READING_CASE_SENSITIVITY,
	QRCODE_READING_DATA,
} state;

#define TAG_BUFSIZE 256         /*bytes*/

static int      tag_index;
static char     tag_buf[TAG_BUFSIZE+1];

typedef struct {
    char *  text;
    int     value;
} alias_t;

static const alias_t alias_table[] = {
    {"NUL", NUL},
    {"SOH", SOH},
    {"STX", STX},
    {"ETX", ETX},
    {"EOT", EOT},
    {"ENQ", ENQ},
    {"ACK", ACK},
    {"BEL", BEL},
    {"BS",  BS},
    {"TAB", TAB},
    {"LF",  LF},
    {"VT",  VT},
    {"FF",  FF},
    {"CR",  CR},
    {"SO",  SO},
    {"SI",  SI},
    {"DLE", DLE},
    {"DC1", DC1},
    {"DC2", DC2},
    {"DC3", DC3},
    {"DC4", DC4},
    {"NAK", NAK},
    {"SYN", SYN},
    {"ETB", ETB},
    {"CAN", CAN},
    {"EM",  EM},
    {"SUB", SUB},
    {"ESC", ESC},
    {"FS",  FS},
    {"GS",  GS},
    {"RS",  RS},
    {"US",  US}
};

#define ALIAS_TABLE_SIZE    (int)(sizeof(alias_table)/sizeof(alias_table[0]))

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
Name      :  tag_to_char
Purpose   :  Convert current tag to character value
Inputs    :  <>
Outputs   :  <>
Return    :  character value or -1 if conversion is impossible
-----------------------------------------------------------------------------*/
static int tag_to_char(void)
{
    int i;
    int n;

    /*lookup tag in alias table*/
    for (i=0; i<ALIAS_TABLE_SIZE; i++)
        if (strcmp(alias_table[i].text,tag_buf)==0)
            return alias_table[i].value;

    /*try converting numerical value*/
    if (sscanf(tag_buf,"%i",&n)==1) {
        if (n<0 || n>255)
            return -1;
        else
            return n;
    }

    /*we tried everything we could*/
    return -1;
}

/*-----------------------------------------------------------------------------
Name      :  process_and_write
Purpose   :  read data from "fd" and WITH Esc sentence interpretation 
             and convert it (if font_path!=NULL) and put it on stdout
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void process_and_write(void)
{
    int c;

    debug("Processing esc sentence.",NULL);
    
    while ((c = utf8_get_code()) >= 0)
    {
        if (cancel_flag)
            break;

        switch (state) {
            case PROCESSING_IDLE:
                if (c == '<') 
                {
                    tag_index = 0;
                    state = PROCESSING_TAG;
                }
                else
                    if (font_path != NULL) 
                        text_putc(c);
                    else
                        fputc(c,stdout);
                break;

			default:
            case PROCESSING_TAG:
                if (c=='>') {
                    int n;

                    tag_buf[tag_index] = 0;
                    n = tag_to_char();

                    if (n==-1)
                        printf("<%s>",tag_buf);
                    else
                        fputc(n,stdout);

                    state = PROCESSING_IDLE;
                }
                else if (c=='<') {
                    /*reset tag index in case of '<<LF>'*/
                    fputc('<',stdout);
                    tag_index = 0;
                }
                else {
                    if (tag_index==TAG_BUFSIZE) {
                        tag_buf[tag_index] = 0;
                        printf("<%s",tag_buf);

                        state = PROCESSING_IDLE;
                    }
                    else {
                        tag_buf[tag_index] = c;
                        tag_index++;
                    }
                }
                break;
        }
    }
}


static int fromascii(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else
		return -1;
}


/*-----------------------------------------------------------------------------
Name      :  noprocess_and_write
Purpose   :  read data from "fd" and WITHOUT Esc sentence interpretation 
             and convert it (if font_path!=NULL) and put it on stdout
Inputs    :  <>
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void noprocess_and_write(void)
{
    int c;
	char * qrbuf;
	int size, idx;

	int ver, level, mode, casesensitivity;

    debug("NOT processing esc sentence.",NULL);
    
    while ((c = utf8_get_code()) >= 0)
    {
        if (cancel_flag)
            break;

		switch (state)
		{
			case PROCESSING_TAG:
				if (c == '.')
				{
					state = QRCODE_READING_VER0;
					qrbuf = malloc((size = 1024) + 1);
					idx = 0;
					if (!qrbuf)
						state = PROCESSING_IDLE;
					break;
				}
				/* fall out */
				state = PROCESSING_IDLE;
				break;
			default:
			case PROCESSING_IDLE:
				if (c == ESC) 
				{
					tag_index = 0;
					state = PROCESSING_TAG;
				}
				else
        if (font_path != NULL) 
            text_putc(c);
        else
						fputc(c,stdout);
				break;
			case QRCODE_READING_VER0:
				ver = level = mode = casesensitivity = 0;
				if ((c = fromascii(c)) == -1)
				{
					state = PROCESSING_IDLE;
					break;
				}
				else
				{
					ver = c * 10;
					state = QRCODE_READING_VER1;
				}
				break;
			case QRCODE_READING_VER1:
				if ((c = fromascii(c)) == -1)
				{
					state = PROCESSING_IDLE;
					break;
				}
				else
				{
					ver += c;
					state = QRCODE_READING_LEVEL;
				}
				break;
			case QRCODE_READING_LEVEL:
				if ((c = fromascii(c)) == -1)
				{
					state = PROCESSING_IDLE;
					break;
				}
				else
				{
					level = c;
					state = QRCODE_READING_MODE;
				}
				break;
			case QRCODE_READING_MODE:
				if ((c = fromascii(c)) == -1)
				{
					state = PROCESSING_IDLE;
					break;
				}
				else
				{
					mode = c;
					state = QRCODE_READING_CASE_SENSITIVITY;
				}
				break;
			case QRCODE_READING_CASE_SENSITIVITY:
				if ((c = fromascii(c)) == -1)
				{
					state = PROCESSING_IDLE;
					break;
				}
				else
				{
					casesensitivity = c;
					state = QRCODE_READING_DATA;
				}
				break;
			case QRCODE_READING_DATA:
				if (c == ESC)
				{
					char * qr_data;
					int qrlen;
					command_t cmd;

					qrbuf[idx] = 0;

					fprintf(stderr,"DEBUG: %s() ver = %i, level = %i, mode = %i, case = %i\n", __func__, ver, level, mode, casesensitivity);


					if (cmd_mrs_qrcode(ver, level, mode, casesensitivity, qrbuf, &qr_data, &qrlen, &cmd) == APS_OK)
					{
						write_command(1, &cmd, qr_data, qrlen);

						fflush(stdout);
						free(qr_data);
					}

					free(qrbuf);

					state = PROCESSING_IDLE;

				}
#if 0
				{
					int i, j, k, x, bcnt, len;
					QRcode *qrcode;
					unsigned char * p, cb;
#if 1
					qrbuf[idx] = 0;
					qrcode = QRcode_encodeString(qrbuf, 0, QR_ECLEVEL_L, QR_MODE_8, 0);
					if (!qrcode)
					{
						state = PROCESSING_IDLE;
						break;
					}

					//p = malloc(len = ((qrcode->width + 7) / 8) * 8);
					len = (qrcode->width + 7) / 8;
					p = malloc(len * qrcode->width);
					memset(p, 0x5a, 10 * len * qrcode->width);

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

					command_t cmd;
					cmd.size = 4;
					cmd.buf[0] = GS;
					cmd.buf[1] = 'k';
					cmd.buf[2] = 8;
					cmd.buf[3] = qrcode->width;
#endif
					//write_command(0, &cmd, p, idx);
					write_command(1, &cmd, p, idx);

					fflush(stdout);

					QRcode_free(qrcode);

					free(qrbuf);
					free(p);

					state = PROCESSING_IDLE;

				}
#endif
				else
				{
					if (idx == size)
        {
						qrbuf = realloc(qrbuf, (size *= 2) + 1);
						if (!qrbuf)
						{
							state = PROCESSING_IDLE;
							break;
						}
        }
					qrbuf[idx ++] = c;
    }

				break;

		}
	}

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
int main(int argc,char** argv)
{
    struct sigaction sa;
    int fd;

    setbuf(stderr,NULL);

    debug("texttoaps filter started",NULL);

    /*record version information*/
    fprintf(stderr,"DEBUG: textaps compiled with APS library version %d.%d.%d\n",
            APS_MAJOR,
            APS_MINOR,
            APS_BUGFIX);

    /*check arguments*/
    if (argc<6 || argc>7) {
        fputs("ERROR: texttoaps job-id user title copies options [file]\n",stderr);
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

    /*stat job file if any*/
    if (argc==7) {
        struct stat st;

        if (stat(argv[6], &st)<0) {
            error("Unable to stat text file");
        }
        else {
            fprintf(stderr,"DEBUG: st.st_mode = %d\n", st.st_mode);
            fprintf(stderr,"DEBUG: st.st_uid = %d\n", st.st_uid);
            fprintf(stderr,"DEBUG: st.st_gid = %d\n", st.st_gid);
            //fprintf(stderr,"DEBUG: st.st_size = %d\n", st.st_size);
        }
    }

    /*open page stream*/
    if (argc==7) {
        if ((fd = open(argv[6],O_RDONLY))==-1) {
            error("Unable to open text file");
        }
    }
    else
        fd = 0; /*stdin*/

    /*install cancel handler*/
    cancel_flag = 0;

    memset(&sa,0,sizeof(sa));
    sa.sa_handler = &cancel_handler;
    sigaction(SIGTERM,&sa,NULL);

    /*enter raw ticket mode*/
    enter_raw_mode();

    /*write ticket prolog*/
    write_prolog(1);

    /*perform simple page accounting*/
    fprintf(stderr,"PAGE: 1 1\n");


    /*pipe text file to standard output*/
    if(font_path != NULL)
        utf8_set_file(fd,1);
    else
        utf8_set_file(fd,0);

    /*load external font if needed */
    if (font_path != NULL) {
        debug("Font used is :",NULL);
        debug(font_path,NULL);
        text_create(1,font_path);
    }
    else {
        debug("No font Setted used internal printer font.",NULL);
    }

    if (process)
        process_and_write();
    else
        noprocess_and_write();

    /*Unload external font if needed */
    if (font_path != NULL)
        text_free();

    if (cancel_flag) {
        debug("Print job was cancelled",NULL);
    }
    else {
        /*write ticket epilog*/
        write_epilog(1);
    }

    /*uninstall cancel handler*/
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGTERM,&sa,NULL);

    /*close input file*/
    if (fd!=0) {
        close(fd);
    }

    free_options();

    debug("textttoaps filter finished",NULL);

    return 0;
}


