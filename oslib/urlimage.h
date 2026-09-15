#ifndef urlimage_H
#define urlimage_H

/* C header file for GURLImage
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * Class number derived from GURLImage's own client header
 * (external.h, Justin Fletcher, 2004-2005): SWIChunkBase 0x140540
 * ("Allocated"), giving class ((0x140540 - 0x100000) >> 4) = 0x4054,
 * confirmed against captured resource files where this class is
 * exactly 52 bytes (gadget_OBJECT_MEMBERS + url + event_click +
 * foreground + background).
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
typedef struct urlimage_object urlimage_object;

/********************
 * Type definitions *
 ********************/
struct urlimage_object
   {  gadget_OBJECT_MEMBERS
      toolbox_string_reference url;
      int event_click;
      unsigned int foreground;
      unsigned int background;
   };

typedef bits urlimage_flags;

/************************
 * Constant definitions *
 ************************/
#define class_URLIMAGE                          ((toolbox_class) 0x4054u)
#define urlimage_RETAIN_SOURCE_IMAGE             ((urlimage_flags) 0x1u)
#define urlimage_DIRECT_RENDER                   ((urlimage_flags) 0x2u)
#define urlimage_BORDER                          ((urlimage_flags) 0x4u)
#define urlimage_DELIVER_CLICK                   ((urlimage_flags) 0x8u)
#define urlimage_DELIVER_COMPLETE                ((urlimage_flags) 0x10u)
#define urlimage_PROGRESS_PERCENTAGE             ((urlimage_flags) 0x20u)

#endif
