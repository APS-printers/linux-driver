/******************************************************************************
* COMPANY       : APS ENGINEERING
* PROJECT       : LINUX DRIVER
*******************************************************************************
* NAME          : version.h
* DESCRIPTION   : APS Library - version information and support macros
* CVS           : $Id: version.h,v 1.4 2006/03/16 09:40:15 nicolas Exp $
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
*   13feb2006   nico    Initial revision
******************************************************************************/

#ifndef _VERSION_H
#define _VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <aps/version.def>

/* use
 *  #if APS_VERSION_CODE >= APS_VERSION(x,y,z)
 * or similar to check for library versions
 */

#define APS_VERSION(a,b,c)      (((a) << 16) + ((b) << 8) + (c))

#define APS_VERSION_CODE        APS_VERSION(APS_MAJOR,APS_MINOR,APS_BUGFIX)

#ifdef __cplusplus
}
#endif

#endif /*_VERSION_H*/

