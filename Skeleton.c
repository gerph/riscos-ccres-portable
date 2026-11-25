/*******************************************************************
 * File:        Skeleton.c
 * Purpose:     Build Resource and Templates file images holding one of
 *              every kind of object, gadget and icon, for decoding into
 *              a complete skeleton of the text format.
 * Author:      Gerph
 ******************************************************************/

/* Copyright (c) 2026 Gerph

   This file is part of CCres.

   CCres is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   CCres is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <oslib/keyboardshortcut.h>
#include <oslib/menu.h>
#include <oslib/toolbox.h>
#include <oslib/wimp.h>
#include <oslib/window.h>

#include "ccres.h"
#include "Skeleton.h"
#include "Template.h"
#include "Utils.h"
#include "_Window.h"

/* Number of entries in the skeleton menu: one text and one sprite entry */
#define SKELETON_MENU_ENTRIES 2

/* Number of keyboard shortcuts in the skeleton window */
#define SKELETON_SHORTCUTS 1

/* Template name of the skeleton window (at most 11 characters) */
#define SKELETON_TEMPLATE_NAME "skeleton"

/* Font used by the anti-aliased icon in the skeleton window, and its size
   in sixteenths of a point */
#define SKELETON_FONT_NAME "Homerton.Medium"
#define SKELETON_FONT_SIZE (12 * 16)

/* Font handle of that font within the Templates file (fonts are numbered
   from 1) */
#define SKELETON_FONT_HANDLE 1

/* Sprite area value meaning the Wimp sprite area */
#define SKELETON_WIMP_SPRITE_AREA 1

/* Most characters of an object name, which must be terminated within its
   12 byte field */
#define SKELETON_OBJECT_NAME_MAX 11

/* Icon flags for each kind of icon data in the skeleton window, in the
   order the icons appear */
static const bits SkeletonIconFlags[] = {
	0,
	wimp_ICON_TEXT,
	wimp_ICON_SPRITE,
	wimp_ICON_TEXT | wimp_ICON_SPRITE,
	wimp_ICON_INDIRECTED | wimp_ICON_TEXT,
	wimp_ICON_INDIRECTED | wimp_ICON_SPRITE,
	wimp_ICON_INDIRECTED | wimp_ICON_TEXT | wimp_ICON_SPRITE,
	wimp_ICON_INDIRECTED | wimp_ICON_TEXT | wimp_ICON_ANTI_ALIASED | (SKELETON_FONT_HANDLE << wimp_ICON_FONT_HANDLE_SHIFT)
};


/*******************************************************************
 Function:      put_word
 Description:   Store a 32-bit word in an image at a byte offset. Used
                for fields which are pointers on RISC OS but 32-bit
                values in the file, so that no casts are needed.
 Parameters:    pImage = start of the structure
                nOffset = byte offset of the field
                nValue = value to store
 Returns:       none
 ******************************************************************/
static void put_word(char *pImage, int nOffset, int32_t nValue)
{
	memcpy(pImage + nOffset, &nValue, sizeof(nValue));
}


/*******************************************************************
 Function:      object_body_size
 Description:   Work out the size of the body of a skeleton object,
                including the shortcuts and gadgets of a window and the
                entries of a menu.
 Parameters:    pClass = class of the object
 Returns:       size of the body in bytes
 ******************************************************************/
static int object_body_size(const CLASSES *pClass)
{
	const GADGETS *pGadgets;
	int cb, n, nGadgets;

	cb = pClass->size;
	if (pClass->class_no == Window_ClassSWI) {
		pGadgets = window_gadgets(&nGadgets);
		cb += SKELETON_SHORTCUTS * sizeof(keyboardshortcut_object);
		for (n = 0; n < nGadgets; n++) {
			cb += pGadgets[n].size;
		}
	} else if (pClass->class_no == Menu_ClassSWI) {
		cb += SKELETON_MENU_ENTRIES * sizeof(menu_entry_object);
	}
	return cb;
}


