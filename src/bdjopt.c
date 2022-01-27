#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjopt.h"
#include "datafile.h"
#include "pathbld.h"
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
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_NUM, parseConvBoolean, -1 },
  { "CHANGESPACE",        OPT_G_CHANGESPACE,        VALUE_NUM, parseConvBoolean, -1 },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_NUM, NULL, -1 },
  { "ENABLEIMGPLAYER",    OPT_G_ENABLEIMGPLAYER,    VALUE_NUM, parseConvBoolean, -1 },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_DATA, NULL, -1 },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, parseConvBoolean, -1 },
  { "MUSICDIRDFLT",       OPT_G_MUSICDIRDFLT,       VALUE_DATA, NULL, -1 },
  { "PATHFMT",            OPT_G_PATHFMT,            VALUE_DATA, NULL, -1 },
  { "PATHFMT_CL",         OPT_G_PATHFMT_CL,         VALUE_DATA, NULL, -1 },
  { "PATHFMT_CLVA",       OPT_G_PATHFMT_CLVA,       VALUE_DATA, NULL, -1 },
  { "PATHFMT_VA",         OPT_G_PATHFMT_VA,         VALUE_DATA, NULL, -1 },
  { "PLAYER",             OPT_G_PLAYER_INTFC,       VALUE_DATA, NULL, -1 },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_NUM, NULL, -1 },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_DATA, NULL, -1 },
  { "SHOWALBUM",          OPT_G_SHOWALBUM,          VALUE_NUM, parseConvBoolean, -1 },
  { "SHOWBPM",            OPT_G_SHOWBPM,            VALUE_NUM, parseConvBoolean, -1 },
  { "SHOWCLASSICAL",      OPT_G_SHOWCLASSICAL,      VALUE_NUM, parseConvBoolean, -1 },
  { "SHOWSTATUS",         OPT_G_SHOWSTATUS,         VALUE_NUM, parseConvBoolean, -1 },
  { "SLOWDEVICE",         OPT_G_SLOWDEVICE,         VALUE_NUM, parseConvBoolean, -1 },
  { "STARTMAXIMIZED",     OPT_G_STARTMAXIMIZED,     VALUE_NUM, parseConvBoolean, -1 },
  { "VARIOUS",            OPT_G_VARIOUS,            VALUE_DATA, NULL, -1 },
  { "VOLUME",             OPT_G_VOLUME_INTFC,       VALUE_DATA, NULL, -1 },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_DATA, NULL, -1 },
};
#define BDJOPT_GLOBAL_DFKEY_COUNT (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptprofiledfkeys[] = {
  { "ALLOWEDIT",            OPT_P_ALLOWEDIT,            VALUE_NUM, parseConvBoolean, -1 },
  { "AUTOSTARTUP",          OPT_P_AUTOSTARTUP,          VALUE_DATA, NULL, -1 },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_NUM, NULL, -1 },
  { "DONEMSG",              OPT_P_DONEMSG,              VALUE_DATA, NULL, -1 },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_NUM, NULL, -1 },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_NUM, NULL, -1 },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_DATA, bdjoptConvFadeType, -1 },
  { "GAP",                  OPT_P_GAP,                  VALUE_NUM, NULL, -1 },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_NUM, NULL, -1 },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_NUM, bdjoptConvMobileMq, -1 },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_NUM, NULL, -1 },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_DATA, NULL, -1 },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_DATA, NULL, -1 },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_NUM, NULL, -1 },
  { "MQSHOWINFO",           OPT_P_MQ_SHOW_INFO,         VALUE_NUM, parseConvBoolean, -1 },
  { "PAUSEMSG",             OPT_P_PAUSEMSG,             VALUE_DATA, NULL, -1 },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_DATA, NULL, -1 },
  { "QUEUENAME0",           OPT_P_QUEUENAME0,           VALUE_DATA, NULL, -1 },
  { "QUEUENAME1",           OPT_P_QUEUENAME1,           VALUE_DATA, NULL, -1 },
  { "QUICKPLAYENABLED",     OPT_P_QUICKPLAYENABLED,     VALUE_NUM, parseConvBoolean, -1 },
  { "QUICKPLAYSHOW",        OPT_P_QUICKPLAYSHOW,        VALUE_NUM, parseConvBoolean, -1 },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_DATA, NULL, -1 },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_NUM, NULL, -1 },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_DATA, NULL, -1 },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_NUM, parseConvBoolean, -1 },
  { "SERVERNAME",           OPT_P_SERVERNAME,           VALUE_DATA, NULL, -1 },
  { "SERVERPASS",           OPT_P_SERVERPASS,           VALUE_DATA, NULL, -1 },
  { "SERVERPORT",           OPT_P_SERVERPORT,           VALUE_NUM, NULL, -1 },
  { "SERVERTYPE",           OPT_P_SERVERTYPE,           VALUE_DATA, NULL, -1 },
  { "SERVERUSER",           OPT_P_SERVERUSER,           VALUE_DATA, NULL, -1 },
  { "UIACCENTCOLOR",        OPT_P_UIACCENTCOLOR,        VALUE_DATA, NULL, -1 },
  { "UITHEME",              OPT_P_UITHEME,              VALUE_DATA, NULL, -1 },
};
#define BDJOPT_PROFILE_DFKEY_COUNT (sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachdfkeys[] = {
  { "AUDIOSINK",      OPT_M_AUDIOSINK,      VALUE_DATA, NULL, -1 },
  { "DIRARCHIVE",     OPT_M_DIR_ARCHIVE,    VALUE_DATA, NULL, -1 },
  { "DIRDELETE",      OPT_M_DIR_DELETE,     VALUE_DATA, NULL, -1 },
  { "DIRIMAGE",       OPT_M_DIR_IMAGE,      VALUE_DATA, NULL, -1 },
  { "DIRMUSIC",       OPT_M_DIR_MUSIC,      VALUE_DATA, NULL, -1 },
  { "DIRMUSICTMP",    OPT_M_DIR_MUSICTMP,   VALUE_DATA, NULL, -1 },
  { "DIRORIGINAL",    OPT_M_DIR_ORIGINAL,   VALUE_DATA, NULL, -1 },
  { "HOST",           OPT_M_HOST,           VALUE_DATA, NULL, -1 },
  { "SHUTDOWNSCRIPT", OPT_M_SHUTDOWNSCRIPT, VALUE_DATA, NULL, -1 },
  { "STARTUPSCRIPT",  OPT_M_STARTUPSCRIPT,  VALUE_DATA, NULL, -1 },
};
#define BDJOPT_MACHINE_DFKEY_COUNT (sizeof (bdjoptmachdfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachprofiledfkeys[] = {
  { "FONTSIZE",             OPT_MP_FONTSIZE,              VALUE_NUM, NULL, -1 },
  { "LISTINGFONTSIZE",      OPT_MP_LISTINGFONTSIZE,       VALUE_NUM, NULL, -1 },
  { "MQFONT",               OPT_MP_MQFONT,              VALUE_DATA, NULL, -1 },
  { "PLAYEROPTIONS",        OPT_MP_PLAYEROPTIONS,         VALUE_DATA, NULL, -1 },
  { "PLAYERSHUTDOWNSCRIPT", OPT_MP_PLAYERSHUTDOWNSCRIPT,  VALUE_DATA, NULL, -1 },
  { "PLAYERSTARTSCRIPT",    OPT_MP_PLAYERSTARTSCRIPT,     VALUE_DATA, NULL, -1 },
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
  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  if (! fileopExists (path)) {
    bdjoptCreateNewConfigs ();
  }
  df = datafileAllocParse ("bdjopt-g", DFTYPE_KEY_VAL, path,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, DATAFILE_NO_LOOKUP);

  /* profile */
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  datafileParseMerge (tlist, ddata, "bdjopt-p", DFTYPE_KEY_VAL,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine */
  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine per profile */
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
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
  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  if (! fileopExists (path)) {
    bdjoptCreateDefaultFiles ();
  }

    /* global */
  sysvarSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* profile */
  sysvarSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine */
  sysvarSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine per profile */
  sysvarSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

  sysvarSetNum (SVL_BDJIDX, currProfile);
}

static void
bdjoptCreateDefaultFiles (void)
{
  char      path [MAXPATHLEN];

  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  pathbldMakePath (path, MAXPATHLEN, "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  pathbldMakePath (path, MAXPATHLEN, "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
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
    val = MOBILEMQ_LOCAL;
  }
  ret->u.num = val;
}

