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
#include "bdjstring.h"
#include "datafile.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "nlist.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"

static int  bdjoptQueueIndex (nlistidx_t idx, int musiqc);
static void bdjoptConvFadeType (datafileconv_t *conv);
static void bdjoptConvWriteTags (datafileconv_t *conv);
static void bdjoptCreateNewConfigs (void);

typedef struct {
  int           currprofile;
  datafile_t    *df;
  nlist_t       *bdjoptList;
  char          *fname [OPTTYPE_MAX];
  datafilekey_t *dfkeys [OPTTYPE_MAX];
  int           dfcount [OPTTYPE_MAX];
  const char    *tag [OPTTYPE_MAX];
  const char    *shorttag [OPTTYPE_MAX];
} bdjopt_t;

static bdjopt_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys [] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_NUM, convBoolean, -1 },
  { "BDJ3COMPATTAGS",     OPT_G_BDJ3_COMPAT_TAGS,   VALUE_NUM, convBoolean, -1 },
  { "BPM",                OPT_G_BPM,                VALUE_NUM, bdjoptConvBPM, -1 },
  { "CLOCKDISP",          OPT_G_CLOCK_DISP,         VALUE_NUM, bdjoptConvClock, -1 },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_NUM, NULL, -1 },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, convBoolean, -1 },
  { "ORGPATH",            OPT_G_ORGPATH,            VALUE_STR, NULL, -1 },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_NUM, NULL, -1 },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_STR, NULL, -1 },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_NUM, bdjoptConvWriteTags, -1 },
};

static datafilekey_t bdjoptprofiledfkeys [] = {
  { "COMPLETEMSG",          OPT_P_COMPLETE_MSG,         VALUE_STR, NULL, -1 },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_NUM, NULL, -1 },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_NUM, bdjoptConvFadeType, -1 },
  { "HIDEMARQUEEONSTART",   OPT_P_HIDE_MARQUEE_ON_START,VALUE_NUM, convBoolean, -1 },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_NUM, convBoolean, -1 },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_NUM, NULL, -1 },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_STR, NULL, -1 },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_NUM, NULL, -1 },
  { "MQSHOWINFO",           OPT_P_MQ_SHOW_INFO,         VALUE_NUM, convBoolean, -1 },
  { "MQ_ACCENT_COL",        OPT_P_MQ_ACCENT_COL,        VALUE_STR, NULL, -1 },
  { "MQ_INFO_COL",          OPT_P_MQ_INFO_COL,          VALUE_STR, NULL, -1 },
  { "MQ_TEXT_COL",          OPT_P_MQ_TEXT_COL,          VALUE_STR, NULL, -1 },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_STR, NULL, -1 },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_STR, NULL, -1 },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_NUM, NULL, -1 },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_STR, NULL, -1 },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_NUM, convBoolean, -1 },
  { "STOPATTIME",           OPT_P_STOPATTIME,           VALUE_NUM, NULL, -1 },
  { "UI_ACCENT_COL",        OPT_P_UI_ACCENT_COL,        VALUE_STR, NULL, -1 },
  { "UI_ERROR_COL",         OPT_P_UI_ERROR_COL,         VALUE_STR, NULL, -1 },
  { "UI_MARK_COL",          OPT_P_UI_MARK_COL,          VALUE_STR, NULL, -1 },
  { "UI_PROFILE_COL",       OPT_P_UI_PROFILE_COL,       VALUE_STR, NULL, -1 },
};

static datafilekey_t bdjoptqueuedfkeys [] = {
  { "ACTIVE",               OPT_Q_ACTIVE,               VALUE_NUM, convBoolean, -1 },
  { "DISPLAY",              OPT_Q_DISPLAY,              VALUE_NUM, convBoolean, -1 },
  { "FADEINTIME",           OPT_Q_FADEINTIME,           VALUE_NUM, NULL, -1 },
  { "FADEOUTTIME",          OPT_Q_FADEOUTTIME,          VALUE_NUM, NULL, -1 },
  { "GAP",                  OPT_Q_GAP,                  VALUE_NUM, NULL, -1 },
  { "MAXPLAYTIME",          OPT_Q_MAXPLAYTIME,          VALUE_NUM, NULL, -1 },
  { "PAUSEEACHSONG",        OPT_Q_PAUSE_EACH_SONG,      VALUE_NUM, convBoolean, -1 },
  { "PLAYANNOUNCE",         OPT_Q_PLAY_ANNOUNCE,        VALUE_NUM, convBoolean, -1 },
  { "QUEUE_NAME",           OPT_Q_QUEUE_NAME,           VALUE_STR, NULL, -1 },
};

