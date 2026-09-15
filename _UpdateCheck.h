/* _UpdateCheck.h

   Copyright (c) 2003-2003 Dave Appleby
   Copyright (c) 2003-2007 John Tytgat
   Copyright (c) 2026 Charles Ferguson (Gerph)

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

#ifndef UPDATECHECK_HEADER_INCLUDED
#define UPDATECHECK_HEADER_INCLUDED

#include <stdio.h>

#include "ccres.h"

int updatecheck_t2g(DATA *data, char *pszIn, toolbox_relocatable_object_base * object);
void updatecheck_g2t(DATA *data, FILE * hf, toolbox_resource_file_object_base * object, char *pszStringTable, char *pszMessageTable, int cbObject);

#endif
