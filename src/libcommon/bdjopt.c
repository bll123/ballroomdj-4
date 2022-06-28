#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "datafile.h"
#include "pathbld.h"
#include "filemanip.h"
#include "fileop.h"
#include "nlist.h"
#include "sysvars.h"

static void bdjoptConvWriteTags (datafileconv_t *conv);
static void bdjoptCreateNewConfigs (void);
static void bdjoptConvMobileMq (datafileconv_t *conv);

static bdjopt_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys[] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_NUM, convBoolean, -1 },
  { "BPM",                OPT_G_BPM,                VALUE_NUM, bdjoptConvBPM, -1 },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_NUM, NULL, -1 },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_NUM, convBoolean, -1 },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, convBoolean, -1 },
  { "PATHFMT",            OPT_G_AO_PATHFMT,         VALUE_STR, NULL, -1 },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_NUM, NULL, -1 },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_STR, NULL, -1 },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_NUM, bdjoptConvWriteTags, -1 },
};
enum {
  BDJOPT_GLOBAL_DFKEY_COUNT = (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))
};

datafilekey_t bdjoptprofiledfkeys[] = {
  { "COMPLETEMSG",          OPT_P_COMPLETE_MSG,         VALUE_STR, NULL, -1 },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_NUM, NULL, -1 },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_NUM, NULL, -1 },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_NUM, NULL, -1 },
  { "GAP",                  OPT_P_GAP,                  VALUE_NUM, NULL, -1 },
  { "HIDEMARQUEEONSTART",   OPT_P_HIDE_MARQUEE_ON_START,VALUE_NUM, convBoolean, -1 },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_NUM, NULL, -1 },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_NUM, bdjoptConvMobileMq, -1 },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_NUM, NULL, -1 },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_STR, NULL, -1 },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_STR, NULL, -1 },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_NUM, NULL, -1 },
  { "MQSHOWINFO",           OPT_P_MQ_SHOW_INFO,         VALUE_NUM, convBoolean, -1 },
  { "MQ_ACCENT_COL",        OPT_P_MQ_ACCENT_COL,        VALUE_STR, NULL, -1 },
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
enum {
  BDJOPT_PROFILE_DFKEY_COUNT = (sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t))
};

static datafilekey_t bdjoptmachdfkeys[] = {
  { "AUDIOSINK",      OPT_M_AUDIOSINK,      VALUE_STR, NULL, -1 },
  { "DIRMUSIC",       OPT_M_DIR_MUSIC,      VALUE_STR, NULL, -1 },
  { "DIROLDSKIP",     OPT_M_DIR_OLD_SKIP,   VALUE_STR, NULL, -1 },
  { "PLAYER",         OPT_M_PLAYER_INTFC,   VALUE_STR, NULL, -1 },
  { "SHUTDOWNSCRIPT", OPT_M_SHUTDOWNSCRIPT, VALUE_STR, NULL, -1 },
  { "STARTUPSCRIPT",  OPT_M_STARTUPSCRIPT,  VALUE_STR, NULL, -1 },
  { "VOLUME",         OPT_M_VOLUME_INTFC,   VALUE_STR, NULL, -1 },
};
enum {
  BDJOPT_MACHINE_DFKEY_COUNT = (sizeof (bdjoptmachdfkeys) / sizeof (datafilekey_t))
};

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
enum {
  BDJOPT_MACH_PROFILE_DFKEY_COUNT = (sizeof (bdjoptmachprofiledfkeys) / sizeof (datafilekey_t))
};

