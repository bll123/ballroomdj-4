#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjopt.h"
#include "datafile.h"
#include "datautil.h"
#include "filemanip.h"
#include "fileop.h"
#include "nlist.h"
#include "portability.h"
#include "sysvars.h"

static void bdjoptConvFadeType (char *, datafileret_t *);
static void bdjoptCreateNewConfigs (void);
static void bdjoptCreateDefaultFiles (void);
static void bdjoptConvMobileMq (char *data, datafileret_t *ret);

static datafile_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys[] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_NUM, parseConvBoolean },
  { "CHANGESPACE",        OPT_G_CHANGESPACE,        VALUE_NUM, parseConvBoolean },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_NUM, NULL },
  { "ENABLEIMGPLAYER",    OPT_G_ENABLEIMGPLAYER,    VALUE_NUM, parseConvBoolean },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_DATA, NULL },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, parseConvBoolean },
  { "MUSICDIRDFLT",       OPT_G_MUSICDIRDFLT,       VALUE_DATA, NULL },
  { "PATHFMT",            OPT_G_PATHFMT,            VALUE_DATA, NULL },
  { "PATHFMT_CL",         OPT_G_PATHFMT_CL,         VALUE_DATA, NULL },
  { "PATHFMT_CLVA",       OPT_G_PATHFMT_CLVA,       VALUE_DATA, NULL },
  { "PATHFMT_VA",         OPT_G_PATHFMT_VA,         VALUE_DATA, NULL },
  { "PLAYER",             OPT_G_PLAYER_INTFC,       VALUE_DATA, NULL },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_NUM, NULL },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_DATA, NULL },
  { "SHOWALBUM",          OPT_G_SHOWALBUM,          VALUE_NUM, parseConvBoolean },
  { "SHOWBPM",            OPT_G_SHOWBPM,            VALUE_NUM, parseConvBoolean },
  { "SHOWCLASSICAL",      OPT_G_SHOWCLASSICAL,      VALUE_NUM, parseConvBoolean },
  { "SHOWSTATUS",         OPT_G_SHOWSTATUS,         VALUE_NUM, parseConvBoolean },
  { "SLOWDEVICE",         OPT_G_SLOWDEVICE,         VALUE_NUM, parseConvBoolean },
  { "STARTMAXIMIZED",     OPT_G_STARTMAXIMIZED,     VALUE_NUM, parseConvBoolean },
  { "VARIOUS",            OPT_G_VARIOUS,            VALUE_DATA, NULL },
  { "VOLUME",             OPT_G_VOLUME_INTFC,       VALUE_DATA, NULL },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_DATA, NULL },
};
#define BDJOPT_GLOBAL_DFKEY_COUNT (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptprofiledfkeys[] = {
  { "ALLOWEDIT",            OPT_P_ALLOWEDIT,            VALUE_NUM, parseConvBoolean },
  { "AUTOSTARTUP",          OPT_P_AUTOSTARTUP,          VALUE_DATA, NULL },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_NUM, NULL },
  { "DONEMSG",              OPT_P_DONEMSG,              VALUE_DATA, NULL },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_NUM, NULL },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_NUM, NULL },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_DATA, bdjoptConvFadeType },
  { "GAP",                  OPT_P_GAP,                  VALUE_NUM, NULL },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_NUM, NULL },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_NUM, bdjoptConvMobileMq },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_NUM, NULL },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_DATA, NULL },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_DATA, NULL },
  { "MQFONT",               OPT_P_MQFONT,               VALUE_DATA, NULL },
  { "MQFULLSCREEN",         OPT_P_MQFULLSCREEN,         VALUE_DATA, NULL },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_DATA, NULL },
  { "PAUSEMSG",             OPT_P_PAUSEMSG,             VALUE_DATA, NULL },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_DATA, NULL },
  { "QUEUENAME0",           OPT_P_QUEUENAME0,           VALUE_DATA, NULL },
  { "QUEUENAME1",           OPT_P_QUEUENAME1,           VALUE_DATA, NULL },
  { "QUICKPLAYENABLED",     OPT_P_QUICKPLAYENABLED,     VALUE_NUM, parseConvBoolean },
  { "QUICKPLAYSHOW",        OPT_P_QUICKPLAYSHOW,        VALUE_NUM, parseConvBoolean },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_DATA, NULL },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_NUM, NULL },
  { "REMCONTROLSHOWDANCE",  OPT_P_REMCONTROLSHOWDANCE,  VALUE_NUM, parseConvBoolean },
  { "REMCONTROLSHOWSONG",   OPT_P_REMCONTROLSHOWSONG,   VALUE_NUM, parseConvBoolean },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_DATA, NULL },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_NUM, parseConvBoolean },
  { "SERVERNAME",           OPT_P_SERVERNAME,           VALUE_DATA, NULL },
  { "SERVERPASS",           OPT_P_SERVERPASS,           VALUE_DATA, NULL },
  { "SERVERPORT",           OPT_P_SERVERPORT,           VALUE_NUM, NULL },
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
  { "SHUTDOWNSCRIPT", OPT_M_SHUTDOWNSCRIPT, VALUE_DATA, NULL },
  { "STARTUPSCRIPT",  OPT_M_STARTUPSCRIPT,  VALUE_DATA, NULL },
};
#define BDJOPT_MACHINE_DFKEY_COUNT (sizeof (bdjoptmachdfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachprofiledfkeys[] = {
  { "FONTSIZE",             OPT_MP_FONTSIZE,              VALUE_NUM, NULL },
  { "LISTINGFONTSIZE",      OPT_MP_LISTINGFONTSIZE,       VALUE_NUM, NULL },
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
  nlist_t       *tlist;

  /* global */
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  if (! fileopExists (path)) {
    bdjoptCreateNewConfigs ();
  }
  df = datafileAllocParse ("bdjopt-g", DFTYPE_KEY_VAL, path,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, DATAFILE_NO_LOOKUP);

  /* profile */
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  datafileParseMerge (tlist, ddata, "bdjopt-p", DFTYPE_KEY_VAL,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine */
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine per profile */
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  bdjopt = df;
}

