/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  MARQUEE_SHOW_OFF,
  MARQUEE_SHOW_MINIMIZE,
  MARQUEE_SHOW_VISIBLE,
} bdjmarqueeshow_t;

typedef enum {
  FADETYPE_EXPONENTIAL_SINE,
  FADETYPE_HALF_SINE,
  FADETYPE_INVERTED_PARABOLA,
  FADETYPE_QUADRATIC,
  FADETYPE_QUARTER_SINE,
  FADETYPE_TRIANGLE,
} bdjfadetype_t;

typedef enum {
  BPM_BPM,
  BPM_MPM,
} bdjbpm_t;

/* expected-count removed 4.3.2.4 */
enum {
  DANCESEL_METHOD_WINDOWED,
};

typedef enum {
  OPT_G_ACOUSTID_KEY,
  OPT_G_ACRCLOUD_API_KEY,
  OPT_G_ACRCLOUD_API_SECRET,
  OPT_G_ACRCLOUD_API_HOST,
  OPT_G_AUTOORGANIZE,
  OPT_G_BPM,
  OPT_G_CLOCK_DISP,
  OPT_G_DANCESEL_METHOD,
  OPT_G_DEBUGLVL,
  OPT_G_LOADDANCEFROMGENRE,
  OPT_G_OLDORGPATH,
  OPT_G_ORGPATH,
  OPT_G_PLAYERQLEN,
  OPT_G_REMCONTROLHTML,
  OPT_G_WRITETAGS,
  OPT_G_YOUTUBEAPIKEY,
  OPT_M_AUDIOTAG_INTFC,
  OPT_M_DIR_ITUNES_MEDIA,
  OPT_M_DIR_MUSIC,
  /* DIR_OLD_SKIP will be used for a time until the conversion from bdj3 to */
  /* bdj4 is complete.  It will be removed in a later version */
  OPT_M_DIR_OLD_SKIP,
  OPT_M_ITUNES_XML_FILE,
  OPT_M_PLAYER_INTFC,
  OPT_M_PLAYER_INTFC_NM,
  OPT_M_SCALE,
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
  OPT_P_COMPLETE_MSG,
  OPT_P_DEFAULTVOLUME,
  OPT_P_FADETYPE,
  OPT_P_MARQUEE_SHOW,
  OPT_P_MOBILEMARQUEE,
  OPT_P_MOBILEMQPORT,
  OPT_P_MOBILEMQTITLE,
  OPT_P_MQ_ACCENT_COL,
  OPT_P_MQ_INFO_COL,
  OPT_P_MQQLEN,
  OPT_P_MQ_SHOW_INFO,
  OPT_P_MQ_TEXT_COL,
  OPT_P_PROFILENAME,
  OPT_P_REMCONTROLPASS,
  OPT_P_REMCONTROLPORT,
  OPT_P_REMCONTROLUSER,
  OPT_P_REMOTECONTROL,
  OPT_P_UI_ACCENT_COL,
  OPT_P_UI_ERROR_COL,
  OPT_P_UI_MARK_COL,
  OPT_P_UI_PROFILE_COL,
  /* the queue values must be at the end of enum list, as they are */
  /* duplicated for each playback queue */
  /* opt_q_active must be the first opt_q_ item */
  OPT_Q_ACTIVE,
  OPT_Q_DISPLAY,
  OPT_Q_FADEINTIME,
  OPT_Q_FADEOUTTIME,
  OPT_Q_GAP,
  OPT_Q_MAXPLAYTIME,
  OPT_Q_PAUSE_EACH_SONG,
  OPT_Q_PLAY_ANNOUNCE,
  OPT_Q_PLAY_WHEN_QUEUED,
  OPT_Q_QUEUE_NAME,
  OPT_Q_SHOW_QUEUE_DANCE,
  OPT_Q_STOP_AT_TIME,
} bdjoptkey_t;

typedef enum {
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_QUEUE,
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
const char *bdjoptGetStr (nlistidx_t idx);
int64_t bdjoptGetNum (nlistidx_t idx);
void    bdjoptSetStr (nlistidx_t idx, const char *value);
void    bdjoptSetNum (nlistidx_t idx, int64_t value);
const char *bdjoptGetStrPerQueue (nlistidx_t idx, int musicq);
int64_t bdjoptGetNumPerQueue (nlistidx_t idx, int musicq);
void    bdjoptSetStrPerQueue (nlistidx_t idx, const char *value, int musicq);
void    bdjoptSetNumPerQueue (nlistidx_t idx, int64_t value, int musicq);
void    bdjoptCreateDirectories (void);
void    bdjoptDeleteProfile (void);
void    bdjoptSave (void);
void    bdjoptConvBPM (datafileconv_t *conv);
void    bdjoptConvClock (datafileconv_t *conv);
void    bdjoptConvFadeType (datafileconv_t *conv);
void    bdjoptConvWriteTags (datafileconv_t *conv);
void    bdjoptConvMarqueeShow (datafileconv_t *conv);
void    bdjoptConvDanceselMethod (datafileconv_t *conv);
void    bdjoptDump (void);
bool    bdjoptProfileExists (void);
char    * bdjoptGetProfileName (void);

#endif /* INC_BDJOPT_H */