void
bdjoptInit (void)
{
  datafile_t    *df;
  char          path [MAXPATHLEN];
  char          *ddata;
  nlist_t       *tlist;

  if (bdjopt != NULL) {
    bdjoptFree ();
  }

  bdjopt = malloc (sizeof (bdjopt_t));
  assert (bdjopt != NULL);
  bdjopt->currprofile = 0;
  bdjopt->bdjoptList = NULL;
  bdjopt->globalFname = NULL;
  bdjopt->profileFname = NULL;
  bdjopt->machineFname = NULL;
  bdjopt->machineProfileFname = NULL;

  bdjoptprofiledfcount = BDJOPT_PROFILE_DFKEY_COUNT;
  bdjopt->currprofile = sysvarsGetNum (SVL_BDJIDX);

  /* global */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_DATA);
  bdjopt->globalFname = strdup (path);

  /* profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX | PATHBLD_MP_DATA);
  bdjopt->profileFname = strdup (path);

  /* per machine */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_DATA);
  bdjopt->machineFname = strdup (path);

  /* per machine per profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX | PATHBLD_MP_DATA);
  bdjopt->machineProfileFname = strdup (path);

  if (! fileopFileExists (bdjopt->profileFname)) {
    bdjoptCreateNewConfigs ();
  }

  df = datafileAllocParse ("bdjopt-g", DFTYPE_KEY_VAL, bdjopt->globalFname,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, DATAFILE_NO_LOOKUP);

  /* profile */
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, bdjopt->profileFname);
  tlist = datafileGetList (df);
  datafileParseMerge (tlist, ddata, "bdjopt-p", DFTYPE_KEY_VAL,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine */
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, bdjopt->machineFname);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-m", DFTYPE_KEY_VAL,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  /* per machine per profile */
  ddata = datafileLoad (df, DFTYPE_KEY_VAL, bdjopt->machineProfileFname);
  tlist = datafileGetList (df);
  tlist = datafileParseMerge (tlist, ddata, "bdjopt-mp", DFTYPE_KEY_VAL,
      bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT,
      DATAFILE_NO_LOOKUP, NULL);
  datafileSetData (df, tlist);
  free (ddata);

  bdjopt->df = df;
  bdjopt->bdjoptList = datafileGetList (df);
}

void
bdjoptFree (void)
{
  if (bdjopt != NULL) {
    datafileFree (bdjopt->df);
    if (bdjopt->globalFname != NULL) {
      free (bdjopt->globalFname);
    }
    if (bdjopt->profileFname != NULL) {
      free (bdjopt->profileFname);
    }
    if (bdjopt->machineFname != NULL) {
      free (bdjopt->machineFname);
    }
    if (bdjopt->machineProfileFname != NULL) {
      free (bdjopt->machineProfileFname);
    }
    free (bdjopt);
  }
  bdjopt = NULL;
}

char *
bdjoptGetStr (ssize_t idx)
{
  void      *value = NULL;

  if (bdjopt == NULL) {
    return NULL;
  }
  if (bdjopt->bdjoptList == NULL) {
    return NULL;
  }
  value = nlistGetStr (bdjopt->bdjoptList, idx);
  return value;
}

ssize_t
bdjoptGetNum (ssize_t idx)
{
  ssize_t       value;

  if (bdjopt == NULL) {
    return -1;
  }
  if (bdjopt->bdjoptList == NULL) {
    return -1;
  }
  value = nlistGetNum (bdjopt->bdjoptList, idx);
  return value;
}

void
bdjoptSetStr (ssize_t idx, const char *value)
{
  if (bdjopt == NULL) {
    return;
  }
  if (bdjopt->bdjoptList == NULL) {
    return;
  }
  nlistSetStr (bdjopt->bdjoptList, idx, value);
}

void
bdjoptSetNum (ssize_t idx, ssize_t value)
{
  if (bdjopt == NULL) {
    return;
  }
  nlistSetNum (bdjopt->bdjoptList, idx, value);
}

void
bdjoptCreateDirectories (void)
{
  char      path [MAXPATHLEN];

  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DATA);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_HOSTNAME);
  fileopMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "",
      PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  fileopMakeDir (path);
}

void
bdjoptSave (void)
{
  if (bdjopt == NULL) {
    return;
  }

  /* global */
  datafileSaveKeyVal ("config-global", bdjopt->globalFname,
      bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, bdjopt->bdjoptList);

  /* profile */
  datafileSaveKeyVal ("config-profile", bdjopt->profileFname,
      bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT, bdjopt->bdjoptList);

  /* machine */
  datafileSaveKeyVal ("config-machine", bdjopt->machineFname,
      bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT, bdjopt->bdjoptList);

  /* machine/profile */
  datafileSaveKeyVal ("config-machine-profile", bdjopt->machineProfileFname,
      bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT, bdjopt->bdjoptList);
}

