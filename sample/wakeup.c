/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : sample1.c
* DESCRIPTION   : Basic sample of libaps use
* CVS           : $Id: sample1.c,v 1.5 2007/12/10 15:14:36 nicolas Exp $
*******************************************************************************
* HISTORY       :
*   08mar2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <aps/aps.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static const char ticket[] = "\x16hello, world!\n";

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

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
                printf("Usage: sample1 URI\n");
                exit(0);
        }
        
        /* create communication port from URI */
        port = aps_create_port(argv[1]);

        /* open communication port */
        errnum = aps_open(port);

        /* write to communication port */
        errnum = aps_write(port, ticket, strlen(ticket));

        /* close communication port */
        errnum = aps_close(port);

        /* destroy communication port */
        errnum = aps_destroy_port(port);

        return 0;
}

