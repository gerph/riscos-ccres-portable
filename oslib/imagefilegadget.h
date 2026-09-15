#ifndef imagefilegadget_H
#define imagefilegadget_H

/* C header file for ImageFileGadget
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * The class number is not from an OSLib DefMod run: ImageFileGadget's
 * Toolbox gadget type number was never published as a full OSLib
 * interface, but its own client header (imagefilegadget.h, Justin
 * Fletcher, 2004-2005) records it directly as 0x4024, which also
 * matches the general Pineapple-era convention of
 * ((SWIChunkBase - 0x100000) >> 4) with SWIChunkBase 0x140240.
 */

#ifndef types_H
#include "oslib/types.h"
#endif

#ifndef os_H
#include "oslib/os.h"
#endif

#ifndef toolbox_H
#include "oslib/toolbox.h"
#endif

#ifndef gadget_H
#include "oslib/gadget.h"
#endif

/************************************
 * Structure and union declarations *
 ************************************/
typedef struct imagefilegadget_object imagefilegadget_object;

/********************
 * Type definitions *
 ********************/
struct imagefilegadget_object
   {  gadget_OBJECT_MEMBERS
      int event;
      unsigned int unused;
      unsigned int background;
   };

typedef bits imagefilegadget_flags;

/************************
 * Constant definitions *
 ************************/
#define class_IMAGEFILEGADGET                   ((toolbox_class) 0x4024u)
#define imagefilegadget_NO_BORDER               ((imagefilegadget_flags) 0x1u)
#define imagefilegadget_COLOUR_MAP_WHOLE         ((imagefilegadget_flags) 0x2u)

#endif