void
bdjoptConvBPM (datafileconv_t *conv)
{
  bdjbpm_t   nbpm = BPM_BPM;
  char       *sval = NULL;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->str, "BPM") == 0) {
      nbpm = BPM_BPM;
    }
    if (strcmp (conv->str, "MPM") == 0) {
      nbpm = BPM_MPM;
    }
    conv->num = nbpm;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->num) {
      case BPM_BPM: { sval = "BPM"; break; }
      case BPM_MPM: { sval = "MPM"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptDump (void)
{
  /* global */
  datafileDumpKeyVal ("g", bdjoptglobaldfkeys, BDJOPT_GLOBAL_DFKEY_COUNT, bdjopt->bdjoptList);

  /* profile */
  datafileDumpKeyVal ("p", bdjoptprofiledfkeys, BDJOPT_PROFILE_DFKEY_COUNT, bdjopt->bdjoptList);

  /* machine */
  datafileDumpKeyVal ("m", bdjoptmachdfkeys, BDJOPT_MACHINE_DFKEY_COUNT, bdjopt->bdjoptList);

  /* machine/profile */
  datafileDumpKeyVal ("mp", bdjoptmachprofiledfkeys, BDJOPT_MACH_PROFILE_DFKEY_COUNT, bdjopt->bdjoptList);
}

/* internal routines */

static void
bdjoptConvWriteTags (datafileconv_t *conv)
{
  bdjwritetags_t  wtag = WRITE_TAGS_NONE;
  char            *sval;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    wtag = WRITE_TAGS_NONE;
    if (strcmp (conv->str, "NONE") == 0) {
      wtag = WRITE_TAGS_NONE;
    }
    if (strcmp (conv->str, "ALL") == 0) {
      wtag = WRITE_TAGS_ALL;
    }
    if (strcmp (conv->str, "BDJ") == 0) {
      wtag = WRITE_TAGS_BDJ_ONLY;
    }
    conv->num = wtag;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    sval = "NONE";
    switch (conv->num) {
      case WRITE_TAGS_ALL: { sval = "ALL"; break; }
      case WRITE_TAGS_BDJ_ONLY: { sval = "BDJ"; break; }
      case WRITE_TAGS_NONE: { sval = "NONE"; break; }
    }
    conv->str = sval;
  }
}

static void
bdjoptCreateNewConfigs (void)
{
  char      path [MAXPATHLEN];

  if (bdjopt == NULL) {
    return;
  }

  if (bdjopt->profileFname == NULL) {
    return;
  }

  if (! fileopFileExists (bdjopt->profileFname)) {
    bdjoptCreateDirectories ();
  }

  /* global */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ_CONFIG_EXT, PATHBLD_MP_DATA);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->globalFname);

  /* profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->profileFname);

  /* per machine */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->machineFname);

  /* per machine per profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ_CONFIG_EXT, PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->machineProfileFname);
}

static void
bdjoptConvMobileMq (datafileconv_t *conv)
{
  bdjmobilemq_t val = MOBILEMQ_OFF;
  char          *sval;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    val = MOBILEMQ_OFF;
    if (strcmp (conv->str, "off") == 0) {
      val = MOBILEMQ_OFF;
    }
    if (strcmp (conv->str, "local") == 0) {
      val = MOBILEMQ_LOCAL;
    }
    if (strcmp (conv->str, "internet") == 0) {
      val = MOBILEMQ_INTERNET;
    }
    conv->num = val;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    sval = "off";
    switch (conv->num) {
      case MOBILEMQ_OFF: { sval = "off"; break; }
      case MOBILEMQ_LOCAL: { sval = "local"; break; }
      case MOBILEMQ_INTERNET: { sval = "internet"; break; }
    }
    conv->str = sval;
  }
}

