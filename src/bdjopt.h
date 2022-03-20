#ifndef INC_BDJOPT_H
#define INC_BDJOPT_H

#include "datafile.h"

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
  OPT_G_AUTOORGANIZE,
  OPT_G_CHANGESPACE,
  OPT_G_DEBUGLVL,                 //
  OPT_G_ENABLEIMGPLAYER,
  OPT_G_ITUNESSUPPORT,
  OPT_G_LOADDANCEFROMGENRE,
  OPT_G_MUSICDIRDFLT,
  OPT_G_PATHFMT,
  OPT_G_PATHFMT_CL,
  OPT_G_PATHFMT_CLVA,
  OPT_G_PATHFMT_VA,
  OPT_G_PLAYERQLEN,               //
  OPT_G_REMCONTROLHTML,
  OPT_G_SHOWALBUM,
  OPT_G_SHOWBPM,
  OPT_G_SHOWCLASSICAL,
  OPT_G_SHOWSTATUS,
  OPT_G_SLOWDEVICE,
  OPT_G_VARIOUS,
  OPT_G_WRITETAGS,
  OPT_M_AUDIOSINK,                //
  OPT_M_DIR_MUSIC,                //
  OPT_M_HOST,
  OPT_M_PLAYER_INTFC,             //
  OPT_M_SHUTDOWNSCRIPT,
  OPT_M_STARTUPSCRIPT,
  OPT_M_VOLUME_INTFC,             //
  OPT_MP_LISTING_FONT,            //
  OPT_MP_MQFONT,                    //
  OPT_MP_MQ_THEME,                  //
  OPT_MP_PLAYEROPTIONS,
  OPT_MP_PLAYERSHUTDOWNSCRIPT,
  OPT_MP_PLAYERSTARTSCRIPT,
  OPT_MP_UIFONT,                    //
  OPT_MP_UI_THEME,                  //
  OPT_P_ALLOWEDIT,
  OPT_P_AUTOSTARTUP,
  OPT_P_DEFAULTVOLUME,              //
  OPT_P_DONEMSG,
  OPT_P_FADEINTIME,                 //
  OPT_P_FADEOUTTIME,                //
  OPT_P_FADETYPE,                   //
  OPT_P_GAP,                        //
  OPT_P_HIDE_MARQUEE_ON_START,
  OPT_P_INSERT_LOCATION,            //
  OPT_P_MAXPLAYTIME,                //
  OPT_P_MOBILEMARQUEE,              //
  OPT_P_MOBILEMQPORT,               //
  OPT_P_MOBILEMQTAG,                //
  OPT_P_MOBILEMQTITLE,              //
  OPT_P_MQ_ACCENT_COL,              //
  OPT_P_MQQLEN,                     //
  OPT_P_MQ_SHOW_INFO,               //
  OPT_P_PAUSEMSG,
  OPT_P_PROFILENAME,                //
  OPT_P_QUEUE_NAME_A,               //
  OPT_P_QUEUE_NAME_B,               //
  OPT_P_REMCONTROLPASS,             //
  OPT_P_REMCONTROLPORT,             //
  OPT_P_REMCONTROLUSER,             //
  OPT_P_REMOTECONTROL,              //
  OPT_P_UI_ACCENT_COL,
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
void    bdjoptSetNum (ssize_t idx, ssize_t value);
void    bdjoptCreateDirectories (void);

#endif /* INC_BDJOPT_H */
