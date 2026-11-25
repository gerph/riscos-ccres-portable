/* _Object.h

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

#ifndef OBJECT_HEADER_INCLUDED
#define OBJECT_HEADER_INCLUDED

#include <stdio.h>

#include "ccres.h"

void put_objects(DATA *data, char *pszIn, int nOffset, char *object, const OBJECTLIST *Objectlist, int nObjects);
void get_objects(DATA *data, FILE * hf, char *pszStringTable, char *pszMessageTable, const char *objectP, const OBJECTLIST *ObjectList, int nObjects, int nIndent, int cbObject);

/*******************************************************************
 Function:      describe_field
 Description:   Write '#@' comment lines describing a field which has
                just been written, for --describe. The lines are:
                  #@ values <n> kind=flags|enum
                  #@ value <n> <hex value> <name>
                for each flag or enumeration table the first time it
                is used, followed by:
                  #@ field <name> type=<type> [key=value ...]
                where the keys are limit= (the limit or size field of a
                string), bits= (width of flags), values= and buttons=
                (table numbers), size= (characters in a fixed string)
                and the word 'terminated'.
 Parameters:    hf = file to write to
                pszIndent = indent of the field
                ObjectList = the field's entry in its object list
 Returns:       none
 ******************************************************************/
void describe_field(FILE *hf, const char *pszIndent, const OBJECTLIST *ObjectList);

/*******************************************************************
 Function:      describe_variant
 Description:   Write a '#@ variant <name> when=<condition>' comment
                line, for --describe, saying which form of a block the
                fields that follow belong to.
 Parameters:    hf = file to write to
                pszIndent = indent of the block's fields
                pszName = short name of the variant (no spaces)
                pszWhen = condition for the variant (no spaces)
 Returns:       none
 ******************************************************************/
void describe_variant(FILE *hf, const char *pszIndent, const char *pszName, const char *pszWhen);
char *next_object(char ** pszIn, char *pszEnd);
char *object_end(DATA *data, char *pszIn, char *pszEnd);
void object_text2resource(DATA *data, FILE * hf, char *pszIn, char *pszOut, const CLASSES *pClass);
void object_resource2text(DATA *data, FILE * hf, toolbox_relocatable_object_base * object, object2text o2t);

#endif
