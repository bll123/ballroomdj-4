#ifndef INC_BDJOPT_H
#define INC_BDJOPT_H

#include <stdint.h>

#include "datafile.h"
#include "nlist.h"

typedef enum {
  WRITE_TAGS_NONE,
  WRITE_TAGS_BDJ_ONLY,
  WRITE_TAGS_ALL,
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

typedef enum {
  BPM_BPM,
  BPM_MPM,
} bdjbpm_t;

typedef enum {
  OPT_G_AO_PATHFMT,
  OPT_G_AUTOORGANIZE,
  OPT_G_BPM,
  OPT_G_DEBUGLVL,
  OPT_G_LOADDANCEFROMGENRE,
  OPT_G_PLAYERQLEN,
  OPT_G_REMCONTROLHTML,
  OPT_G_WRITETAGS,
  OPT_G_BDJ3_COMPAT_TAGS,
  OPT_M_DIR_ITUNES_MEDIA,
  OPT_M_DIR_MUSIC,
  /* DIR_OLD_SKIP will be used for a time until the conversion from bdj3 to */
  /* bdj4 is complete.  It will be removed in a later version */
  OPT_M_DIR_OLD_SKIP,
  OPT_M_ITUNES_XML_FILE,
  OPT_M_PLAYER_INTFC,
  OPT_M_SHUTDOWNSCRIPT,
  OPT_M_STARTUPSCRIPT,
  OPT_M_VOLUME_INTFC,
  OPT_MP_AUDIOSINK,
  OPT_MP_LISTING_FONT,
  OPT_MP_MQFONT,
  OPT_MP_MQ_THEME,
  OPT_MP_PLAYEROPTIONS,
  OPT_MP_PLAYERSHUTDOWNSCRIPT,
  OPT_MP_PLAYERSTARTSCRIPT,
  OPT_MP_UIFONT,
  OPT_MP_UI_THEME,
  OPT_P_DEFAULTVOLUME,
  OPT_P_COMPLETE_MSG,
  OPT_P_FADEINTIME,
  OPT_P_FADEOUTTIME,
  OPT_P_FADETYPE,
  OPT_P_GAP,
  OPT_P_HIDE_MARQUEE_ON_START,
  OPT_P_MAXPLAYTIME,
  OPT_P_MOBILEMARQUEE,
  OPT_P_MOBILEMQPORT,
  OPT_P_MOBILEMQTAG,
  OPT_P_MOBILEMQTITLE,
  OPT_P_MQ_ACCENT_COL,
  OPT_P_MQ_INFO_COL,
  OPT_P_MQ_TEXT_COL,
  OPT_P_MQQLEN,
  OPT_P_MQ_SHOW_INFO,
  OPT_P_PLAY_ANNOUNCE,
  OPT_P_PROFILENAME,
  /* the queue name identifiers must be in sequence */
  /* the number of queue names must match MUSICQ_PB_MAX */
  OPT_P_QUEUE_NAME_A,
  OPT_P_QUEUE_NAME_B,
  OPT_P_REMCONTROLPASS,
  OPT_P_REMCONTROLPORT,
  OPT_P_REMCONTROLUSER,
  OPT_P_REMOTECONTROL,
  OPT_P_STOPATTIME,
  OPT_P_UI_ACCENT_COL,
  OPT_P_UI_ERROR_COL,
  OPT_P_UI_MARK_COL,
  OPT_P_UI_PROFILE_COL,
} bdjoptkey_t;

typedef enum {
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_MACHINE,
  OPTTYPE_MACH_PROF,
  OPTTYPE_MAX,
} bdjopttype_t;

#define BDJ_CONFIG_BASEFN   "bdjconfig"

enum {
  BDJOPT_MAX_PROFILES = 10,
};

void    bdjoptInit (void);
void    bdjoptCleanup (void);
char    *bdjoptGetStr (nlistidx_t idx);
int64_t bdjoptGetNum (nlistidx_t idx);
void    bdjoptSetStr (nlistidx_t idx, const char *value);
void    bdjoptSetNum (nlistidx_t idx, int64_t value);
void    bdjoptCreateDirectories (void);
void    bdjoptSave (void);
void    bdjoptConvBPM (datafileconv_t *conv);
void    bdjoptDump (void);
bool    bdjoptProfileExists (void);
char    * bdjoptGetProfileName (void);

#endif /* INC_BDJOPT_H */
