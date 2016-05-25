/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : sample4.c
* DESCRIPTION   : APS Linux Driver sample using CUPS API
*                 Display printers and state info
*                 Code heavily based on CUPS sources
* CVS           : $Id: sample4.c,v 1.5 2008/03/25 17:37:41 nicolas Exp $
*******************************************************************************
* HISTORY       :
*   08mar2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define _IPP_PRIVATE_STRUCTURES		1

#include <cups/cups.h>
#include <cups/language.h>

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
        cups_lang_t *           lang;
        http_t *                http;
        ipp_t *                 request;
        ipp_t *                 response;
        ipp_attribute_t *       attr;
        int                     port;
        const char *            pattrs[] = {
                "printer-name",
                "printer-state",
                "printer-state-message",
                "printer-state-reasons"
        };

        /*unused parameters*/
        (void)argc;
        (void)argv;

        /*get default language*/
        lang = cupsLangDefault();

        /*connect to HTTP server*/
        port = ippPort();
        
        http = httpConnect("localhost",port);

        /*create new IPP request*/
        request = ippNew();

        request->request.op.operation_id = CUPS_GET_PRINTERS;
        request->request.op.request_id = 1;

        /*add required attributes*/
        ippAddString(request,IPP_TAG_OPERATION,IPP_TAG_CHARSET,
                        "attributes-charset",NULL,cupsLangEncoding(lang));

        ippAddString(request,IPP_TAG_OPERATION,IPP_TAG_LANGUAGE,
                        "attributes-natural-language",NULL,lang->language);

        ippAddStrings(request,IPP_TAG_OPERATION,IPP_TAG_KEYWORD,
                        "requested-attributes",sizeof(pattrs)/sizeof(pattrs[0]),
                        NULL,pattrs);

        response = cupsDoRequest(http,request,"/");

        /*disconnect from server*/
        httpClose(http);

        if (response==NULL) {
                error(ippErrorString(cupsLastError()));
        }
        else {
                for (attr = response->attrs; attr!=NULL; ) {

                        /*search for printer*/
                        while (attr!=NULL && attr->group_tag!=IPP_TAG_PRINTER) {
                                attr = attr->next;
                        }

                        /*show printer info*/
                        if (attr!=NULL) {
                                printf("*printer found*\n");
                        }

                        while (attr!=NULL && attr->group_tag==IPP_TAG_PRINTER) {
                                if (strcmp(attr->name,"printer-name")==0) {
                                        printf("printer-name=%s\n",attr->values[0].string.text);
                                }
                                else if (strcmp(attr->name,"printer-state")==0) {

                                        printf("printer-state=");

                                        switch (attr->values[0].integer) {
                                        case IPP_PRINTER_IDLE:
                                                printf("idle\n");
                                                break;
                                        case IPP_PRINTER_PROCESSING:
                                                printf("processing\n");
                                                break;
                                        case IPP_PRINTER_STOPPED:
                                                printf("stopped\n");
                                                break;
                                        }
                                }
                                else if (strcmp(attr->name,"printer-state-message")==0) {
                                        printf("printer-state-message=%s\n",attr->values[0].string.text);
                                }
                                else if (strcmp(attr->name,"printer-state-reasons")==0) {
                                        int i;
                                        
                                        for (i=0; i<attr->num_values; i++) {
                                                printf("printer-state-reasons[%d]=%s\n",i,attr->values[i].string.text);
                                        }
                                }

                                attr = attr->next;
                        }
                }
        }

        ippDelete(response);

        return 0;
}

