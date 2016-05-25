/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : sample2.c
* DESCRIPTION   : Basic sample of libaps use (includes error checking)
* CVS           : $Id: sample2.c,v 1.4 2006/03/08 18:13:06 nicolas Exp $
*******************************************************************************
* HISTORY       :
*   08mar2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <aps/aps.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static const char ticket[] = "Hello world!\n";

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  error
Purpose   :  Print error string to stderr and exit program with error
Inputs    :  s : error string
Outputs   :  <>
Return    :  <>
-----------------------------------------------------------------------------*/
static void error(const char *s)
{
        fprintf(stderr,"error: %s\n",s);
        exit(1);
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
        void *port;
        int errnum;
        
        if (argc<2) {
                printf("usage: sample2 uri\n");
                return 0;
        }

        /* create communication port from URI */
        port = aps_create_port(argv[1]);

        if (port==NULL) {
                error("cannot create port");
        }
        else if ((errnum = aps_get_error(port))<0) {
                error(aps_strerror(errnum));
        }

        /* open communication port */
        if ((errnum = aps_open(port))<0) {
                error(aps_strerror(errnum));
        }

        /* write to communication port */
        if ((errnum = aps_write(port,ticket,sizeof(ticket)-1))<0) {
                error(aps_strerror(errnum));
        }
        
        /* close communication port */
        if ((errnum = aps_close(port))<0) {
                error(aps_strerror(errnum));
        }

        /* destroy communication port */
        if ((errnum = aps_destroy_port(port))<0) {
                error(aps_strerror(errnum));
        }

        return 0;
}