/*******************************************************************
 Function:      fill_object_body
 Description:   Fill in the parts of a zeroed skeleton object body that
                say what it contains: the shortcuts and gadgets of a
                window, and the entries of a menu.
 Parameters:    pClass = class of the object
                pBody = the zeroed body
 Returns:       none
 ******************************************************************/
static void fill_object_body(const CLASSES *pClass, char *pBody)
{
	const GADGETS *pGadgets;
	int n, nGadgets, nOffset;

	if (pClass->class_no == Window_ClassSWI) {
		pGadgets = window_gadgets(&nGadgets);
		nOffset = sizeof(window_object_base);
		put_word(pBody, offsetof(window_object_base, shortcut_count), SKELETON_SHORTCUTS);
		put_word(pBody, offsetof(window_object_base, shortcuts), nOffset);
		nOffset += SKELETON_SHORTCUTS * sizeof(keyboardshortcut_object);
		put_word(pBody, offsetof(window_object_base, gadget_count), nGadgets);
		put_word(pBody, offsetof(window_object_base, gadgets), nOffset);
		for (n = 0; n < nGadgets; n++) {
			put_word(pBody + nOffset, offsetof(gadget_object_base, class_no_and_size),
			         (int32_t) ((pGadgets[n].size << 16) | pGadgets[n].class_no));
			nOffset += pGadgets[n].size;
		}
	} else if (pClass->class_no == Menu_ClassSWI) {
		menu_entry_object *entry;

		put_word(pBody, offsetof(menu_object_base, entry_count), SKELETON_MENU_ENTRIES);
		/* The second entry shows a sprite rather than text */
		entry = (menu_entry_object *) (pBody + sizeof(menu_object_base)) + 1;
		put_word((char *) entry, offsetof(menu_entry_object, flags), menu_ENTRY_IS_SPRITE);
	}
}


bool skeleton_resource(const CLASSES *pClasses, int nClasses, char **ppImage, int *pcbImage)
{
	toolbox_resource_file_base *header;
	toolbox_relocatable_object_base *object;
	char *pImage, *p;
	int cb, cbBody, n, nName;
	const char *pszSuffix;

	/* Header, then each object with no string, message or relocation table */
	cb = sizeof(toolbox_resource_file_base);
	for (n = 0; n < nClasses; n++) {
		cb += sizeof(toolbox_relocatable_object_base) + object_body_size(&pClasses[n]);
	}
	if ((pImage = MyAlloc(cb)) == NULL) {
		return false;
	}

	header = (toolbox_resource_file_base *) pImage;
	header->file_id = RESF;
	header->version = 101;
	header->header_size = sizeof(toolbox_resource_file_base);

	p = pImage + sizeof(toolbox_resource_file_base);
	for (n = 0; n < nClasses; n++) {
		cbBody = object_body_size(&pClasses[n]);
		object = (toolbox_relocatable_object_base *) p;
		object->string_table_offset = -1;
		object->message_table_offset = -1;
		object->relocation_table_offset = -1;
		object->rf_obj.class_no = pClasses[n].class_no;
		object->rf_obj.version = pClasses[n].version;
		/* Named after the class, without "_object" */
		pszSuffix = strstr(pClasses[n].name, "_object");
		nName = (pszSuffix != NULL) ? (int) (pszSuffix - pClasses[n].name) : (int) strlen(pClasses[n].name);
		if (nName > SKELETON_OBJECT_NAME_MAX) {
			nName = SKELETON_OBJECT_NAME_MAX;
		}
		memcpy(object->rf_obj.name, pClasses[n].name, nName);
		object->rf_obj.header_size = sizeof(toolbox_resource_file_object_base);
		object->rf_obj.body_size = cbBody;
		object->rf_obj.size = sizeof(toolbox_resource_file_object_base) + cbBody;
		fill_object_body(&pClasses[n], (char *) (object + 1));
		p += sizeof(toolbox_relocatable_object_base) + cbBody;
	}

	*ppImage = pImage;
	*pcbImage = cb;
	return true;
}