void
bdjoptFree (void)
{
  if (bdjopt != NULL) {
    datafileFree (bdjopt);
  }
  bdjopt = NULL;
}


void *
bdjoptGetData (ssize_t idx)
{
  void      *value = NULL;

  if (bdjopt == NULL) {
    return NULL;
  }
  value = nlistGetStr (bdjopt->data, idx);
  return value;
}

ssize_t
bdjoptGetNum (ssize_t idx)
{
  ssize_t       value;

  if (bdjopt == NULL) {
    return -1;
  }
  value = nlistGetNum (bdjopt->data, idx);
  return value;
}

void
bdjoptSetNum (ssize_t idx, ssize_t value)
{
  if (bdjopt == NULL) {
    return;
  }
  nlistSetNum (bdjopt->data, idx, value);
}

static void
bdjoptConvFadeType (char *data, datafileret_t *ret)
{
  bdjfadetype_t   fadetype = FADETYPE_TRIANGLE;

  ret->valuetype = VALUE_NUM;

  if (strcmp (data, "quartersine") == 0) {
    fadetype = FADETYPE_QUARTER_SINE;
  }
  if (strcmp (data, "halfsine") == 0) {
    fadetype = FADETYPE_HALF_SINE;;
  }
  if (strcmp (data, "logarithmic") == 0) {
    fadetype = FADETYPE_LOGARITHMIC;
  }
  if (strcmp (data, "invertedparabola") == 0) {
    fadetype = FADETYPE_INVERTED_PARABOLA;
  }
  ret->u.num = fadetype;
}

static void
bdjoptCreateNewConfigs (void)
{
  ssize_t   currProfile = lsysvars [SVL_BDJIDX];
  char      path [MAXPATHLEN];
  char      tpath [MAXPATHLEN];

    /* see if profile 0 exists */
  sysvarSetNum (SVL_BDJIDX, 0);
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  if (! fileopExists (path)) {
    bdjoptCreateDefaultFiles ();
  }

    /* global */
  sysvarSetNum (SVL_BDJIDX, currProfile);
  datautilMakePath (tpath, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* profile */
  sysvarSetNum (SVL_BDJIDX, 0);
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  datautilMakePath (tpath, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine */
  sysvarSetNum (SVL_BDJIDX, 0);
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  datautilMakePath (tpath, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine per profile */
  sysvarSetNum (SVL_BDJIDX, 0);
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  datautilMakePath (tpath, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  filemanipCopy (path, tpath);

  sysvarSetNum (SVL_BDJIDX, currProfile);
}

static void
bdjoptCreateDefaultFiles (void)
{
  char      path [MAXPATHLEN];

  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_USEIDX);
  datautilMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
  datautilMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, DATAUTIL_MP_HOSTNAME | DATAUTIL_MP_USEIDX);
}

static void
bdjoptConvMobileMq (char *data, datafileret_t *ret)
{
  bdjmobilemq_t   val = MOBILEMQ_OFF;

  ret->valuetype = VALUE_NUM;

  if (strcmp (data, "internet") == 0) {
    val = MOBILEMQ_INTERNET;
  }
  if (strcmp (data, "local") == 0) {
    val = MOBILEMQ_INTERNET;
  }
  ret->u.num = val;
}

