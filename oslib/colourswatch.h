#ifndef colourswatch_H
#define colourswatch_H

/* C header file for ColourSwatch
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * Class number derived from ColourSwatch's own client header
 * (colourswatch.h, Justin Fletcher, 2004-2005): SWIChunkBase 0x140300,
 * giving class ((0x140300 - 0x100000) >> 4) = 0x4030, confirmed
 * against captured resource files where this class is exactly 44
 * bytes (gadget_OBJECT_MEMBERS + colour + title).
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
typedef struct colourswatch_object colourswatch_object;

/********************
 * Type definitions *
 ********************/
struct colourswatch_object
   {  gadget_OBJECT_MEMBERS
      unsigned int colour;
      toolbox_string_reference title;
   };

typedef bits colourswatch_flags;

/************************
 * Constant definitions *
 ************************/
#define class_COLOURSWATCH                      ((toolbox_class) 0x4030u)
#define colourswatch_INCLUDE_NONE_BUTTON        ((colourswatch_flags) 0x4u)
#define colourswatch_SELECT_NONE_BUTTON         ((colourswatch_flags) 0x8u)

#endif
