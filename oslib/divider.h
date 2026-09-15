#ifndef divider_H
#define divider_H

/* C header file for GDivider
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * Class number derived from GDivider's own client header (divider.h,
 * Justin Fletcher, 2005): SWIChunkBase 0x140580 ("allocated"), giving
 * class ((0x140580 - 0x100000) >> 4) = 0x4058, confirmed against
 * captured resource files where this class is exactly 36 bytes
 * (gadget_OBJECT_MEMBERS with no extra fields).
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
typedef struct divider_object divider_object;

/********************
 * Type definitions *
 ********************/
struct divider_object
   {  gadget_OBJECT_MEMBERS
   };

typedef bits divider_flags;

/************************
 * Constant definitions *
 ************************/
#define class_DIVIDER                           ((toolbox_class) 0x4058u)
#define divider_VERTICAL                        ((divider_flags) 0x1u)

#endif
