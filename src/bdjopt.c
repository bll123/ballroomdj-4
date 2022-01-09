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
#include "portability.h"

static void bdjoptConvFadeType (char *, datafileret_t *);

static datafile_t   *bdjopt = NULL;

static datafilekey_t bdjoptglobaldfkeys[] = {
  { "AUTOORGANIZE",       OPT_G_AUTOORGANIZE,       VALUE_LONG, parseConvBoolean },
  { "CHANGESPACE",        OPT_G_CHANGESPACE,        VALUE_LONG, parseConvBoolean },
  { "DEBUGLVL",           OPT_G_DEBUGLVL,           VALUE_LONG, NULL },
  { "ENABLEIMGPLAYER",    OPT_G_ENABLEIMGPLAYER,    VALUE_LONG, parseConvBoolean },
  { "ITUNESSUPPORT",      OPT_G_ITUNESSUPPORT,      VALUE_DATA, NULL },
  { "LOADDANCEFROMGENRE", OPT_G_LOADDANCEFROMGENRE, VALUE_LONG, parseConvBoolean },
  { "MUSICDIRDFLT",       OPT_G_MUSICDIRDFLT,       VALUE_DATA, NULL },
  { "PATHFMT",            OPT_G_PATHFMT,            VALUE_DATA, NULL },
  { "PATHFMT_CL",         OPT_G_PATHFMT_CL,         VALUE_DATA, NULL },
  { "PATHFMT_CLVA",       OPT_G_PATHFMT_CLVA,       VALUE_DATA, NULL },
  { "PATHFMT_VA",         OPT_G_PATHFMT_VA,         VALUE_DATA, NULL },
  { "PLAYER",             OPT_G_PLAYER,             VALUE_DATA, NULL },
  { "PLAYERQLEN",         OPT_G_PLAYERQLEN,         VALUE_LONG, NULL },
  { "REMCONTROLHTML",     OPT_G_REMCONTROLHTML,     VALUE_DATA, NULL },
  { "SHOWALBUM",          OPT_G_SHOWALBUM,          VALUE_LONG, parseConvBoolean },
  { "SHOWBPM",            OPT_G_SHOWBPM,            VALUE_LONG, parseConvBoolean },
  { "SHOWCLASSICAL",      OPT_G_SHOWCLASSICAL,      VALUE_LONG, parseConvBoolean },
  { "SHOWSTATUS",         OPT_G_SHOWSTATUS,         VALUE_LONG, parseConvBoolean },
  { "SLOWDEVICE",         OPT_G_SLOWDEVICE,         VALUE_LONG, parseConvBoolean },
  { "STARTMAXIMIZED",     OPT_G_STARTMAXIMIZED,     VALUE_LONG, parseConvBoolean },
  { "VARIOUS",            OPT_G_VARIOUS,            VALUE_DATA, NULL },
  { "WRITETAGS",          OPT_G_WRITETAGS,          VALUE_DATA, NULL },
};
#define BDJOPT_GLOBAL_DFKEY_COUNT (sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t))

