#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdjopt.h"
#include "datafile.h"
#include "datautil.h"
#include "portability.h"

static datafile_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys[] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_DATA, NULL },
  { "CHANGESPACE",        OPT_G_CHANGESPACE,        VALUE_DATA, NULL },
  { "CLPATHFMT",          OPT_G_CLPATHFMT,          VALUE_DATA, NULL },
  { "CLVAPATHFMT",        OPT_G_CLVAPATHFMT,        VALUE_DATA, NULL },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_DATA, NULL },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_DATA, NULL },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_DATA, NULL },
  { "MUSICDIRDFLT",       OPT_G_MUSICDIRDFLT,       VALUE_DATA, NULL },
  { "PATHFMT",            OPT_G_PATHFMT,            VALUE_DATA, NULL },
  { "PLAYER",             OPT_G_PLAYER,             VALUE_DATA, NULL },
  { "PLAYERQLEN0",        OPT_G_PLAYERQLEN0,        VALUE_DATA, NULL },
  { "PLAYERQLEN1",        OPT_G_PLAYERQLEN1,        VALUE_DATA, NULL },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_DATA, NULL },
  { "SHOWALBUM",          OPT_G_SHOWALBUM,          VALUE_DATA, NULL },
  { "SHOWBPM",            OPT_G_SHOWBPM,            VALUE_DATA, NULL },
  { "SHOWCLASSICAL",      OPT_G_SHOWCLASSICAL,      VALUE_DATA, NULL },
  { "SHOWSTATUS",         OPT_G_SHOWSTATUS,         VALUE_DATA, NULL },
  { "SHUTDOWNSCRIPT",     OPT_G_SHUTDOWNSCRIPT,     VALUE_DATA, NULL },
  { "SLOWDEVICE",         OPT_G_SLOWDEVICE,         VALUE_DATA, NULL },
  { "STARTMAXIMIZED",     OPT_G_STARTMAXIMIZED,     VALUE_DATA, NULL },
  { "STARTUPSCRIPT",      OPT_G_STARTUPSCRIPT,      VALUE_DATA, NULL },
  { "VAPATHFMT",          OPT_G_VAPATHFMT,          VALUE_DATA, NULL },
  { "VARIOUS",            OPT_G_VARIOUS,            VALUE_DATA, NULL },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_DATA, NULL },
};
#define BDJOPT_GLOBAL_DFKEY_COUNT (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptprofiledfkeys[] = {
  { "ALLOWEDIT",            OPT_P_ALLOWEDIT,            VALUE_DATA, NULL },
  { "AUTOSTARTUP",          OPT_P_AUTOSTARTUP,          VALUE_DATA, NULL },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_DATA, NULL },
  { "DONEMSG",              OPT_P_DONEMSG,              VALUE_DATA, NULL },
  { "ENABLEIMGPLAYER",      OPT_P_ENABLEIMGPLAYER,      VALUE_DATA, NULL },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_DATA, NULL },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_DATA, NULL },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_DATA, NULL },
  { "GAP",                  OPT_P_GAP,                  VALUE_DATA, NULL },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_DATA, NULL },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_DATA, NULL },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_DATA, NULL },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_DATA, NULL },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_DATA, NULL },
  { "MQFONT",               OPT_P_MQFONT,               VALUE_DATA, NULL },
  { "MQFULLSCREEN",         OPT_P_MQFULLSCREEN,         VALUE_DATA, NULL },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_DATA, NULL },
  { "PAUSEMSG",             OPT_P_PAUSEMSG,             VALUE_DATA, NULL },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_DATA, NULL },
  { "QUEUENAME0",           OPT_P_QUEUENAME0,           VALUE_DATA, NULL },
  { "QUEUENAME1",           OPT_P_QUEUENAME1,           VALUE_DATA, NULL },
  { "QUICKPLAYENABLED",     OPT_P_QUICKPLAYENABLED,     VALUE_DATA, NULL },
  { "QUICKPLAYSHOW",        OPT_P_QUICKPLAYSHOW,        VALUE_DATA, NULL },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_DATA, NULL },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_DATA, NULL },
  { "REMCONTROLSHOWDANCE",  OPT_P_REMCONTROLSHOWDANCE,  VALUE_DATA, NULL },
  { "REMCONTROLSHOWSONG",   OPT_P_REMCONTROLSHOWSONG,   VALUE_DATA, NULL },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_DATA, NULL },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_DATA, NULL },
  { "SERVERNAME",           OPT_P_SERVERNAME,           VALUE_DATA, NULL },
  { "SERVERPASS",           OPT_P_SERVERPASS,           VALUE_DATA, NULL },
  { "SERVERPORT",           OPT_P_SERVERPORT,           VALUE_DATA, NULL },
  { "SERVERTYPE",           OPT_P_SERVERTYPE,           VALUE_DATA, NULL },
  { "SERVERUSER",           OPT_P_SERVERUSER,           VALUE_DATA, NULL },
  { "UIACCENTCOLOR",        OPT_P_UIACCENTCOLOR,        VALUE_DATA, NULL },
  { "UITHEME",              OPT_P_UITHEME,              VALUE_DATA, NULL },
};
#define BDJOPT_PROFILE_DFKEY_COUNT (sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachdfkeys[] = {
  { "AUDIOSINK",      OPT_M_AUDIOSINK,      VALUE_DATA, NULL },
  { "DIRARCHIVE",     OPT_M_DIR_ARCHIVE,    VALUE_DATA, NULL },
  { "DIRDELETE",      OPT_M_DIR_DELETE,     VALUE_DATA, NULL },
  { "DIRIMAGE",       OPT_M_DIR_IMAGE,      VALUE_DATA, NULL },
  { "DIRMUSIC",       OPT_M_DIR_MUSIC,      VALUE_DATA, NULL },
  { "DIRMUSICTMP",    OPT_M_DIR_MUSICTMP,   VALUE_DATA, NULL },
  { "DIRORIGINAL",    OPT_M_DIR_ORIGINAL,   VALUE_DATA, NULL },
  { "HOST",           OPT_M_HOST,           VALUE_DATA, NULL },
};
#define BDJOPT_MACHINE_DFKEY_COUNT (sizeof (bdjoptmachdfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachprofiledfkeys[] = {
  { "FONTSIZE",             OPT_MP_FONTSIZE,              VALUE_DATA, NULL },
  { "LISTINGFONTSIZE",      OPT_MP_LISTINGFONTSIZE,       VALUE_DATA, NULL },
  { "PLAYEROPTIONS",        OPT_MP_PLAYEROPTIONS,         VALUE_DATA, NULL },
  { "PLAYERSHUTDOWNSCRIPT", OPT_MP_PLAYERSHUTDOWNSCRIPT,  VALUE_DATA, NULL },
  { "PLAYERSTARTSCRIPT",    OPT_MP_PLAYERSTARTSCRIPT,     VALUE_DATA, NULL },
  { "UIFIXEDFONT",          OPT_MP_UIFIXEDFONT,           VALUE_DATA, NULL },
  { "UIFONT",               OPT_MP_UIFONT,                VALUE_DATA, NULL },
};
#define BDJOPT_MACH_PROFILE_DFKEY_COUNT (sizeof (bdjoptmachprofiledfkeys) / sizeof (datafilekey_t))

void
bdjoptInit (void)
{
  datafile_t    *df;
  char          path [MAXPATHLEN];
  char          *ddata;
  list_t        *tlist;

  /* global */
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  df = datafileAllocParse ("bdjopt-g", DFTYPE_KEY_VAL, path,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT);

  /* profile */
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetData (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-p", DFTYPE_KEY_VAL,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine */
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetData (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine per profile */
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetData (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT);
  datafileSetData (df, tlist);
  free (ddata);

  bdjopt = df;
}

void
bdjoptFree (void)
{
  datafileFree (bdjopt);
}
