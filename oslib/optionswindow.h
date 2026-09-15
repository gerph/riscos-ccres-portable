#ifndef optionswindow_H
#define optionswindow_H

/* C header file for OptionsWindow
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this object.
 *
 * OptionsWindow_ObjectClass (0x100280) is confirmed both in the
 * module's own cmhg ("swi-chunk-base-number: 0x100280 ; Allocated")
 * and in Justin Fletcher's personal Pineapple allocation ledger. The
 * template struct is taken directly from the module's own client
 * header (optionswindow.h, Justin Fletcher, 2004-2005); the "title"
 * and "windowlist" fields are plain (non-indirected) string
 * references, following the same convention as quit_object's
 * alternative_window_name.
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

/************************************
 * Structure and union declarations *
 ************************************/
typedef struct optionswindow_object optionswindow_object;

/********************
 * Type definitions *
 ********************/
typedef bits optionswindow_flags;

struct optionswindow_object
   {  optionswindow_flags flags;
      toolbox_string_reference title;
      toolbox_string_reference windowlist;
   };

/************************
 * Constant definitions *
 ************************/
#define OptionsWindow_ClassSWI                  0x100280
#define class_OPTIONSWINDOW                     ((toolbox_class) 0x100280u)
#define optionswindow_NO_AUTO_MODIFIED          ((optionswindow_flags) 0x1u)
#define optionswindow_HAS_SET_BUTTON            ((optionswindow_flags) 0x2u)
#define optionswindow_HAS_CANCEL_BUTTON         ((optionswindow_flags) 0x4u)
#define optionswindow_HAS_SAVE_BUTTON           ((optionswindow_flags) 0x8u)
#define optionswindow_HAS_DEFAULT_BUTTON        ((optionswindow_flags) 0x10u)

#endif
