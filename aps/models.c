/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : models.c
* DESCRIPTION   : APS library - models database
* CVS           : $Id: models.c,v 1.4 2009/01/16 16:14:34 pierre Exp $
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
*   15may2006   nico    Initial revision
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aps/aps.h>
#include <aps/aps-private.h>

/* PRIVATE DEFINITIONS ------------------------------------------------------*/

/* Note pnb: under unix a "_" appared at prefix of printer name ??? */
/*the models database*/
static const model_t models_table[] = {
{MODEL_CP290MRS,    APS_MRS,      432,  "CP290MRS",   "CP 290 MRS"},
{MODEL_CP324MRS,    APS_MRS,      576,  "CP324MRS",   "CP 324 MRS"},
{MODEL_CP424MRS,    APS_MRS,      864,  "CP424MRS",   "CP 424 MRS"},
{MODEL_CP295MRS,    APS_MRS,      384,  "CP295MRS",   "CP 295 MRS"},
{MODEL_CP305MRS,    APS_MRS,      576,  "CP305MRS",   "CP 305 MRS"},
{MODEL_CP405MRS,    APS_MRS,      832,  "CP405MRS",   "CP 405 MRS"},
{MODEL_CP205MRS,    APS_MRS,      384,  "CP205MRS",   "CP 205 MRS"},
{MODEL_EPM205MRS,   APS_MRS,      384,  "EPM205MRS",  "EPM 205 MRS"},
{MODEL_EPM224MRS,   APS_MRS,      384,  "EPM224MRS",  "EPM 224 MRS"},
{MODEL_EPM203MRS,   APS_MRS,      384,  "EPM203MRS",  "EPM 203 MRS"},
{MODEL_BPM205,      APS_MRS,      384,  "BPM205",     "BPM 205"},
{MODEL_BPM224L,     APS_MRS,      384,  "BPM224L",    "BPM224L"},
{MODEL_BPM224,      APS_MRS,      384,  "BPM224",     "BPM224"},
{MODEL_CP290HRS,    APS_HRS,      432,  "CP290HRS",   "CP290HRS"},
{MODEL_CP324HRSW,   APS_HRS,      640,  "CP324HRSW",  "CP324HRS         W"},
{MODEL_CP324HRS,    APS_HRS,      576,  "CP324HRS",   "CP324HRS"},
{MODEL_CP424HRS,    APS_HRS,      864,  "CP424HRS",   "CP424HRS"},
{MODEL_KCP200,      APS_KCP,      432,  "KCP200",     "KCP200"},

{MODEL_HSP3100FC,   APS_HSP,      640,  "HSP3100FC",  "_HSP"},
{MODEL_HSP3100FC,   APS_HSP,      640,  "HSP3100FC",  "_HSP3100FC"},

{MODEL_POS1525,     APS_HSP,      640,  "POS1525",    "_HRS"},
{MODEL_EPM205HRS,   APS_HRS,      384,  "EPM205HRS",  "EPM205HRS"},
{MODEL_EPM205HRS,   APS_HRS,      384,  "EPM205HRS",  "EPM205-HRS"},

{MODEL_EPM207HRS,   APS_HRS,      384,  "EPM207HRS",  "EPM207-HRS"},

{MODEL_FCB500,      APS_HSP,      640,  "FCB500",     "_FCB500"},
{MODEL_LCB500,      APS_HRS,      640,  "LCB500",     "LCB500"},

{MODEL_LPM400,      APS_HRS,      448,  "LPM400",     "LPM400"},

{MODEL_UNKNOWN,     APS_UNKNOWN,  0,    "unknown",    ""}
};

#define NUM_MODELS      (int)(sizeof(models_table)/sizeof(models_table[0]))

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* PUBLIC FUNCTIONS ---------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name      :  model_find_by_number
Purpose   :  Find a model in the database by its model number
Inputs    :  model : model number
Outputs   :  <>
Return    :  Pointer to database entry or NULL if not found
-----------------------------------------------------------------------------*/
const model_t *model_find_by_number(int model)
{
        int i;

        for (i=0; i<NUM_MODELS; i++) {
                if (models_table[i].model==model) {
                        return &models_table[i];
                }
        }

        return NULL;
}

/*-----------------------------------------------------------------------------
Name      :  model_find_by_id
Purpose   :  Find a model in the database by its identity string
Inputs    :  s : identity string
Outputs   :  <>
Return    :  Pointer to database entry or NULL if not found
-----------------------------------------------------------------------------*/
const model_t *model_find_by_id(const char *s)
{
        int i;

        for (i=0; i<NUM_MODELS; i++) {
                if (strstr(s,models_table[i].id)==s) {
                        return &models_table[i];
                }
        }

        return NULL;
}

