/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : sample5.c
* DESCRIPTION   : Demonstrates custom high-speed baudrates for HSP printer
* CVS           : $Id: sample5.c,v 1.1 2007/12/10 15:14:36 nicolas Exp $
*******************************************************************************
* HISTORY       :
*   06apr2007   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aps/aps.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

#define ESC     27
#define GS      29

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  print_ticket
Purpose   :  Print test ticket (100 lines)
Inputs    :  port : port structure
Outputs   :  <>
Return    :  APS_OK or negative error code
-----------------------------------------------------------------------------*/
static int print_ticket(void *port)
{
        int errnum;
        int i;
        char line[100];

        if ((errnum = aps_set_write_timeout(port,5000))<0) {
                return errnum;
        }

        for (i=0; i<100; i++) {
                snprintf(line,sizeof(line),"%d: Hello world at high speed!\n",i);

                if ((errnum = aps_write(port,line,strlen(line)))<0) {
                        return errnum;
                }
        }

        return APS_OK;
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
        unsigned char buf[3];

        if (argc<2) {
                printf("Usage: sample5 URI\n");
                exit(0);
        }
        
        /* create communication port from URI */
        /* this must point to a HSP printer on a RS232 port */
        port = aps_create_port(argv[1]);

        /* open communication port */
        errnum = aps_open(port);

        /* set baudrate to 250000 bauds (hardware handshake mode) */
        buf[0] = GS;
        buf[1] = 'S';
        buf[2] = 0x89;

        errnum = aps_write(port,buf,sizeof(buf));
        errnum = aps_flush(port);

        errnum = aps_serial_set_baudrate(port,APS_B250000);
        errnum = aps_serial_set_handshake(port,APS_RTSCTS);

        errnum = print_ticket(port);

        /* revert to default baudrate (9600 bauds) */
        buf[0] = GS;
        buf[1] = 'S';
        buf[2] = 0x83;

        errnum = aps_write(port,buf,sizeof(buf));
        errnum = aps_flush(port);
        
        /* close communication port */
        errnum = aps_close(port);

        /* destroy communication port */
        errnum = aps_destroy_port(port);

        return 0;
}