static datafilekey_t bdjoptmachinedfkeys [] = {
  { "DIRITUNESMEDIA", OPT_M_DIR_ITUNES_MEDIA,VALUE_STR, NULL, -1 },
  { "DIRMUSIC",       OPT_M_DIR_MUSIC,      VALUE_STR, NULL, -1 },
  { "DIROLDSKIP",     OPT_M_DIR_OLD_SKIP,   VALUE_STR, NULL, -1 },
  { "ITUNESXMLFILE",  OPT_M_ITUNES_XML_FILE,VALUE_STR, NULL, -1 },
  { "PLAYER",         OPT_M_PLAYER_INTFC,   VALUE_STR, NULL, -1 },
  { "SHUTDOWNSCRIPT", OPT_M_SHUTDOWNSCRIPT, VALUE_STR, NULL, -1 },
  { "STARTUPSCRIPT",  OPT_M_STARTUPSCRIPT,  VALUE_STR, NULL, -1 },
  { "VOLUME",         OPT_M_VOLUME_INTFC,   VALUE_STR, NULL, -1 },
};

static datafilekey_t bdjoptmachprofdfkeys [] = {
  { "AUDIOSINK",            OPT_MP_AUDIOSINK,             VALUE_STR, NULL, -1 },
  { "LISTINGFONT",          OPT_MP_LISTING_FONT,          VALUE_STR, NULL, -1 },
  { "MQFONT",               OPT_MP_MQFONT,                VALUE_STR, NULL, -1 },
  { "MQ_THEME",             OPT_MP_MQ_THEME,              VALUE_STR, NULL, -1 },
  { "PLAYEROPTIONS",        OPT_MP_PLAYEROPTIONS,         VALUE_STR, NULL, -1 },
  { "PLAYERSHUTDOWNSCRIPT", OPT_MP_PLAYERSHUTDOWNSCRIPT,  VALUE_STR, NULL, -1 },
  { "PLAYERSTARTSCRIPT",    OPT_MP_PLAYERSTARTSCRIPT,     VALUE_STR, NULL, -1 },
  { "UIFONT",               OPT_MP_UIFONT,                VALUE_STR, NULL, -1 },
  { "UI_THEME",             OPT_MP_UI_THEME,              VALUE_STR, NULL, -1 },
};

