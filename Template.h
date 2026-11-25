/* Template.h

   Copyright (c) 2003-2003 Dave Appleby
   Copyright (c) 2003-2007 John Tytgat

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

/* Layout of a Wimp Templates file (filetype &FEC): a header, an index of
   window entries ended by a zero word, the window definitions, and then the
   font data, if any. Moved from Convert.c so that Skeleton.c can build one. */

#ifndef TEMPLATE_HEADER_INCLUDED
#define TEMPLATE_HEADER_INCLUDED

#include "ccres.h"

#define template_NO_FONTS 	((bits)-1)
#define template_TYPE_WINDOW	((bits)0x1u)

typedef struct {
	bits font_offset; /* Can be template_NO_FONTS too */
	bits reserved[3];
} template_header;

typedef struct {
	bits offset;
	bits size;
	int type;          /* See template_TYPE_* */
	char name[12];
} template_index;

typedef struct {
	bits x_point_size;
	bits y_point_size;
	char font_name[40];
} template_font_data;

#endif
