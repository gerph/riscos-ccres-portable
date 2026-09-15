#ifndef mdnsservice_H
#define mdnsservice_H

/* C header file for GMDNSService
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * Class number derived from GMDNSService's own client header
 * (MDNSService.h, Justin Fletcher, 2004-2005): SWIChunkBase 0x140500
 * ("Allocated"), giving class ((0x140500 - 0x100000) >> 4) = 0x4050,
 * confirmed against captured resource files where this class is
 * exactly 56 bytes (gadget_OBJECT_MEMBERS + event + background +
 * foreground + service + domain).
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
typedef struct mdnsservice_object mdnsservice_object;

/********************
 * Type definitions *
 ********************/
struct mdnsservice_object
   {  gadget_OBJECT_MEMBERS
      int event;
      unsigned int background;
      unsigned int foreground;
      toolbox_string_reference service;
      toolbox_string_reference domain;
   };

/************************
 * Constant definitions *
 ************************/
#define class_MDNSSERVICE                       ((toolbox_class) 0x4050u)

#endif
