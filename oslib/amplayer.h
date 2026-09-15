#ifndef amplayer_H
#define amplayer_H

/* C header file for GAMPlayer
 *
 * Minimal hand-written header (not DefMod output) covering only the
 * on-disk resource layout, added to let ccres decompile this gadget.
 *
 * Class number derived from GAMPlayer's own client header
 * (amplayercontrol.h, Justin Fletcher, 2004-2005): SWIChunkBase
 * 0x140440, giving class ((0x140440 - 0x100000) >> 4) = 0x4044,
 * confirmed against captured resource files where this class is
 * exactly 40 bytes (gadget_OBJECT_MEMBERS + type).
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
typedef struct amplayer_object amplayer_object;

/********************
 * Type definitions *
 ********************/
struct amplayer_object
   {  gadget_OBJECT_MEMBERS
      int type;
   };

/************************
 * Constant definitions *
 ************************/
#define class_AMPLAYER                          ((toolbox_class) 0x4044u)

/* GAMPlayer_Type* values for the "type" field - buttons */
#define amplayer_TYPE_STOP                      (0x00)
#define amplayer_TYPE_PAUSE_PLAY                (0x01)
#define amplayer_TYPE_REWIND                    (0x02)
#define amplayer_TYPE_FORWARD                   (0x03)
#define amplayer_TYPE_SKIP                      (0x04)
/* sliders */
#define amplayer_TYPE_POSITION                  (0x10)
#define amplayer_TYPE_VOLUME                    (0x11)
/* displays - timers */
#define amplayer_TYPE_ELAPSED_TIME              (0x20)
#define amplayer_TYPE_TOTAL_TIME                (0x21)
#define amplayer_TYPE_REMAIN_TIME               (0x22)
/* displays - file information */
#define amplayer_TYPE_TRACK                     (0x30)
#define amplayer_TYPE_ALBUM                     (0x31)
#define amplayer_TYPE_ARTIST                    (0x32)
#define amplayer_TYPE_TRACK_NUM                 (0x33)
#define amplayer_TYPE_YEAR                      (0x34)
#define amplayer_TYPE_COMMENT                   (0x35)
#define amplayer_TYPE_TRACK_OR_LEAF             (0x3F)
/* displays - frame information */
#define amplayer_TYPE_BITRATE                   (0x40)
#define amplayer_TYPE_FREQUENCY                 (0x41)
#define amplayer_TYPE_MPEG_VERSION              (0x42)
#define amplayer_TYPE_LAYER                     (0x43)
#define amplayer_TYPE_MODE                      (0x44)
#define amplayer_TYPE_CHANNELS                  (0x45)
/* displays - flags */
#define amplayer_TYPE_ORIGINAL                  (0x50)
#define amplayer_TYPE_COPYRIGHT                 (0x51)
#define amplayer_TYPE_CRC                        (0x52)

#endif
