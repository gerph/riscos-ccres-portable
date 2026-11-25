/* _Window.h

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

#ifndef WINDOW_HEADER_INCLUDED
#define WINDOW_HEADER_INCLUDED

#include <stdio.h>

#include "ccres.h"

int window_t2g(DATA *data, char *pszIn, toolbox_relocatable_object_base * object);
void window_g2t(DATA *data, FILE * hf, toolbox_resource_file_object_base * object, char *pszStringTable, char *pszMessageTable, int cbObject);
/*******************************************************************
 Function:      window_text2template
 Description:   Encode the text form of a wimp_window and its icons
                into the fixed part of a Template window definition.
                The indirected data is not copied; it is left in
                data->StringTable, with offsets in the icon data
                relative to the window base, so the caller must place
                data->StringTable.ref bytes from the table immediately
                after the icons, then reset the table.
 Parameters:    data = conversion session
                pszIn = text of the window object
                nOffset = negated size of the fixed part (window base
                          plus all icons), used to make the indirected
                          data offsets relative to the window base
                object = buffer to receive the window, with room for
                         the window base and every icon
 Returns:       size of the fixed part written, in bytes
 ******************************************************************/
int window_text2template(DATA *data, char *pszIn, int nOffset, wimp_window_base * object);
void window_template2text(DATA *data, FILE * hf, char *pszBuff);

/*******************************************************************
 Function:      window_gadgets
 Description:   Give the table of the gadget classes known in windows,
                so that a skeleton of each can be built.
 Parameters:    pnGadgets = updated with the number of entries
 Returns:       pointer to the table
 ******************************************************************/
const GADGETS *window_gadgets(int *pnGadgets);

#endif
