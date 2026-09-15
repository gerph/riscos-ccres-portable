/* _UpdateCheck.c

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

#include <oslib/updatecheck.h>

#include "ccres.h"
#include "_Object.h"
#include "_UpdateCheck.h"

/* filename_flags and download_flags are 2-bit sub-fields of "flags"
   (see updatecheck_FILENAME_MASK/updatecheck_DOWNLOAD_MASK), not
   independent bits, so both are listed alongside the single-bit
   flags in one OR-decoded table rather than as separate fields. */
static const FLAGS UpdateCheckFlags[] = {
	{updatecheck_GENERATE_ABOUT_TO_BE_SHOWN, "updatecheck_GENERATE_ABOUT_TO_BE_SHOWN"},
	{updatecheck_FILENAME_AUTO             , "updatecheck_FILENAME_AUTO"             },
	{updatecheck_FILENAME_REQUEST          , "updatecheck_FILENAME_REQUEST"          },
	{updatecheck_DOWNLOAD_RUN              , "updatecheck_DOWNLOAD_RUN"              },
	{updatecheck_DOWNLOAD_NOTIFY           , "updatecheck_DOWNLOAD_NOTIFY"           }
};

static const OBJECTLIST UpdateCheckObjectList[] = {
	{iol_FLAGS,  "updatecheck_flags:", offsetof(updatecheck_object, flags), UpdateCheckFlags, ELEMENTS(UpdateCheckFlags)},
	{iol_STRING, "program_name:",    offsetof(updatecheck_object, program_name),    NULL,             0                         },
	{iol_INT,    "current_version:", offsetof(updatecheck_object, current_version), NULL,             0                         },
	{iol_STRING, "url:",             offsetof(updatecheck_object, url),             NULL,             0                         }
};

        int updatecheck_t2g(DATA *data, char *pszIn, toolbox_relocatable_object_base * object)
//      =====================================================================================
{
put_objects(data, pszIn, 0, (char *) (object + 1), UpdateCheckObjectList, ELEMENTS(UpdateCheckObjectList));

return sizeof(updatecheck_object);
}


        void updatecheck_g2t(DATA *data, FILE * hf, toolbox_resource_file_object_base * object, char *pszStringTable, char *pszMessageTable, int cbObject)
//      ======================================================================================================================================================
{
get_objects(data, hf, pszStringTable, pszMessageTable, (const char *)(object + 1), UpdateCheckObjectList, ELEMENTS(UpdateCheckObjectList), 1, cbObject);
}