void
bdjoptInit (void)
{
  datafile_t    *df;
  char          path [MAXPATHLEN];
  char          *ddata;
  nlist_t       *tlist;

  if (bdjopt != NULL) {
    bdjoptCleanup ();
  }

  bdjopt = malloc (sizeof (bdjopt_t));
  assert (bdjopt != NULL);
  bdjopt->currprofile = 0;
  bdjopt->bdjoptList = NULL;

  bdjopt->dfkeys [OPTTYPE_GLOBAL] = bdjoptglobaldfkeys;
  bdjopt->dfkeys [OPTTYPE_PROFILE] = bdjoptprofiledfkeys;
  bdjopt->dfkeys [OPTTYPE_QUEUE] = bdjoptqueuedfkeys;
  bdjopt->dfkeys [OPTTYPE_MACHINE] = bdjoptmachinedfkeys;
  bdjopt->dfkeys [OPTTYPE_MACH_PROF] = bdjoptmachprofdfkeys;
  bdjopt->dfcount [OPTTYPE_GLOBAL] = sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_PROFILE] = sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_QUEUE] = sizeof (bdjoptqueuedfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_MACHINE] = sizeof (bdjoptmachinedfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_MACH_PROF] = sizeof (bdjoptmachprofdfkeys) / sizeof (datafilekey_t);
  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    bdjopt->fname [i] = NULL;
  }
  bdjopt->tag [OPTTYPE_GLOBAL] = "bdjopt-g";
  bdjopt->tag [OPTTYPE_PROFILE] = "bdjopt-p";
  bdjopt->tag [OPTTYPE_QUEUE] = "bdjopt-q";
  bdjopt->tag [OPTTYPE_MACHINE] = "bdjopt-m";
  bdjopt->tag [OPTTYPE_MACH_PROF] = "bdjopt-mp";
  bdjopt->shorttag [OPTTYPE_GLOBAL] = "g";
  bdjopt->shorttag [OPTTYPE_PROFILE] = "p";
  bdjopt->shorttag [OPTTYPE_QUEUE] = "q";
  bdjopt->shorttag [OPTTYPE_MACHINE] = "m";
  bdjopt->shorttag [OPTTYPE_MACH_PROF] = "mp";

  bdjopt->currprofile = sysvarsGetNum (SVL_BDJIDX);

  /* global */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  bdjopt->fname [OPTTYPE_GLOBAL] = strdup (path);

  /* profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_PROFILE] = strdup (path);

  /* queue */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      "", PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_QUEUE] = strdup (path);

  /* per machine */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME);
  bdjopt->fname [OPTTYPE_MACHINE] = strdup (path);

  /* per machine per profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_MACH_PROF] = strdup (path);

  if (! fileopFileExists (bdjopt->fname [OPTTYPE_PROFILE])) {
    bdjoptCreateNewConfigs ();
  }

  df = datafileAllocParse (bdjopt->tag [OPTTYPE_GLOBAL], DFTYPE_KEY_VAL,
      bdjopt->fname [OPTTYPE_GLOBAL], bdjopt->dfkeys [OPTTYPE_GLOBAL],
      bdjopt->dfcount [OPTTYPE_GLOBAL]);

  for (int i = 1; i < OPTTYPE_MAX; ++i) {
    if (i == OPTTYPE_QUEUE) {
      continue;
    }
    ddata = datafileLoad (df, DFTYPE_KEY_VAL, bdjopt->fname [i]);
    tlist = datafileGetList (df);
    datafileParseMerge (tlist, ddata, bdjopt->tag [i], DFTYPE_KEY_VAL,
        bdjopt->dfkeys [i], bdjopt->dfcount [i], 0);
    datafileSetData (df, tlist);
    free (ddata);
  }

  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    int   offset;

    snprintf (path, sizeof (path), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);
    ddata = datafileLoad (df, DFTYPE_KEY_VAL, path);
    tlist = datafileGetList (df);
    offset = bdjopt->dfcount [OPTTYPE_QUEUE] * i;
    datafileParseMerge (tlist, ddata, bdjopt->tag [OPTTYPE_QUEUE], DFTYPE_KEY_VAL,
        bdjopt->dfkeys [OPTTYPE_QUEUE], bdjopt->dfcount [OPTTYPE_QUEUE], offset);
    datafileSetData (df, tlist);
    free (ddata);
  }

  bdjopt->df = df;
  bdjopt->bdjoptList = datafileGetList (df);
}

void
bdjoptCleanup (void)
{
  if (bdjopt != NULL) {
    datafileFree (bdjopt->df);
    for (int i = 0; i < OPTTYPE_MAX; ++i) {
      dataFree (bdjopt->fname [i]);
      bdjopt->fname [i] = NULL;
    }
    free (bdjopt);
  }
  bdjopt = NULL;
}

char *
bdjoptGetStr (nlistidx_t idx)
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

int64_t
bdjoptGetNum (nlistidx_t idx)
{
  int64_t     value;

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
bdjoptSetStr (nlistidx_t idx, const char *value)
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
bdjoptSetNum (nlistidx_t idx, int64_t value)
{
  if (bdjopt == NULL) {
    return;
  }
  nlistSetNum (bdjopt->bdjoptList, idx, value);
}

char *
bdjoptGetStrPerQueue (nlistidx_t idx, int musicq)
{
  nlistidx_t  nidx;
  void        *value = NULL;

  if (bdjopt == NULL) {
    return NULL;
  }
  if (bdjopt->bdjoptList == NULL) {
    return NULL;
  }

  nidx = bdjoptQueueIndex (idx, musicq);
  value = nlistGetStr (bdjopt->bdjoptList, nidx);
  return value;
}

int64_t
bdjoptGetNumPerQueue (nlistidx_t idx, int musicq)
{
  nlistidx_t  nidx;
  int64_t     value;

  if (bdjopt == NULL) {
    return -1;
  }
  if (bdjopt->bdjoptList == NULL) {
    return -1;
  }

  nidx = bdjoptQueueIndex (idx, musicq);
  value = nlistGetNum (bdjopt->bdjoptList, nidx);
  return value;
}

void
bdjoptSetStrPerQueue (nlistidx_t idx, const char *value, int musicq)
{
  nlistidx_t    nidx;

  if (bdjopt == NULL) {
    return;
  }
  if (bdjopt->bdjoptList == NULL) {
    return;
  }

  if (musicq >= BDJ4_QUEUE_MAX) {
    musicq = 0;
  }
  nidx = idx + musicq * bdjopt->dfcount [OPTTYPE_QUEUE];
  nlistSetStr (bdjopt->bdjoptList, nidx, value);
}

void
bdjoptSetNumPerQueue (nlistidx_t idx, int64_t value, int musicq)
{
  nlistidx_t    nidx;

  if (bdjopt == NULL) {
    return;
  }

  if (musicq >= BDJ4_QUEUE_MAX) {
    musicq = 0;
  }
  nidx = idx + musicq * bdjopt->dfcount [OPTTYPE_QUEUE];
  nlistSetNum (bdjopt->bdjoptList, nidx, value);
}

void
bdjoptCreateDirectories (void)
{
  char      path [MAXPATHLEN];

  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DATA);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "",
      PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  diropMakeDir (path);
}

void
bdjoptSave (void)
{
  char    path [MAXPATHLEN];

  if (bdjopt == NULL) {
    return;
  }

  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    if (i == OPTTYPE_QUEUE) {
      continue;
    }
    datafileSaveKeyVal (bdjopt->tag [i], bdjopt->fname [i],
        bdjopt->dfkeys [i], bdjopt->dfcount [i], bdjopt->bdjoptList, 0);
  }

  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    int   offset;

    snprintf (path, sizeof (path), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);
    offset = bdjopt->dfcount [OPTTYPE_QUEUE] * i;
    datafileSaveKeyVal (bdjopt->tag [OPTTYPE_QUEUE], path,
        bdjopt->dfkeys [OPTTYPE_QUEUE], bdjopt->dfcount [OPTTYPE_QUEUE],
        bdjopt->bdjoptList, offset);
  }
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
bdjoptConvClock (datafileconv_t *conv)
{
  int   nclock = TM_CLOCK_LOCAL;
  char  *sval = NULL;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    if (strcmp (conv->str, "iso") == 0) {
      nclock = TM_CLOCK_ISO;
    }
    if (strcmp (conv->str, "local") == 0) {
      nclock = TM_CLOCK_LOCAL;
    }
    if (strcmp (conv->str, "time12") == 0) {
      nclock = TM_CLOCK_TIME_12;
    }
    if (strcmp (conv->str, "time24") == 0) {
      nclock = TM_CLOCK_TIME_24;
    }
    if (strcmp (conv->str, "off") == 0) {
      nclock = TM_CLOCK_OFF;
    }
    conv->num = nclock;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    switch (conv->num) {
      case TM_CLOCK_ISO: { sval = "iso"; break; }
      case TM_CLOCK_LOCAL: { sval = "local"; break; }
      case TM_CLOCK_TIME_12: { sval = "time12"; break; }
      case TM_CLOCK_TIME_24: { sval = "time24"; break; }
      case TM_CLOCK_OFF: { sval = "off"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptDump (void)
{
  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    if (i == OPTTYPE_QUEUE) {
      continue;
    }
    datafileDumpKeyVal (bdjopt->shorttag [i], bdjopt->dfkeys [i],
        bdjopt->dfcount [i], bdjopt->bdjoptList, 0);
  }
  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    char  tmp [20];
    int   offset;

    snprintf (tmp, sizeof (tmp), "%s%d", bdjopt->shorttag [OPTTYPE_QUEUE], i);
    offset = bdjopt->dfcount [OPTTYPE_QUEUE] * i;
    datafileDumpKeyVal (tmp, bdjopt->dfkeys [OPTTYPE_QUEUE],
        bdjopt->dfcount [OPTTYPE_QUEUE], bdjopt->bdjoptList, offset);
  }
}

bool
bdjoptProfileExists (void)
{
  char      tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  return fileopFileExists (tbuff);
}

char *
bdjoptGetProfileName (void)
{
  char        tbuff [MAXPATHLEN];
  datafile_t  *df = NULL;
  nlist_t     *dflist = NULL;
  char        *pname = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  df = datafileAllocParse (bdjopt->tag [OPTTYPE_PROFILE], DFTYPE_KEY_VAL,
      tbuff, bdjopt->dfkeys [OPTTYPE_PROFILE],
      bdjopt->dfcount [OPTTYPE_PROFILE]);
  dflist = datafileGetList (df);
  pname = strdup (nlistGetStr (dflist, OPT_P_PROFILENAME));
  datafileFree (df);
  return pname;
}

/* internal routines */

