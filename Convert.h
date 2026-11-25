/* Convert.h

   Copyright (c) 2005-2007 John Tytgat

   This file is part of CCres.

   CCres is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   CCres is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with CCres; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CONVERT_HEADER_INCLUDED
#define CONVERT_HEADER_INCLUDED

#include "ccres.h"

// Returns 0 for error, 1 otherwise.
int ccres_initialise(DATA *sessionP);
// Returns 0 for error, 1 otherwise.
int ccres_finish(void);
// Returns 0 for error, 1 otherwise.
bool ccres_convert(DATA *data, char *pszOutFile);

/*******************************************************************
 Function:      ccres_skeleton
 Description:   Write a text file holding one of every kind of object
                (with every gadget) for a Resource file, or a window
                with every kind of icon for a Templates file, with each
                field at its zero value. With data->fDescribe set, each
                field is annotated with '#@' comment lines describing it
                (see describe_field in _Object.h). The text can be
                converted back to a binary file.
 Parameters:    data = conversion session
                nFiletype = osfile_TYPE_RESOURCE or osfile_TYPE_TEMPLATE
                pszOutFile = text file to write
 Returns:       true if written, false on error (reported)
 ******************************************************************/
bool ccres_skeleton(DATA *data, bits nFiletype, char *pszOutFile);

#endif
