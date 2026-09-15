/* _OptionsWindow.c

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

#include <stdio.h>

#include <oslib/optionswindow.h>

#include "ccres.h"
#include "_Object.h"
#include "_OptionsWindow.h"

static const FLAGS OptionsWindowFlags[] = {
	{optionswindow_NO_AUTO_MODIFIED  , "optionswindow_NO_AUTO_MODIFIED"  },
	{optionswindow_HAS_SET_BUTTON    , "optionswindow_HAS_SET_BUTTON"    },
	{optionswindow_HAS_CANCEL_BUTTON , "optionswindow_HAS_CANCEL_BUTTON" },
	{optionswindow_HAS_SAVE_BUTTON   , "optionswindow_HAS_SAVE_BUTTON"   },
	{optionswindow_HAS_DEFAULT_BUTTON, "optionswindow_HAS_DEFAULT_BUTTON"}
};

static const OBJECTLIST OptionsWindowObjectList[] = {
	{iol_FLAGS,  "optionswindow_flags:", offsetof(optionswindow_object, flags), OptionsWindowFlags, ELEMENTS(OptionsWindowFlags)},
	{iol_STRING, "title:",      offsetof(optionswindow_object, title),      NULL,               0                           },
	{iol_STRING, "windowlist:", offsetof(optionswindow_object, windowlist), NULL,               0                           }
};

        int optionswindow_t2g(DATA *data, char *pszIn, toolbox_relocatable_object_base * object)
//      =======================================================================================
{
put_objects(data, pszIn, 0, (char *) (object + 1), OptionsWindowObjectList, ELEMENTS(OptionsWindowObjectList));

return sizeof(optionswindow_object);
}


        void optionswindow_g2t(DATA *data, FILE * hf, toolbox_resource_file_object_base * object, char *pszStringTable, char *pszMessageTable, int cbObject)
//      =========================================================================================================================================================
{
get_objects(data, hf, pszStringTable, pszMessageTable, (const char *)(object + 1), OptionsWindowObjectList, ELEMENTS(OptionsWindowObjectList), 1, cbObject);
}
