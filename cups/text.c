/******************************************************************************
 * COMPANY       : APS ENGINEERING
 * PROJECT       : LINUX DRIVER
 *******************************************************************************
 * NAME          : text.c
 *
 * DESCRIPTION   : this module is convert all character to a bipmap
 *                 and send it to the printer when the line is finished
 *
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
 *   09mar2009   pierre  Initial revision
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include "aps_fnt.h"

#include "text.h"

/* PRIVATE DEFINITIONS ------------------------------------------------------*/
void*   fnt = NULL;
uint8_t *graphic_buf = NULL;
int     graphic_high;
int     pix_pos;
int     raw;

#define BLANK_BUFSIZE   256             /*bytes*/

static unsigned char    blank_buf[BLANK_BUFSIZE];

/*
 * -----------------------------------------------------------------------------
 * Name      :  print_blank_normal
 * Purpose   :  Print blank dotlines using normal print command
 * Inputs    :  nbytes : width of dotline in bytes
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void print_blank_normal(int nbytes)
{
        aps_error_t errnum;
        command_t cmd;

        errnum = cmd_print_dotline(printer_type,&cmd,nbytes);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(raw,&cmd,blank_buf,nbytes);
        }

        fflush(stdout);
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  print_blank_opt
 * Purpose   :  Print blank dotlines using optimized technique
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void print_blank_opt(void)
{
        aps_error_t errnum;
        command_t cmd;

        errnum = cmd_print_dotline(printer_type,&cmd,1);

        if (errnum<0) {
                error(aps_strerror(errnum));
        }
        else {
                write_command(raw,&cmd,blank_buf,1);
        }

        fflush(stdout);
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  print_blank
 * Purpose   :  Write APS commands to print blank dotlines
 * Inputs    :  n      : number of dotlines to print
 * nbytes : width of dotline in bytes
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void print_blank(int n,int nbytes)
{
    while (n--) {

        if (optprint) {
            print_blank_opt();
        }
        else {
            print_blank_normal(nbytes);
        }
    }
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  print_dotline
 * Purpose   :  Write APS command to print dotline
 * Inputs    :  buf    : dotline buffer
 * nbytes    :  width of dotline in bytes
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void print_dotline(const unsigned char *buf,int nbytes)
{
    aps_error_t errnum;
    command_t cmd;

    errnum = cmd_print_dotline(printer_type,&cmd,nbytes);

    if (errnum<0) {
        error(aps_strerror(errnum));
    }
    else {
        write_command(raw,&cmd,buf,nbytes);
    }

    fflush(stdout);
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  print_text_line
 * Purpose   :  send to the printer the current text line
 *
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void print_text_line(void)
{
    int i;
    uint8_t *p;
    
    i = graphic_high;
    p = graphic_buf;

    while (i--)
    {
        print_dotline(p,printer_width);
        p+=printer_width;
    }
    if (linespacing < 0)
        print_blank(3,printer_width);
    else
        print_blank(linespacing,printer_width);
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  clear_text_line
 * Purpose   :  prepare the new text line
 *
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void clear_text_line(void)
{
    pix_pos = 0;
    memset(graphic_buf,0,printer_width*graphic_high);
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  do_new_line
 * Purpose   :  send to the printer the current line
 *              end prepare to build an new one
 *
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
static void do_new_line(void)
{
    print_text_line();
    clear_text_line();
}


/* PUBLIC DEFINITIONS ------------------------------------------------------*/

/*
 * -----------------------------------------------------------------------------
 * Name      :  text_init
 * Purpose   :  initialise the text module
 *
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
void text_create(int _raw,char *_font_path)
{
    if (_font_path == NULL)
        return ;

    raw = _raw;

    fnt = aps_fnt_create(_font_path);

#if 1
    if (fnt != NULL)
    {
        aps_fnt_details_t details;
        aps_fnt_get_details(fnt,&details);

        fprintf(stderr,"DEBUG: Font Name        : %s\n",details.name); 
        fprintf(stderr,"DEBUG: Font Version     : %d\n",details.version); 
        fprintf(stderr,"DEBUG: Font width       : %d\n",details.width); 
        fprintf(stderr,"DEBUG: Font height      : %d\n",details.height); 
        fprintf(stderr,"DEBUG: Font downstroke  : %d\n",details.downstroke); 
        fprintf(stderr,"DEBUG: Font compression : %d\n",details.compression); 
        fprintf(stderr,"DEBUG: Font Bank size   : %d\n",details.char_list_size); 
    }
#endif

    if (aps_fnt_error(fnt) < 0)
    {
        fprintf(stderr,"ERROR: TextToAPS.font_path  : %s \n",_font_path);
        fprintf(stderr,"ERROR: TextToAPS.font_error : (%d) str: %s \n",aps_fnt_error(fnt),aps_fnt_error_str(fnt));
        error("Impossible to load aps font.");
        return;
    }

    graphic_high = aps_fnt_get_high(fnt);
    graphic_buf = malloc(printer_width * graphic_high);

    clear_text_line();

    /*do a blank buffer*/
    memset(blank_buf,0,sizeof(blank_buf));

}

/*
 * -----------------------------------------------------------------------------
 * Name      :  text_putc
 * Purpose   :  put a character in graphic buffer
 *
 * Inputs    :  c code of character to put in text line
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
void text_putc(int c)
{
    if (fnt == NULL)
        return;

    if ((c == LF) || (c == CR))
    {
        do_new_line();
    }
    else
    {
        int res;
        res = aps_fnt_draw_char(fnt, graphic_buf, &pix_pos,printer_width,c);
        
        if (res != fntERR_OK){
	        fprintf(stderr,"DEBUG: TextToAPS.font_error : %d \n",aps_fnt_error(fnt));
        }

        if (res == fntERR_CHAR_BIPMAP_PTR_NULL)
        {
            do_new_line();

            /* try again */
            res = aps_fnt_draw_char(fnt,graphic_buf,&pix_pos,printer_width,c);
            if (res != fntERR_OK){
                fprintf(stderr,"DEBUG: TextToAPS.font_error : %d \n",aps_fnt_error(fnt));
            }
            if (res == fntERR_LINE_FULL)
                return;
        }
        if (charspacing < 0)
            pix_pos += 1; /* spacing character */
        else
            pix_pos += charspacing; /* spacing character */
    }
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  text_finish
 * Purpose   :  valid and send last line and unload the module
 *
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
void text_free(void)
{
    aps_fnt_free(fnt);
    if (graphic_buf != NULL)
    {
        free(graphic_buf);
    }
}

