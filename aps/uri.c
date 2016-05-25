/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : uri.c
* DESCRIPTION   : APS library - Uniform Resource Identifier routines
* CVS           : $Id: uri.c,v 1.4 2007/12/10 15:14:36 nicolas Exp $
*******************************************************************************
*   Copyright (C) 2006  APS Engineering
*
*   This file is part of libaps.
*
*   libaps is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   libaps is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with libaps; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*******************************************************************************
* HISTORY       :
*   10feb2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  copy_token
Purpose   :  Copy token into URI structure
Inputs    :  su    : URI structure
             token : token string
Outputs   :  URI structure is modified
Return    :  pointer to token copied in URI structure
-----------------------------------------------------------------------------*/
static char *copy_token(struct aps_uri *su,const char *token)
{
        int len;
        char *dest;

        len = strlen(token);

        if (su->bufpos+len > (int)(sizeof(su->buf)-1)) {
                dest = NULL;
        }
        else {
                dest = &su->buf[su->bufpos];
                strcpy(dest,token);

                su->bufpos += len+1;
        }

        return dest;
}

/*-----------------------------------------------------------------------------
Name      :  get_token
Purpose   :  Get next token from URI buffer. Token ends at delimiter
Inputs    :  su : URI structure
             c  : character delimiter
Outputs   :  URI structure is modified
Return    :  token string or NULL if at end of URI buffer
-----------------------------------------------------------------------------*/
static char *get_token(struct aps_uri *su,int c)
{
        char *token;
        char *delim;

        token = &su->buf[su->bufpos];

        if (*token==0) {
                token = NULL;
        }
        else {
                delim = strchr(token,c);

                if (delim==NULL) {
                        su->bufpos += strlen(token);
                }
                else {
                        *delim = 0;
                        su->bufpos += strlen(token)+1;
                }
        }

        return token;
}

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  uri_split
Purpose   :  Split URI string into scheme, device and key=value parts
Inputs    :  su  : URI structure
             uri : URI string
Outputs   :  Fills URI structure
Return    :  0 on success, -1 if URI is not valid
-----------------------------------------------------------------------------*/
int uri_split(struct aps_uri *su,const char *uri)
{
        enum {
                GET_SCHEME      = 0,
                GET_DEVICE      = 1,
                GET_KEY         = 2,
                GET_VALUE       = 3,

                DONE            = 4,
                FAIL            = -1
        } state;

        char *token     = NULL;
        char *key       = NULL;
        char *value     = NULL;

        /*reset URI structure*/
        uri_reset(su);

        /*copy URI into structure to be able to parse it*/
        if (strlen(uri)>sizeof(su->buf)-1) {
                return -1;
        }

        strcpy(su->buf,uri);

        /*parse URI*/
        state = GET_SCHEME;

        while (1) {

                switch (state) {
                case GET_SCHEME:
                        token = get_token(su,':');

                        if (token==NULL || strcmp(token,"aps")!=0) {
                                state = FAIL;
                        }
                        else {
                                state = GET_DEVICE;
                        }
                        break;
                case GET_DEVICE:
                        token = get_token(su,'?');

                        if (token==NULL || strlen(token)==0) {
                                state = FAIL;
                        }
                        else {
                                su->device = token;
                                state = GET_KEY;
                        }
                        break;
                case GET_KEY:
                        key = get_token(su,'=');

                        if (key==NULL || strlen(key)==0) {
                                state = DONE;
                        }
                        else {
                                if (su->nopts==OPTS_MAX) {
                                        state = FAIL;
                                }
                                else {
                                        state = GET_VALUE;
                                }
                        }
                        break;
                case GET_VALUE:
                        value = get_token(su,'+');

                        if (value==NULL || strlen(value)==0) {
                                state = FAIL;
                        }
                        else {
                                int i = su->nopts;

                                su->opts[i].key = key;
                                su->opts[i].value = value;

                                su->nopts++;
                                state = GET_KEY;
                        }
                        break;
                case DONE:
                        return 0;
                case FAIL:
                        return -1;
                }
        }
}

