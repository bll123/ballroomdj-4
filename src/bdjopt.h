#ifndef INC_BDJOPT_H
#define INC_BDJOPT_H

#include "datafile.h"

typedef enum {
  WRITE_TAGS_ALL,
  WRITE_TAGS_BDJ_ONLY,
  WRITE_TAGS_NONE,
} bdjwritetags_t;

typedef enum {
  FADETYPE_TRIANGLE,
  FADETYPE_QUARTER_SINE,
  FADETYPE_HALF_SINE,
  FADETYPE_LOGARITHMIC,
  FADETYPE_INVERTED_PARABOLA,
} bdjfadetype_t;

typedef enum {
  MOBILEMQ_OFF,
  MOBILEMQ_LOCAL,
  MOBILEMQ_INTERNET,
} bdjmobilemq_t;

/* development notes
 * u - in use
 * d - display working in configui
 * s - save working in configui
 */

typedef enum {
  OPT_G_AUTOORGANIZE,
  OPT_G_CHANGESPACE,
  OPT_G_DEBUGLVL,                 // u
  OPT_G_ITUNESSUPPORT,            //   d
  OPT_G_LOADDANCEFROMGENRE,       //   d
  OPT_G_MUSICDIRDFLT,
  OPT_G_PATHFMT,
  OPT_G_PATHFMT_CL,
  OPT_G_PATHFMT_CLVA,
  OPT_G_PATHFMT_VA,
  OPT_G_PLAYERQLEN,               // u
  OPT_G_REMCONTROLHTML,           //   d
  OPT_G_SHOWALBUM,
  OPT_G_SHOWBPM,
  OPT_G_SHOWCLASSICAL,
  OPT_G_VARIOUS,
  OPT_G_WRITETAGS,                //   d
  OPT_M_AUDIOSINK,                //   d
  OPT_M_DIR_MUSIC,                // u d
  OPT_M_PLAYER_INTFC,             // u d
  OPT_M_SHUTDOWNSCRIPT,           //   d
  OPT_M_STARTUPSCRIPT,            //   d
  OPT_M_VOLUME_INTFC,             // u d
  OPT_MP_LISTING_FONT,            // u d
  OPT_MP_MQFONT,                  // u d
  OPT_MP_MQ_THEME,                // u d
  OPT_MP_PLAYEROPTIONS,
  OPT_MP_PLAYERSHUTDOWNSCRIPT,
  OPT_MP_PLAYERSTARTSCRIPT,
  OPT_MP_UIFONT,                  // u d
  OPT_MP_UI_THEME,                //
  OPT_P_DEFAULTVOLUME,            // u d
  OPT_P_DONEMSG,
  OPT_P_FADEINTIME,               // u d
  OPT_P_FADEOUTTIME,              // u d
  OPT_P_FADETYPE,                 // u d
  OPT_P_GAP,                      // u d
  OPT_P_HIDE_MARQUEE_ON_START,    // u d
  OPT_P_INSERT_LOCATION,          // u d
  OPT_P_MAXPLAYTIME,              // u d
  OPT_P_MOBILEMARQUEE,            // u d
  OPT_P_MOBILEMQPORT,             // u d
  OPT_P_MOBILEMQTAG,              // u d
  OPT_P_MOBILEMQTITLE,            // u d
  OPT_P_MQ_ACCENT_COL,            //   d
  OPT_P_MQQLEN,                   // u d
  OPT_P_MQ_SHOW_INFO,             // u d
  OPT_P_PAUSEMSG,
  OPT_P_PROFILENAME,              // u d
  OPT_P_QUEUE_NAME_A,             // u d
  OPT_P_QUEUE_NAME_B,             // u d
  OPT_P_REMCONTROLPASS,           // u d
  OPT_P_REMCONTROLPORT,           // u d
  OPT_P_REMCONTROLUSER,           // u d
  OPT_P_REMOTECONTROL,            // u d
  OPT_P_UI_ACCENT_COL,            //   d
} bdjoptkey_t;

typedef enum {
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_MACHINE,
  OPTTYPE_MACH_PROF
} bdjopttype_t;

#define BDJ_CONFIG_BASEFN   "bdjconfig"
#define BDJ_CONFIG_EXT      ".txt"

extern datafilekey_t  bdjoptprofiledfkeys[];
extern int            bdjoptprofiledfcount;

void    bdjoptInit (void);
void    bdjoptFree (void);
void    *bdjoptGetStr (ssize_t idx);
ssize_t bdjoptGetNum (ssize_t idx);
void    bdjoptSetStr (ssize_t idx, const char *value);
void    bdjoptSetNum (ssize_t idx, ssize_t value);
void    bdjoptCreateDirectories (void);
void    bdjoptSave (void);

#endif /* INC_BDJOPT_H */