bool skeleton_template(char **ppImage, int *pcbImage)
{
	template_header *header;
	template_index *index;
	template_font_data *font;
	char *pImage, *pWindow, *pIcon;
	int cb, cbWindow, nWindow, nStrings, nIcons, n;

	nIcons = ELEMENTS(SkeletonIconFlags);
	/* All the indirected data points at one empty string after the icons,
	   padded to a word */
	nStrings = sizeof(wimp_window_base) + nIcons * sizeof(wimp_icon);
	cbWindow = nStrings + sizeof(int32_t);
	/* Header, one index entry and its terminator, the window, the font */
	nWindow = sizeof(template_header) + sizeof(template_index) + sizeof(int32_t);
	cb = nWindow + cbWindow + sizeof(template_font_data);
	if ((pImage = MyAlloc(cb)) == NULL) {
		return false;
	}

	header = (template_header *) pImage;
	header->font_offset = nWindow + cbWindow;

	index = (template_index *) (header + 1);
	index->offset = nWindow;
	index->size = cbWindow;
	index->type = template_TYPE_WINDOW;
	n = strlen(SKELETON_TEMPLATE_NAME);
	memcpy(index->name, SKELETON_TEMPLATE_NAME, n);
	index->name[n] = '\r';

	pWindow = pImage + nWindow;
	put_word(pWindow, offsetof(wimp_window_base, next), (int32_t) wimp_TOP);
	put_word(pWindow, offsetof(wimp_window_base, title_flags), wimp_ICON_TEXT | wimp_ICON_INDIRECTED);
	put_word(pWindow, offsetof(wimp_window_base, sprite_area), SKELETON_WIMP_SPRITE_AREA);
	put_word(pWindow, offsetof(wimp_window_base, title_data) + offsetof(wimp_icon_data, indirected_text.text), nStrings);
	put_word(pWindow, offsetof(wimp_window_base, title_data) + offsetof(wimp_icon_data, indirected_text.validation), nStrings);
	put_word(pWindow, offsetof(wimp_window_base, title_data) + offsetof(wimp_icon_data, indirected_text.size), 1);
	put_word(pWindow, offsetof(wimp_window_base, icon_count), nIcons);

	pIcon = pWindow + sizeof(wimp_window_base);
	for (n = 0; n < nIcons; n++, pIcon += sizeof(wimp_icon)) {
		bits flags = SkeletonIconFlags[n];
		char *pData = pIcon + offsetof(wimp_icon, data);

		put_word(pIcon, offsetof(wimp_icon, flags), (int32_t) flags);
		if ((flags & (wimp_ICON_INDIRECTED | wimp_ICON_SPRITE | wimp_ICON_TEXT)) == (wimp_ICON_INDIRECTED | wimp_ICON_SPRITE)) {
			put_word(pData, offsetof(wimp_icon_data, indirected_sprite.id), nStrings);
			put_word(pData, offsetof(wimp_icon_data, indirected_sprite.area), SKELETON_WIMP_SPRITE_AREA);
			put_word(pData, offsetof(wimp_icon_data, indirected_sprite.size), 1);
		} else if (flags & wimp_ICON_INDIRECTED) {
			/* The indirected text and text-and-sprite layouts are the same */
			put_word(pData, offsetof(wimp_icon_data, indirected_text.text), nStrings);
			put_word(pData, offsetof(wimp_icon_data, indirected_text.validation), nStrings);
			put_word(pData, offsetof(wimp_icon_data, indirected_text.size), 1);
		}
	}
	pWindow[nStrings] = '\r';

	font = (template_font_data *) (pImage + header->font_offset);
	font->x_point_size = SKELETON_FONT_SIZE;
	font->y_point_size = SKELETON_FONT_SIZE;
	n = strlen(SKELETON_FONT_NAME);
	memcpy(font->font_name, SKELETON_FONT_NAME, n);
	font->font_name[n] = '\r';

	*ppImage = pImage;
	*pcbImage = cb;
	return true;
}
