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

static const char ticket[] = "\x1d\x54";

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
	aps_usb_ctrltransfer_t ctrl = { 2 << 5, 2, 0, 0, 0, 0, };
        int errnum;
	unsigned char buf[3] = { 0, 0, 0, };
	int i;

        if (argc<2) {
                printf("Usage: sample1 URI\n");
                exit(0);
        }
        
        /* create communication port from URI */
        port = aps_create_port(argv[1]);

        /* open communication port */
        errnum = aps_open(port);
	if (errnum != APS_OK)
	{
		printf("cannot open printer\n");
		exit(1);
	}

        /* write to communication port */
	/* try to reset the printer */
	printf("resetting printer...\n");
        if (1) errnum = aps_write(port, "\x1b\x40", 2);
#if 0
	errnum = usb_control(port, &ctrl);
	if (0 && errnum != APS_OK)
	{
		printf("cannot reset printer\n");
		exit(1);
	}
#endif
	sleep(3);

	aps_close(port);
        aps_destroy_port(port);

	/* try reopening the port after resetting the printer */
	printf("reopening printer...\n");
	for (i = 0; i < 100; i ++)
	{
		port = aps_create_port(argv[1]);
		if (errnum != APS_OK)
		{
			printf("aps_open() failed\n");
			sleep(1);
		}
		else
		{
			printf("aps_open() succeeded\n");
			break;
		}
	}
	if (i == 100)
	{
		printf("cannot open printer\n");
		exit(1);
	}

        /* open communication port */
        errnum = aps_open(port);
	if (errnum != APS_OK)
	{
		printf("cannot open printer\n");
		exit(1);
	}


	aps_set_read_timeout(port, 10000);
	aps_set_write_timeout(port, 10000);
	/* calibrate the printer */
	aps_write(port, "\x1d\x45", 2);
	aps_sync(port);

	errnum = aps_read(port, buf, 3);
	printf("aps_read(): errnum is %i\n", errnum);
	printf("0x%02x, 0x%02x, 0x%02x\n", buf[0], buf[1], buf[2]);
        
        /* close communication port */
        aps_close(port);

        /* destroy communication port */
        aps_destroy_port(port);

        return 0;
}