static datafilekey_t bdjoptprofiledfkeys[] = {
  { "ALLOWEDIT",            OPT_P_ALLOWEDIT,            VALUE_LONG, parseConvBoolean },
  { "AUTOSTARTUP",          OPT_P_AUTOSTARTUP,          VALUE_DATA, NULL },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,        VALUE_LONG, NULL },
  { "DONEMSG",              OPT_P_DONEMSG,              VALUE_DATA, NULL },
  { "FADEINTIME",           OPT_P_FADEINTIME,           VALUE_LONG, NULL },
  { "FADEOUTTIME",          OPT_P_FADEOUTTIME,          VALUE_LONG, NULL },
  { "FADETYPE",             OPT_P_FADETYPE,             VALUE_DATA, bdjoptConvFadeType },
  { "GAP",                  OPT_P_GAP,                  VALUE_LONG, NULL },
  { "MAXPLAYTIME",          OPT_P_MAXPLAYTIME,          VALUE_LONG, NULL },
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,        VALUE_LONG, parseConvBoolean },
  { "MOBILEMQPORT",         OPT_P_MOBILEMQPORT,         VALUE_LONG, NULL },
  { "MOBILEMQTAG",          OPT_P_MOBILEMQTAG,          VALUE_DATA, NULL },
  { "MOBILEMQTITLE",        OPT_P_MOBILEMQTITLE,        VALUE_DATA, NULL },
  { "MQFONT",               OPT_P_MQFONT,               VALUE_DATA, NULL },
  { "MQFULLSCREEN",         OPT_P_MQFULLSCREEN,         VALUE_DATA, NULL },
  { "MQQLEN",               OPT_P_MQQLEN,               VALUE_DATA, NULL },
  { "PAUSEMSG",             OPT_P_PAUSEMSG,             VALUE_DATA, NULL },
  { "PROFILENAME",          OPT_P_PROFILENAME,          VALUE_DATA, NULL },
  { "QUEUENAME0",           OPT_P_QUEUENAME0,           VALUE_DATA, NULL },
  { "QUEUENAME1",           OPT_P_QUEUENAME1,           VALUE_DATA, NULL },
  { "QUICKPLAYENABLED",     OPT_P_QUICKPLAYENABLED,     VALUE_LONG, parseConvBoolean },
  { "QUICKPLAYSHOW",        OPT_P_QUICKPLAYSHOW,        VALUE_LONG, parseConvBoolean },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,       VALUE_DATA, NULL },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,       VALUE_LONG, NULL },
  { "REMCONTROLSHOWDANCE",  OPT_P_REMCONTROLSHOWDANCE,  VALUE_LONG, parseConvBoolean },
  { "REMCONTROLSHOWSONG",   OPT_P_REMCONTROLSHOWSONG,   VALUE_LONG, parseConvBoolean },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,       VALUE_DATA, NULL },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,        VALUE_LONG, parseConvBoolean },
  { "SERVERNAME",           OPT_P_SERVERNAME,           VALUE_DATA, NULL },
  { "SERVERPASS",           OPT_P_SERVERPASS,           VALUE_DATA, NULL },
  { "SERVERPORT",           OPT_P_SERVERPORT,           VALUE_LONG, NULL },
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
  { "FONTSIZE",             OPT_MP_FONTSIZE,              VALUE_LONG, NULL },
  { "LISTINGFONTSIZE",      OPT_MP_LISTINGFONTSIZE,       VALUE_LONG, NULL },
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

#if 0
long tl;
char *tval;
char *tnm;
long lval;
valuetype_t vt;
int found;
llistStartIterator (tlist);
while ((tl = llistIterateKeyLong (tlist)) >= 0) {
  found = 0;
  vt = llistGetValueType (tlist, tl);
  for (size_t i = 0; ! found && i < BDJOPT_GLOBAL_DFKEY_COUNT; ++i) {
    if (bdjoptglobaldfkeys [i].itemkey == tl) {
      found = 1;
      tnm = bdjoptglobaldfkeys [i].name;
    }
  }
  for (size_t i = 0; ! found && i < BDJOPT_PROFILE_DFKEY_COUNT; ++i) {
    if (bdjoptprofiledfkeys [i].itemkey == tl) {
      found = 1;
      tnm = bdjoptprofiledfkeys [i].name;
    }
  }
  for (size_t i = 0; ! found && i < BDJOPT_MACHINE_DFKEY_COUNT; ++i) {
    if (bdjoptmachdfkeys [i].itemkey == tl) {
      found = 1;
      tnm = bdjoptmachdfkeys [i].name;
    }
  }
  for (size_t i = 0; ! found && i < BDJOPT_MACH_PROFILE_DFKEY_COUNT; ++i) {
    if (bdjoptmachprofiledfkeys [i].itemkey == tl) {
      found = 1;
      tnm = bdjoptmachprofiledfkeys [i].name;
    }
  }
  if (vt == VALUE_DATA) {
    tval = (char *) llistGetData (tlist, tl);
    fprintf (stderr, "%ld: %s %s\n", tl, tnm, tval);
  }
  if (vt == VALUE_LONG) {
    lval = llistGetLong (tlist, tl);
    fprintf (stderr, "%ld: %s %ld\n", tl, tnm, lval);
  }
}
#endif

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
bdjoptGetData (size_t idx)
{
  void      *value = NULL;

  if (bdjopt == NULL) {
    return NULL;
  }
  value = llistGetData (bdjopt->data, idx);
  return value;
}

long
bdjoptGetLong (size_t idx)
{
  if (bdjopt == NULL) {
    return -1;
  }
  long value = llistGetLong (bdjopt->data, idx);
  return value;
}

void
bdjoptSetLong (size_t idx, long value)
{
  if (bdjopt == NULL) {
    return;
  }
  llistSetLong (bdjopt->data, idx, value);
}

static void
bdjoptConvFadeType (char *data, datafileret_t *ret)
{
  bdjfadetype_t   fadetype = FADETYPE_TRIANGLE;

  ret->valuetype = VALUE_LONG;

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
  ret->u.l = fadetype;
}
