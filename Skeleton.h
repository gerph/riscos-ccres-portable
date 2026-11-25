/*******************************************************************
 * File:        Skeleton.h
 * Purpose:     Build in memory a Resource or Templates file holding one
 *              of every kind of object, gadget and icon that CCres
 *              knows, so that decoding it gives a complete skeleton of
 *              the text format. With DATA.fDescribe set, the decoders
 *              also annotate each field (see describe_field in
 *              _Object.h), which is used to generate documentation.
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

#ifndef SKELETON_HEADER_INCLUDED
#define SKELETON_HEADER_INCLUDED

#include "ccres.h"

/*******************************************************************
 Function:      skeleton_resource
 Description:   Build a Toolbox Resource file image with one object of
                every class, each zero filled at its full size and with
                its usual version. The window has one keyboard shortcut
                and one gadget of every class; the menu has a text entry
                and a sprite entry.
 Parameters:    pClasses = table of the object classes
                nClasses = number of entries in pClasses
                ppImage = updated with the image, allocated with MyAlloc
                pcbImage = updated with the size of the image
 Returns:       true if built, false if memory could not be allocated
 ******************************************************************/
bool skeleton_resource(const CLASSES *pClasses, int nClasses, char **ppImage, int *pcbImage);

/*******************************************************************
 Function:      skeleton_template
 Description:   Build a Wimp Templates file image with one window, with
                an indirected text title and one icon for each kind of
                icon data, and one font entry used by an anti-aliased
                icon.
 Parameters:    ppImage = updated with the image, allocated with MyAlloc
                pcbImage = updated with the size of the image
 Returns:       true if built, false if memory could not be allocated
 ******************************************************************/
bool skeleton_template(char **ppImage, int *pcbImage);

#endif
