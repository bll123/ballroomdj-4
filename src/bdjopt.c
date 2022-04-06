#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "datafile.h"
#include "pathbld.h"
#include "filemanip.h"
#include "fileop.h"
#include "nlist.h"
#include "sysvars.h"

static void bdjoptConvFadeType (datafileconv_t *conv);
static void bdjoptConvWriteTags (datafileconv_t *conv);
static void bdjoptCreateNewConfigs (void);
static void bdjoptCreateDefaultFiles (void);
static void bdjoptConvMobileMq (datafileconv_t *conv);

static datafile_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys[] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_NUM, convBoolean, -1 },
  { "BPM",                OPT_G_BPM,                VALUE_NUM, bdjoptConvBPM, -1 },
  { "CHANGESPACE",        OPT_G_AO_CHANGESPACE,     VALUE_NUM, convBoolean, -1 },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_NUM, NULL, -1 },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_NUM, convBoolean, -1 },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, convBoolean, -1 },
  { "MUSICDIRDFLT",       OPT_G_MUSICDIRDFLT,       VALUE_STR, NULL, -1 },
  { "PATHFMT",            OPT_G_AO_PATHFMT,         VALUE_STR, NULL, -1 },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_NUM, NULL, -1 },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_STR, NULL, -1 },
  { "SHOWALBUM",          OPT_G_SHOWALBUM,          VALUE_NUM, convBoolean, -1 },
  { "SHOWCLASSICAL",      OPT_G_SHOWCLASSICAL,      VALUE_NUM, convBoolean, -1 },
  { "VARIOUS",            OPT_G_VARIOUS,            VALUE_STR, NULL, -1 },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_NUM, bdjoptConvWriteTags, -1 },
};
#define BDJOPT_GLOBAL_DFKEY_COUNT (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))

datafilekey_t bdjoptprofiledfkeys[] = {
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_NUM, NULL, -1 },
  { "DONEMSG",              OPT_P_DONEMSG,              VALUE_STR, NULL, -1 },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_NUM, NULL, -1 },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_NUM, NULL, -1 },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_NUM, bdjoptConvFadeType, -1 },
  { "GAP",                  OPT_P_GAP,                  VALUE_NUM, NULL, -1 },
  { "HIDEMARQUEEONSTART",   OPT_P_HIDE_MARQUEE_ON_START,VALUE_NUM, convBoolean, -1 },
  { "INSERT_LOC",           OPT_P_INSERT_LOCATION,      VALUE_NUM, NULL, -1 },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_NUM, NULL, -1 },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_NUM, bdjoptConvMobileMq, -1 },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_NUM, NULL, -1 },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_STR, NULL, -1 },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_STR, NULL, -1 },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_NUM, NULL, -1 },
  { "MQSHOWINFO",           OPT_P_MQ_SHOW_INFO,         VALUE_NUM, convBoolean, -1 },
  { "MQ_ACCENT_COL",        OPT_P_MQ_ACCENT_COL,        VALUE_STR, NULL, -1 },
  { "PAUSEMSG",             OPT_P_PAUSEMSG,             VALUE_STR, NULL, -1 },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_STR, NULL, -1 },
  { "QUEUE_NAME_A",         OPT_P_QUEUE_NAME_A,         VALUE_STR, NULL, -1 },
  { "QUEUE_NAME_B",         OPT_P_QUEUE_NAME_B,         VALUE_STR, NULL, -1 },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_STR, NULL, -1 },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_NUM, NULL, -1 },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_STR, NULL, -1 },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_NUM, convBoolean, -1 },
  { "UI_ACCENT_COL",        OPT_P_UI_ACCENT_COL,        VALUE_STR, NULL, -1 },
};
int            bdjoptprofiledfcount;
#define BDJOPT_PROFILE_DFKEY_COUNT (sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptmachdfkeys[] = {
  { "AUDIOSINK",      OPT_M_AUDIOSINK,      VALUE_STR, NULL, -1 },
  { "DIRMUSIC",       OPT_M_DIR_MUSIC,      VALUE_STR, NULL, -1 },
  { "PLAYER",         OPT_M_PLAYER_INTFC,   VALUE_STR, NULL, -1 },
  { "SHUTDOWNSCRIPT", OPT_M_SHUTDOWNSCRIPT, VALUE_STR, NULL, -1 },
  { "STARTUPSCRIPT",  OPT_M_STARTUPSCRIPT,  VALUE_STR, NULL, -1 },
  { "VOLUME",         OPT_M_VOLUME_INTFC,   VALUE_STR, NULL, -1 },
};
#define BDJOPT_MACHINE_DFKEY_COUNT (sizeof (bdjoptmachdfkeys) / sizeof (datafilekey_t))

