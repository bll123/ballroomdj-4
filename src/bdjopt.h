#ifndef INC_BDJOPT_H
#define INC_BDJOPT_H

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
  OPT_G_PLAYER_INTFC,             //
  OPT_G_PLAYERQLEN,               //
  OPT_G_REMCONTROLHTML,
  OPT_G_SHOWALBUM,
  OPT_G_SHOWBPM,
  OPT_G_SHOWCLASSICAL,
  OPT_G_SHOWSTATUS,
  OPT_G_SLOWDEVICE,
  OPT_G_STARTMAXIMIZED,
  OPT_G_VARIOUS,
  OPT_G_VOLUME_INTFC,             //
  OPT_G_WRITETAGS,
  OPT_M_AUDIOSINK,                //
  OPT_M_DIR_ARCHIVE,
  OPT_M_DIR_DELETE,
  OPT_M_DIR_IMAGE,
  OPT_M_DIR_MUSIC,                //
  OPT_M_DIR_MUSICTMP,
  OPT_M_DIR_ORIGINAL,
  OPT_M_HOST,
  OPT_M_SHUTDOWNSCRIPT,
  OPT_M_STARTUPSCRIPT,
  OPT_MP_FONTSIZE,
  OPT_MP_LISTINGFONTSIZE,
  OPT_MP_PLAYEROPTIONS,
  OPT_MP_PLAYERSHUTDOWNSCRIPT,
  OPT_MP_PLAYERSTARTSCRIPT,
  OPT_MP_UIFIXEDFONT,
  OPT_MP_UIFONT,
  OPT_P_ALLOWEDIT,
  OPT_P_AUTOSTARTUP,
  OPT_P_DEFAULTVOLUME,              //
  OPT_P_DONEMSG,
  OPT_P_FADEINTIME,                 //
  OPT_P_FADEOUTTIME,                //
  OPT_P_FADETYPE,                   //
  OPT_P_GAP,                        //
  OPT_P_MAXPLAYTIME,                //
  OPT_P_MOBILEMARQUEE,              //
  OPT_P_MOBILEMQPORT,               //
  OPT_P_MOBILEMQTAG,
  OPT_P_MOBILEMQTITLE,
  OPT_P_MQFONT,
  OPT_P_MQFULLSCREEN,
  OPT_P_MQQLEN,
  OPT_P_PAUSEMSG,
  OPT_P_PROFILENAME,
  OPT_P_QUEUENAME0,
  OPT_P_QUEUENAME1,
  OPT_P_QUICKPLAYENABLED,
  OPT_P_QUICKPLAYSHOW,
  OPT_P_REMCONTROLPASS,
  OPT_P_REMCONTROLPORT,
  OPT_P_REMCONTROLSHOWDANCE,
  OPT_P_REMCONTROLSHOWSONG,
  OPT_P_REMCONTROLUSER,
  OPT_P_REMOTECONTROL,
  OPT_P_SERVERNAME,
  OPT_P_SERVERPASS,
  OPT_P_SERVERPORT,
  OPT_P_SERVERTYPE,
  OPT_P_SERVERUSER,
  OPT_P_UIACCENTCOLOR,
  OPT_P_UITHEME,
} bdjoptkey_t;

typedef enum {
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_MACHINE,
  OPTTYPE_MACH_PROF
} bdjopttype_t;

#define BDJ_CONFIG_BASEFN   "bdjconfig"
#define BDJ_CONFIG_EXT      ".txt"

void    bdjoptInit (void);
void    bdjoptFree (void);
void    *bdjoptGetData (ssize_t idx);
ssize_t bdjoptGetNum (ssize_t idx);
void    bdjoptSetNum (ssize_t idx, ssize_t value);

#endif /* INC_BDJOPT_H */