/*-----------------------------------------------------------------------------
Name      :  uri_join
Purpose   :  Join URI parts from URI structure and build URI string
Inputs    :  su   : URI structure
             uri  : URI string
             size : URI string size (includes trailing zero)
Outputs   :  Fills URI string
Return    :  0 on success, -1 on error
-----------------------------------------------------------------------------*/
int uri_join(struct aps_uri *su,char *uri,int size)
{
        int i;
        int len;

        assert(su!=NULL);

        len = snprintf(uri,size,"aps:%s",su->device);

        if (len>=size) {
                return -1;
        }
        else {
                uri += len;
                size -= len;
        }

        for (i=0; i<su->nopts; i++) {
                const char *format;

                if (i==0) {
                        format = "?%s=%s";
                }
                else {
                        format = "+%s=%s";
                }

                len = snprintf(uri,size,format,
                                su->opts[i].key,
                                su->opts[i].value);

                if (len>=size) {
                        return -1;
                }
                else {
                        uri += len;
                        size -= len;
                }
        }

        return 0;
}

/*-----------------------------------------------------------------------------
Name      :  uri_reset
Purpose   :  Reset URI structure
Inputs    :  su : URI structure
Outputs   :  URI structure is modified
Return    :  <>
-----------------------------------------------------------------------------*/
void uri_reset(struct aps_uri *su)
{
        assert(su!=NULL);

        memset(su,0,sizeof(*su));
}

/*-----------------------------------------------------------------------------
Name      :  uri_add_device
Purpose   :  Add device information to URI structure
Inputs    :  su     : URI structure
             device : device string
Outputs   :  URI structure is modified
Return    :  0 on success, -1 on error
-----------------------------------------------------------------------------*/
int uri_add_device(struct aps_uri *su,const char *device)
{
        assert(su!=NULL);
 
        if ((su->device = copy_token(su,device))==NULL) {
                return -1;
        }
        else {
                return 0;
        }
}

/*-----------------------------------------------------------------------------
Name      :  uri_add_opt
Purpose   :  Add key=value option to URI structure
Inputs    :  su    : URI structure
             key   : key string
             value : value string
Outputs   :  URI structure is modified
Return    :  0 on success, -1 on error
-----------------------------------------------------------------------------*/
int uri_add_opt(struct aps_uri *su,const char *key,const char *value)
{
        assert(su!=NULL);

        if (su->nopts==OPTS_MAX) {
                return -1;
        }
        else {
                int i = su->nopts;
                
                su->opts[i].key = copy_token(su,key);
                su->opts[i].value = copy_token(su,value);

                if (su->opts[i].key==NULL || su->opts[i].value==NULL) {
                        return -1;
                }
                else {
                        su->nopts++;
                        return 0;
                }
        }
}

/*-----------------------------------------------------------------------------
Name      :  uri_get_device
Purpose   :  Get device information from URI structure
Inputs    :  su : URI structure
Outputs   :  <>
Return    :  device string or NULL if not found
-----------------------------------------------------------------------------*/
const char *uri_get_device(struct aps_uri *su)
{
        assert(su!=NULL);
        
        return su->device;
}

/*-----------------------------------------------------------------------------
Name      :  uri_get_opt
Purpose   :  Get option value from URI structure
Inputs    :  su  : URI structure
             key : key string
Outputs   :  <>
Return    :  value string or NULL if not found
-----------------------------------------------------------------------------*/
const char *uri_get_opt(struct aps_uri *su,const char *key)
{
        const char *value;
        int i;

        assert(su!=NULL);

        value = NULL;
        
        for (i=0; i<su->nopts; i++) {
                if (strcmp(su->opts[i].key,key)==0) {
                        value = su->opts[i].value;
                        break;
                }
        }

        return value;
}