/* be sure this is sorted in ascii order */
static datafilekey_t bdjoptmachprofiledfkeys[] = {
  { "LISTINGFONT",          OPT_MP_LISTING_FONT,          VALUE_STR, NULL, -1 },
  { "MQFONT",               OPT_MP_MQFONT,                VALUE_STR, NULL, -1 },
  { "MQ_THEME",             OPT_MP_MQ_THEME,              VALUE_STR, NULL, -1 },
  { "PLAYEROPTIONS",        OPT_MP_PLAYEROPTIONS,         VALUE_STR, NULL, -1 },
  { "PLAYERSHUTDOWNSCRIPT", OPT_MP_PLAYERSHUTDOWNSCRIPT,  VALUE_STR, NULL, -1 },
  { "PLAYERSTARTSCRIPT",    OPT_MP_PLAYERSTARTSCRIPT,     VALUE_STR, NULL, -1 },
  { "UIFONT",               OPT_MP_UIFONT,                VALUE_STR, NULL, -1 },
  { "UI_THEME",             OPT_MP_UI_THEME,              VALUE_STR, NULL, -1 },
};
#define BDJOPT_MACH_PROFILE_DFKEY_COUNT (sizeof (bdjoptmachprofiledfkeys) / sizeof (datafilekey_t))

void
bdjoptInit (void)
{
  datafile_t    *df;
  char          path [MAXPATHLEN];
  char          *ddata;
  nlist_t       *tlist;

  bdjoptprofiledfcount = BDJOPT_PROFILE_DFKEY_COUNT;

  /* global */
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  if (! fileopFileExists (path)) {
    bdjoptCreateNewConfigs ();
  }
  df = datafileAllocParse ("bdjopt-g", DFTYPE_KEY_VAL, path,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, DATAFILE_NO_LOOKUP);

  /* profile */
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  datafileParseMerge (tlist, ddata, "bdjopt-p", DFTYPE_KEY_VAL,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine */
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine per profile */
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-mp", DFTYPE_KEY_VAL,
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
bdjoptGetStr (ssize_t idx)
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
bdjoptSetStr (ssize_t idx, const char *value)
{
  if (bdjopt == NULL) {
    return;
  }
  nlistSetStr (bdjopt->data, idx, value);
}

void
bdjoptSetNum (ssize_t idx, ssize_t value)
{
  if (bdjopt == NULL) {
    return;
  }
  nlistSetNum (bdjopt->data, idx, value);
}

void
bdjoptCreateDirectories (void)
{
  char      path [MAXPATHLEN];

  pathbldMakePath (path, sizeof (path), "", "", "", PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "profiles", "", "", PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", "",
      PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "profiles", "", "",
      PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
}

void
bdjoptSave (void)
{
  char    path [MAXPATHLEN];

  /* global */
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("config-global", path, bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, bdjopt->data);

  /* profile */
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("config-profile", path, bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT, bdjopt->data);

  /* machine */
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("config-machine", path, bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT, bdjopt->data);

  /* machine/profile */
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("config-machine-profile", path, bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT, bdjopt->data);
}

void
bdjoptConvBPM (datafileconv_t *conv)
{
  bdjbpm_t   sbpm = BPM_BPM;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->u.str, "BPM") == 0) {
      sbpm = BPM_BPM;
    }
    if (strcmp (conv->u.str, "MPM") == 0) {
      sbpm = BPM_MPM;
    }
    conv->u.num = sbpm;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->u.num) {
      case BPM_BPM: { conv->u.str = "BPM"; break; }
      case BPM_MPM: { conv->u.str = "MPM"; break; }
    }
  }
}