static int
bdjoptQueueIndex (nlistidx_t idx, int musicq)
{
  nlistidx_t    nidx;

  if (musicq >= BDJ4_QUEUE_MAX) {
    musicq = 0;
  }
  nidx = OPT_Q_ACTIVE + (musicq * bdjopt->dfcount [OPTTYPE_QUEUE]);
  /* if the queue is not active, use the values from queue 0 */
  /* excepting the queue name, active flag and display flag */
  if (idx != OPT_Q_QUEUE_NAME &&
      idx != OPT_Q_ACTIVE &&
      idx != OPT_Q_DISPLAY &&
      ! nlistGetNum (bdjopt->bdjoptList, nidx)) {
    musicq = 0;
  }
  nidx = idx + (musicq * bdjopt->dfcount [OPTTYPE_QUEUE]);
  return nidx;
}

static void
bdjoptConvFadeType (datafileconv_t *conv)
{
  bdjfadetype_t   fadetype = FADETYPE_TRIANGLE;
  char            *sval;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    fadetype = FADETYPE_TRIANGLE;
    if (strcmp (conv->str, "quartersine") == 0) {
      fadetype = FADETYPE_QUARTER_SINE;
    }
    if (strcmp (conv->str, "halfsine") == 0) {
      fadetype = FADETYPE_HALF_SINE;;
    }
    if (strcmp (conv->str, "logarithmic") == 0) {
      fadetype = FADETYPE_LOGARITHMIC;
    }
    if (strcmp (conv->str, "invertedparabola") == 0) {
      fadetype = FADETYPE_INVERTED_PARABOLA;
    }
    conv->num = fadetype;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    sval = "triangle";
    switch (conv->num) {
      case FADETYPE_TRIANGLE: { sval = "triangle"; break; }
      case FADETYPE_QUARTER_SINE: { sval = "quartersine"; break; }
      case FADETYPE_HALF_SINE: { sval = "halfsine"; break; }
      case FADETYPE_LOGARITHMIC: { sval = "logarithmic"; break; }
      case FADETYPE_INVERTED_PARABOLA: { sval = "invertedparabola"; break; }
    }
    conv->str = sval;
  }
}

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

  if (bdjopt->fname [OPTTYPE_PROFILE] == NULL) {
    return;
  }

  if (! fileopFileExists (bdjopt->fname [OPTTYPE_PROFILE])) {
    bdjoptCreateDirectories ();
  }

  /* global */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_GLOBAL]);

  /* profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_PROFILE]);

  /* queue */
  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    char  fpath [MAXPATHLEN];
    char  tpath [MAXPATHLEN];

    sysvarsSetNum (SVL_BDJIDX, 0);
    pathbldMakePath (path, sizeof (path),
        BDJ_CONFIG_BASEFN, "", PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
    snprintf (fpath, sizeof (fpath), "%s.q%d%s",
        path, i, BDJ4_CONFIG_EXT);

    sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
    snprintf (tpath, sizeof (tpath), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);

    filemanipCopy (fpath, tpath);
  }

  /* per machine */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_MACHINE]);

  /* per machine per profile */
  sysvarsSetNum (SVL_BDJIDX, 0);
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_BDJIDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_MACH_PROF]);
}


