#ifndef INC_BDJOPT_H
#define INC_BDJOPT_H

#include "datafile.h"

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

/* development notes
 * u - in use
 * d - display working in configui
 * s - save working in configui
 */

typedef enum {
  OPT_G_AO_PATHFMT,               // u d s
  OPT_G_AUTOORGANIZE,             //   d s
  OPT_G_BPM,                      // u d s
  OPT_G_DEBUGLVL,                 // u d s
  OPT_G_ITUNESSUPPORT,            //   d s
  OPT_G_LOADDANCEFROMGENRE,       // u d s
  OPT_G_PLAYERQLEN,               // u d s
  OPT_G_REMCONTROLHTML,           // u d s
  OPT_G_WRITETAGS,                //   d s
  OPT_M_AUDIOSINK,                // u d s
  OPT_M_DIR_MUSIC,                // u d s
  /* DIR_OLD_SKIP will be used for a time until the conversion from bdj3 to */
  /* bdj4 is complete.  It will be removed in a later version */
  OPT_M_DIR_OLD_SKIP,             // u - -
  OPT_M_PLAYER_INTFC,             // u d s
  OPT_M_SHUTDOWNSCRIPT,           // u d s
  OPT_M_STARTUPSCRIPT,            // u d s
  OPT_M_VOLUME_INTFC,             // u d s
  OPT_MP_LISTING_FONT,            // u d s
  OPT_MP_MQFONT,                  // u d s
  OPT_MP_MQ_THEME,                // u d s
  OPT_MP_PLAYEROPTIONS,
  OPT_MP_PLAYERSHUTDOWNSCRIPT,
  OPT_MP_PLAYERSTARTSCRIPT,
  OPT_MP_UIFONT,                  // u d s
  OPT_MP_UI_THEME,                // u d s
  OPT_P_DEFAULTVOLUME,            // u d s
  OPT_P_COMPLETE_MSG,             // u d s
  OPT_P_FADEINTIME,               // u d s
  OPT_P_FADEOUTTIME,              // u d s
  OPT_P_GAP,                      // u d s
  OPT_P_HIDE_MARQUEE_ON_START,    // u d s
  OPT_P_MAXPLAYTIME,              // u d s
  OPT_P_MOBILEMARQUEE,            // u d s
  OPT_P_MOBILEMQPORT,             // u d s
  OPT_P_MOBILEMQTAG,              // u d s
  OPT_P_MOBILEMQTITLE,            // u d s
  OPT_P_MQ_ACCENT_COL,            // u d s
  OPT_P_MQ_INFO_COL,              // u d s
  OPT_P_MQ_TEXT_COL,              // u d s
  OPT_P_MQQLEN,                   // u d s
  OPT_P_MQ_SHOW_INFO,             // u d s
  OPT_P_PROFILENAME,              // u d s
  /* the queue name identifiers must be in sequence */
  /* the number of queue names must match MUSICQ_PB_MAX */
  OPT_P_QUEUE_NAME_A,             // u d s
  OPT_P_QUEUE_NAME_B,             // u d s
  OPT_P_REMCONTROLPASS,           // u d s
  OPT_P_REMCONTROLPORT,           // u d s
  OPT_P_REMCONTROLUSER,           // u d s
  OPT_P_REMOTECONTROL,            // u d s
  OPT_P_UI_ACCENT_COL,            // u d s
  OPT_P_UI_PROFILE_COL,           // u d s
} bdjoptkey_t;

typedef enum {
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_MACHINE,
  OPTTYPE_MACH_PROF
} bdjopttype_t;

typedef struct {
  ssize_t       currprofile;
  datafile_t    *df;
  nlist_t       *bdjoptList;
  char          *globalFname;
  char          *profileFname;
  char          *machineFname;
  char          *machineProfileFname;
} bdjopt_t;

#define BDJ_CONFIG_BASEFN   "bdjconfig"
#define BDJ_CONFIG_EXT      ".txt"
#define BDJOPT_MAX_PROFILES 20

extern datafilekey_t  bdjoptprofiledfkeys[];
extern int            bdjoptprofiledfcount;

void    bdjoptInit (void);
void    bdjoptFree (void);
char    *bdjoptGetStr (ssize_t idx);
ssize_t bdjoptGetNum (ssize_t idx);
void    bdjoptSetStr (ssize_t idx, const char *value);
void    bdjoptSetNum (ssize_t idx, ssize_t value);
void    bdjoptCreateDirectories (void);
void    bdjoptSave (void);
void    bdjoptConvBPM (datafileconv_t *conv);
void    bdjoptDump (void);

#endif /* INC_BDJOPT_H */