/* internal routines */

static void
bdjoptConvFadeType (datafileconv_t *conv)
{
  bdjfadetype_t   fadetype = FADETYPE_TRIANGLE;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->u.str, "quartersine") == 0) {
      fadetype = FADETYPE_QUARTER_SINE;
    }
    if (strcmp (conv->u.str, "halfsine") == 0) {
      fadetype = FADETYPE_HALF_SINE;;
    }
    if (strcmp (conv->u.str, "logarithmic") == 0) {
      fadetype = FADETYPE_LOGARITHMIC;
    }
    if (strcmp (conv->u.str, "invertedparabola") == 0) {
      fadetype = FADETYPE_INVERTED_PARABOLA;
    }
    conv->u.num = fadetype;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->u.num) {
      case FADETYPE_TRIANGLE: { conv->u.str = "triangle"; break; }
      case FADETYPE_QUARTER_SINE: { conv->u.str = "quartersine"; break; }
      case FADETYPE_HALF_SINE: { conv->u.str = "halfsine"; break; }
      case FADETYPE_LOGARITHMIC: { conv->u.str = "logarithmic"; break; }
      case FADETYPE_INVERTED_PARABOLA: { conv->u.str = "invertedparabola"; break; }
    }
  }
}

static void
bdjoptConvWriteTags (datafileconv_t *conv)
{
  bdjwritetags_t   wtag = WRITE_TAGS_NONE;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->u.str, "NONE") == 0) {
      wtag = WRITE_TAGS_NONE;
    }
    if (strcmp (conv->u.str, "ALL") == 0) {
      wtag = WRITE_TAGS_ALL;
    }
    if (strcmp (conv->u.str, "BDJ") == 0) {
      wtag = WRITE_TAGS_BDJ_ONLY;
    }
    conv->u.num = wtag;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->u.num) {
      case WRITE_TAGS_ALL: { conv->u.str = "ALL"; break; }
      case WRITE_TAGS_BDJ_ONLY: { conv->u.str = "BDJ"; break; }
      case WRITE_TAGS_NONE: { conv->u.str = "NONE"; break; }
    }
  }
}

static void
bdjoptCreateNewConfigs (void)
{
  ssize_t   currProfile = sysvarsGetNum (SVL_BDJIDX);
  char      path [MAXPATHLEN];
  char      tpath [MAXPATHLEN];

    /* see if profile 0 exists */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  if (! fileopFileExists (path)) {
    bdjoptCreateDefaultFiles ();
  }

    /* global */
  sysvarsSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, sizeof (tpath), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, sizeof (tpath), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, sizeof (tpath), "", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

    /* per machine per profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, currProfile);
  pathbldMakePath (tpath, sizeof (tpath), "profiles", BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  filemanipCopy (path, tpath);

  sysvarsSetNum (SVL_BDJIDX, currProfile);
}

static void
bdjoptCreateDefaultFiles (void)
{
  bdjoptCreateDirectories ();
}

static void
bdjoptConvMobileMq (datafileconv_t *conv)
{
  bdjmobilemq_t   val = MOBILEMQ_OFF;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->u.str, "off") == 0) {
      val = MOBILEMQ_OFF;
    }
    if (strcmp (conv->u.str, "local") == 0) {
      val = MOBILEMQ_LOCAL;
    }
    if (strcmp (conv->u.str, "internet") == 0) {
      val = MOBILEMQ_INTERNET;
    }
    conv->u.num = val;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->u.num) {
      case MOBILEMQ_OFF: { conv->u.str = "off"; break; }
      case MOBILEMQ_LOCAL: { conv->u.str = "local"; break; }
      case MOBILEMQ_INTERNET: { conv->u.str = "internet"; break; }
    }
  }
}

