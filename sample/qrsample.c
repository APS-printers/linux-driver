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

#include <fcntl.h>

#include <aps/aps.h>
#include <cups/command.h>
#include <cups/command.c>


/* PRIVATE DEFINITIONS ------------------------------------------------------*/

static const char ticket[] = "Hello world!\n";

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
FILE * logfd;
int main(int argc,char** argv)
{
        void *port;
        int errnum;
	int ver, level, mode, casesensitivity;
	char * text;
	char * uri;
	int i;
	int qr_width;
	char * fname;

	logfd = stderr;

        if (argc < 4) {
                printf("Usage: %s [-v version] [-l level] [-m mode] [-c casesensitive] -u URI -f file text\n"
				"-v version - the version of the symbol; if 0 or omitted, the "
				"minimum version sufficient for the given input data will "
				"automatically be picked up\n"
				"-l level - the error correction level to use; valid values are "
				"L, M, Q, H; if omitted, level L will be used\n"
				"-m mode - encoding mode to use; accepted values: 0 - numeric;"
				"1 - alphanumeric; 2 - 8 bit; 3 - kanji; if omitted, 8 bit encoding "
				"will be used\n"
				"-c casesensitive - controls the significance of case; accepted "
				"values: 0 - case insensitive; 1 - case sensitive; if omitted, "
				"case insensitive encoding will be assumed\n"
				"-u URI - the printer URI to use; this is the same that is "
				"specified when installing the driver\n"
				"text - the text to print\n"
				"-f file	file to use as input\n",
				* argv);
                exit(0);
        }
	ver = 0;
	level = 0;
	mode = 2;
	casesensitivity = 0;
	text = 0;
	uri = 0;
	fname = 0;

	for (i = 1; i < argc; i ++)
	{
		if (* argv[i] == '-')
		{
			if (i == argc - 1)
			{
				printf("missing argument\n");
				exit(-1);
			}
			switch(argv[i][1])
			{
				case 'v':
					if (sscanf(argv[i + 1], "%i", & ver) != 1)
					{
						printf("bad version\n");
						exit(-1);
					}
					break;
				case 'l':
					switch (argv[i + 1][0])
					{
						case 'L': case 'l':
							level = 0;
							break;
						case 'M': case 'm':
							level = 1;
							break;
						case 'Q': case 'q':
							level = 2;
							break;
						case 'H': case 'h':
							level = 3;
							break;
						default:
							printf("bad level\n");
							exit(-1);
					}
					break;
				case 'm':
					switch (argv[i + 1][0])
					{
						case '0':
							mode = 0;
							break;
						case '1':
							mode = 1;
							break;
						case '2':
							mode = 2;
							break;
						case '3':
							mode = 3;
							break;
						default:
							printf("bad mode\n");
							exit(-1);
					}
					break;
				case 'c':
					switch (argv[i + 1][0])
					{
						case '0':
							casesensitivity = 0;
							break;
						case '1':
							casesensitivity = 1;
							break;
						default:
							printf("bad case sensitivity value\n");
							exit(-1);
					}
					break;
				case 'u':
					if (uri)
					{
						printf("error: multiple URI requested\n");
						exit(-1);
					}
					uri = argv[i + 1];
					break;
				case 'f':
					if (fname)
					{
						printf("error: multiple files requested\n");
						exit(-1);
					}
					fname = argv[i + 1];
					break;
				default:
					printf("bad request\n");
					exit(-1);
			}
			i ++;
		}
		else
		{
			if (text)
			{
				printf("error: multiple text strings requested\n");
				exit(-1);
			}
			text = argv[i];
		}
	}

	if (!uri)
	{
		printf("error: uri not specified\n");
		exit(-1);
	}
	if (!text && !fname)
	{
		printf("error: text not specified\n");
		exit(-1);
	}
	if (text && fname)
	{
		printf("error: multiple symbols specified\n");
		exit(-1);
	}
        
        /* create communication port from URI */
        port = aps_create_port(uri);

        /* open communication port */
        errnum = aps_open(port);

	if (errnum != APS_OK)
	{
		printf("error opening printer\n");
		exit(-1);
	}

	{
		char * qr_data;
		int qrlen;
		command_t cmd;
		int err;

		if (text)
		{
			err = cmd_mrs_qrcode(ver, level, mode, casesensitivity, text, &qr_data, &qrlen, &cmd);
		}
		else
		{
			int fd;
			char buf[10 * 1024];
			int blen;
			if ((fd = open(fname, O_RDONLY)) < 0)
			{
				printf("error opening file\n");
				exit(-1);
			}
			if ((blen = read(fd, buf, sizeof buf)) < 0)
			{
				printf("error reading file\n");
				exit(-1);
			}

			close(fd);
			err = cmd_mrs_qrcode_bindata(ver, level, 2, buf, blen, &qr_data, &qrlen, &cmd, &qr_width);
		}
		if (err == APS_OK)
		{
#if 0
			aps_class_t *p = port;
			int fd = p->port.set.serial.fd;

			write(fd, cmd.buf, cmd.size);
			write(fd, qr_data, qrlen);
#endif
			aps_write(port, cmd.buf, cmd.size);
			aps_write(port, qr_data, qrlen);
			free(qr_data);
		}
		else
		{
			printf("error encoding data\n");
		}
	}

        /* write to communication port */
        if (1) errnum = aps_write(port,ticket,sizeof(ticket)-1);
        
        /* close communication port */
        errnum = aps_close(port);

        /* destroy communication port */
        errnum = aps_destroy_port(port);

        return 0;
}

