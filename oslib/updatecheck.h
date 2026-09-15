#ifndef updatecheck_H
#define updatecheck_H

/* C header file for UpdateCheck
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this object.
 *
 * UpdateCheck_ObjectClass (0x1002C0) is confirmed both in the
 * module's own client header (updatecheck.h, Justin Fletcher) and in
 * Justin Fletcher's personal Pineapple allocation ledger. The
 * template struct is taken directly from that client header; "url"
 * and "program_name" are plain (non-indirected) string references,
 * following the same convention as quit_object's
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
typedef struct updatecheck_object updatecheck_object;

/********************
 * Type definitions *
 ********************/
typedef bits updatecheck_flags;

struct updatecheck_object
   {  updatecheck_flags flags;
      toolbox_string_reference program_name;
      int current_version;
      toolbox_string_reference url;
   };

/************************
 * Constant definitions *
 ************************/
#define UpdateCheck_ClassSWI                    0x1002C0
#define class_UPDATECHECK                       ((toolbox_class) 0x1002C0u)
#define updatecheck_GENERATE_ABOUT_TO_BE_SHOWN  ((updatecheck_flags) 0x1u)
#define updatecheck_FILENAME_SAVE_BOX           ((updatecheck_flags) 0x0u)
#define updatecheck_FILENAME_AUTO               ((updatecheck_flags) 0x2u)
#define updatecheck_FILENAME_REQUEST            ((updatecheck_flags) 0x4u)
#define updatecheck_FILENAME_MASK               ((updatecheck_flags) 0x6u)
#define updatecheck_DOWNLOAD_OPEN_DIR           ((updatecheck_flags) 0x0u)
#define updatecheck_DOWNLOAD_RUN                ((updatecheck_flags) 0x8u)
#define updatecheck_DOWNLOAD_NOTIFY             ((updatecheck_flags) 0x10u)
#define updatecheck_DOWNLOAD_NONE               ((updatecheck_flags) 0x18u)
#define updatecheck_DOWNLOAD_MASK               ((updatecheck_flags) 0x18u)

#endif
