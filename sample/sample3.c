/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : sample3.c
* DESCRIPTION   : APS Linux Driver sample using CUPS API
* CVS           : $Id: sample3.c,v 1.3 2006/03/09 14:53:31 nicolas Exp $
*******************************************************************************
* HISTORY       :
*   08mar2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cups/cups.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

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
int main(int argc,char **argv)
{
        const char *dest;
        int jobid;
        int timeout;
        int complete;

        /*select destination printer*/
        if (argc<2) {
                dest = cupsGetDefault();
        }
        else {
                dest = argv[1];
        }

        printf("Printing to %s.\n",dest);

        /*print this source file*/
        jobid = cupsPrintFile(dest,__FILE__,"title",0,NULL);

        if (jobid==0) {
                error(ippErrorString(cupsLastError()));
        }

        /*wait until print job is complete*/
        complete = 0;

        for (timeout=30; timeout>0 && !complete; timeout--) {
                cups_job_t *jobs;
                int i,n;

                /*check job state every second*/
                sleep(1);

                /*retrieve completed jobs on selected printer*/
                n = cupsGetJobs(&jobs,dest,1,1);

                for (i=0; i<n; i++) {
                        if (jobs[i].id==jobid) {
                                complete = 1;
                        }
                }
                
                cupsFreeJobs(n,jobs);
        }

        /*cancel print job on timeout*/
        if (timeout==0) {
                printf("Cancelling job %d...\n",jobid);
                cupsCancelJob(dest,jobid);
        }

        return 0;
}

