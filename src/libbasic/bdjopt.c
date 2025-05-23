/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "datafile.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "mdebug.h"
#include "musicq.h"
#include "nlist.h"
#include "osprocess.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"

typedef enum {
  OPTTYPE_URI,
  OPTTYPE_GLOBAL,
  OPTTYPE_PROFILE,
  OPTTYPE_QUEUE,
  OPTTYPE_MACHINE,
  OPTTYPE_MACH_PROF,
  OPTTYPE_MAX,
} bdjopttype_t;

static const char * const BDJ_CONFIG_BASEFN = "bdjconfig";

typedef struct {
  int           currprofile;
  const char    *tag [OPTTYPE_MAX];
  const char    *shorttag [OPTTYPE_MAX];
  char          *fname [OPTTYPE_MAX];
  datafile_t    *df [OPTTYPE_MAX];
  datafilekey_t *dfkeys [OPTTYPE_MAX];
  int           dfcount [OPTTYPE_MAX];
  int           distvers [OPTTYPE_MAX];
  nlist_t       *bdjoptList;
  bool          listallocated;
} bdjopt_t;

static bdjopt_t   *bdjopt = NULL;
static bool       vlccheckdone = false;

static datafilekey_t bdjoptglobaldfkeys [] = {
  { "ACOUSTID_KEY",         OPT_G_ACOUSTID_KEY,       VALUE_STR, NULL, DF_NORM },
  { "ACRCLOUD_API_HOST",    OPT_G_ACRCLOUD_API_HOST,  VALUE_STR, NULL, DF_NORM },
  { "ACRCLOUD_API_KEY",     OPT_G_ACRCLOUD_API_KEY,   VALUE_STR, NULL, DF_NORM },
  { "ACRCLOUD_API_SECRET",  OPT_G_ACRCLOUD_API_SECRET, VALUE_STR, NULL, DF_NORM },
  { "AUD_ADJ_DISP",         OPT_G_AUD_ADJ_DISP,       VALUE_NUM, convBoolean, DF_NORM },
  { "AUTOORGANIZE",         OPT_G_AUTOORGANIZE,       VALUE_NUM, convBoolean, DF_NORM },
  { "BPM",                  OPT_G_BPM,                VALUE_NUM, bdjoptConvBPM, DF_NORM },
  { "CLOCKDISP",            OPT_G_CLOCK_DISP,         VALUE_NUM, bdjoptConvClock, DF_NORM },
  { "DANCESELMETHOD",       OPT_G_DANCESEL_METHOD,    VALUE_NUM, bdjoptConvDanceselMethod, DF_NORM },
  { "DEBUGLVL",             OPT_G_DEBUGLVL,           VALUE_NUM, NULL, DF_NORM },
  { "LOADDANCEFROMGENRE",   OPT_G_LOADDANCEFROMGENRE, VALUE_NUM, convBoolean, DF_NORM },
  { "OLDORGPATH",           OPT_G_OLDORGPATH,         VALUE_STR, NULL, DF_NORM },
  { "ORGPATH",              OPT_G_ORGPATH,            VALUE_STR, NULL, DF_NORM },
  { "PLAYERQLEN",           OPT_G_PLAYERQLEN,         VALUE_NUM, NULL, DF_NORM },
  { "REMCONTROLHTML",       OPT_G_REMCONTROLHTML,     VALUE_STR, NULL, DF_NORM },
  { "USEWORKMOVEMENT",      OPT_G_USE_WORK_MOVEMENT,  VALUE_NUM, convBoolean, DF_NORM },
  { "WRITETAGS",            OPT_G_WRITETAGS,          VALUE_NUM, bdjoptConvWriteTags, DF_NORM },
};

static datafilekey_t bdjoptprofiledfkeys [] = {
  { "COMPLETEMSG",          OPT_P_COMPLETE_MSG,       VALUE_STR, NULL, DF_NORM },
  { "DEFAULTVOLUME",        OPT_P_DEFAULTVOLUME,      VALUE_NUM, NULL, DF_NORM },
  { "FADETYPE",             OPT_P_FADETYPE,           VALUE_NUM, bdjoptConvFadeType, DF_NORM },
  { "MARQUEE_SHOW",         OPT_P_MARQUEE_SHOW,       VALUE_NUM, bdjoptConvMarqueeShow, DF_NORM },
  /* the mobile mq enable switch was changed to mobmq-type */
  { "MOBILEMARQUEE",        OPT_P_MOBILEMARQUEE,      VALUE_NUM, convBoolean, DF_NO_WRITE },
  { "MOBILEMQKEY",          OPT_P_MOBMQ_KEY,          VALUE_STR, NULL, DF_NORM },
  { "MOBILEMQPORT",         OPT_P_MOBMQ_PORT,         VALUE_NUM, NULL, DF_NORM },
  { "MOBILEMQTAG",          OPT_P_MOBMQ_TAG,          VALUE_STR, NULL, DF_NORM },
  { "MOBILEMQTITLE",        OPT_P_MOBMQ_TITLE,        VALUE_STR, NULL, DF_NORM },
  { "MOBILEMQTYPE",         OPT_P_MOBMQ_TYPE,         VALUE_NUM, bdjoptConvMobMQType, DF_NORM },
  { "MQQLEN",               OPT_P_MQQLEN,             VALUE_NUM, NULL, DF_NORM },
  { "MQSHOWINFO",           OPT_P_MQ_SHOW_INFO,       VALUE_NUM, convBoolean, DF_NORM },
  { "MQ_ACCENT_COL",        OPT_P_MQ_ACCENT_COL,      VALUE_STR, NULL, DF_NORM },
  { "MQ_BG_COL",            OPT_P_MQ_BG_COL,          VALUE_STR, NULL, DF_NORM },
  { "MQ_INFO_COL",          OPT_P_MQ_INFO_COL,        VALUE_STR, NULL, DF_NORM },
  { "MQ_INFO_SEP",          OPT_P_MQ_INFO_SEP,        VALUE_STR, NULL, DF_NORM },
  { "MQ_TEXT_COL",          OPT_P_MQ_TEXT_COL,        VALUE_STR, NULL, DF_NORM },
  { "PLAYER_UI_SEP",        OPT_P_PLAYER_UI_SEP,      VALUE_STR, NULL, DF_NORM },
  { "PROFILENAME",          OPT_P_PROFILENAME,        VALUE_STR, NULL, DF_NORM },
  { "REMCONTROLPASS",       OPT_P_REMCONTROLPASS,     VALUE_STR, NULL, DF_NORM },
  { "REMCONTROLPORT",       OPT_P_REMCONTROLPORT,     VALUE_NUM, NULL, DF_NORM },
  { "REMCONTROLUSER",       OPT_P_REMCONTROLUSER,     VALUE_STR, NULL, DF_NORM },
  { "REMOTECONTROL",        OPT_P_REMOTECONTROL,      VALUE_NUM, convBoolean, DF_NORM },
  { "SHOWSPDCONTROL",       OPT_P_SHOW_SPD_CONTROL,   VALUE_NUM, convBoolean, DF_NORM },
  { "UI_ACCENT_COL",        OPT_P_UI_ACCENT_COL,      VALUE_STR, NULL, DF_NORM },
  { "UI_ERROR_COL",         OPT_P_UI_ERROR_COL,       VALUE_STR, NULL, DF_NORM },
  { "UI_MARK_COL",          OPT_P_UI_MARK_COL,        VALUE_STR, NULL, DF_NORM },
  { "UI_MARK_TEXT",         OPT_P_UI_MARK_TEXT,       VALUE_STR, NULL, DF_NORM },
  { "UI_PROFILE_COL",       OPT_P_UI_PROFILE_COL,     VALUE_STR, NULL, DF_NORM },
  { "UI_ROWSEL_COL",        OPT_P_UI_ROWSEL_COL,      VALUE_STR, NULL, DF_NORM },
  { "UI_ROW_HL_COL",        OPT_P_UI_ROW_HL_COL,      VALUE_STR, NULL, DF_NORM },
};

static datafilekey_t bdjoptqueuedfkeys [] = {
  { "ACTIVE",               OPT_Q_ACTIVE,             VALUE_NUM, convBoolean, DF_NORM },
  { "DISPLAY",              OPT_Q_DISPLAY,            VALUE_NUM, convBoolean, DF_NORM },
  { "FADEINTIME",           OPT_Q_FADEINTIME,         VALUE_NUM, NULL, DF_NORM },
  { "FADEOUTTIME",          OPT_Q_FADEOUTTIME,        VALUE_NUM, NULL, DF_NORM },
  { "GAP",                  OPT_Q_GAP,                VALUE_NUM, NULL, DF_NORM },
  { "MAXPLAYTIME",          OPT_Q_MAXPLAYTIME,        VALUE_NUM, NULL, DF_NORM },
  { "PAUSEEACHSONG",        OPT_Q_PAUSE_EACH_SONG,    VALUE_NUM, convBoolean, DF_NORM },
  { "PLAYANNOUNCE",         OPT_Q_PLAY_ANNOUNCE,      VALUE_NUM, convBoolean, DF_NORM },
  { "PLAY_WHEN_QUEUED",     OPT_Q_PLAY_WHEN_QUEUED,   VALUE_NUM, convBoolean, DF_NORM },
  { "QUEUE_NAME",           OPT_Q_QUEUE_NAME,         VALUE_STR, NULL, DF_NORM },
  { "SHOWQUEUEDANCE",       OPT_Q_SHOW_QUEUE_DANCE,   VALUE_NUM, convBoolean, DF_NORM },
  { "START_WAIT_TIME",      OPT_Q_START_WAIT_TIME,    VALUE_NUM, NULL, DF_NORM },
  { "STOP_AT_TIME",         OPT_Q_STOP_AT_TIME,       VALUE_NUM, NULL, DF_NORM },
  { "XFADE",                OPT_Q_XFADE,              VALUE_NUM, NULL, DF_NORM },
};

/* must be ascii sorted, use LANG=C <editor> bdjopt.c */
static datafilekey_t bdjoptmachinedfkeys [] = {
  { "AUDIOTAG",           OPT_M_AUDIOTAG_INTFC,     VALUE_STR, NULL, DF_NORM },
  { "CONTROLLER",         OPT_M_CONTROLLER_INTFC,   VALUE_STR, NULL, DF_NORM },
  { "DIRITUNESMEDIA",     OPT_M_DIR_ITUNES_MEDIA,   VALUE_STR, NULL, DF_NORM },
  { "DIRMUSIC",           OPT_M_DIR_MUSIC,          VALUE_STR, NULL, DF_NORM },
  { "DIROLDSKIP",         OPT_M_DIR_OLD_SKIP,       VALUE_STR, NULL, DF_NORM },
  { "ITUNESXMLFILE",      OPT_M_ITUNES_XML_FILE,    VALUE_STR, NULL, DF_NORM },
  { "LISTING_FONT",       OPT_M_LISTING_FONT,       VALUE_STR, NULL, DF_NORM },
  { "MQ_FONT",            OPT_M_MQ_FONT,            VALUE_STR, NULL, DF_NORM },
  { "MQ_THEME",           OPT_M_MQ_THEME,           VALUE_STR, NULL, DF_NORM },
  { "PLAYER",             OPT_M_PLAYER_INTFC,       VALUE_STR, NULL, DF_NORM },
  { "PLAYER_ARGS",        OPT_M_PLAYER_ARGS,      VALUE_STR, NULL, DF_NORM },
  { "PLAYER_I_NM",        OPT_M_PLAYER_INTFC_NM,    VALUE_STR, NULL, DF_NORM },
  { "SCALE",              OPT_M_SCALE,              VALUE_NUM, NULL, DF_NORM },
  { "SHUTDOWNSCRIPT",     OPT_M_SHUTDOWN_SCRIPT,    VALUE_STR, NULL, DF_NORM },
  { "STARTUPSCRIPT",      OPT_M_STARTUP_SCRIPT,     VALUE_STR, NULL, DF_NORM },
  { "UI_FONT",            OPT_M_UI_FONT,            VALUE_STR, NULL, DF_NORM },
  { "UI_THEME",           OPT_M_UI_THEME,           VALUE_STR, NULL, DF_NORM },
  { "VOLUME",             OPT_M_VOLUME_INTFC,       VALUE_STR, NULL, DF_NORM },
};

static datafilekey_t bdjoptmachprofdfkeys [] = {
  { "AUDIOSINK",            OPT_MP_AUDIOSINK,             VALUE_STR, NULL, DF_NORM },
  /* the following have been moved into the per-machine settings */
  { "LISTINGFONT",          OPT_MP_LISTING_FONT,          VALUE_STR, NULL, DF_NO_WRITE },
  { "MQFONT",               OPT_MP_MQFONT,                VALUE_STR, NULL, DF_NO_WRITE },
  { "MQ_THEME",             OPT_MP_MQ_THEME,              VALUE_STR, NULL, DF_NO_WRITE },
  { "UIFONT",               OPT_MP_UIFONT,                VALUE_STR, NULL, DF_NO_WRITE },
  { "UI_THEME",             OPT_MP_UI_THEME,              VALUE_STR, NULL, DF_NO_WRITE },
};

static datafilekey_t bdjopturidfkeys [] = {
  { "AUDIOID_ACOUSTID_URI", OPT_URI_AUID_ACOUSTID,        VALUE_STR, NULL, DF_NORM },
  { "AUDIOID_MUSICBRAINZ_URI",  OPT_URI_AUID_MUSICBRAINZ, VALUE_STR, NULL, DF_NORM },
  { "DOWNLOAD_HOST",        OPT_HOST_DOWNLOAD,            VALUE_STR, NULL, DF_NORM },
  { "DOWNLOAD_URI",         OPT_URI_DOWNLOAD,             VALUE_STR, NULL, DF_NORM },
  { "FORUM_HOST",           OPT_HOST_FORUM,               VALUE_STR, NULL, DF_NORM },
  { "FORUM_URI",            OPT_URI_FORUM,                VALUE_STR, NULL, DF_NORM },
  { "HOMEPAGE_URI",         OPT_URI_HOMEPAGE,             VALUE_STR, NULL, DF_NORM },
  { "MOBMQ_HOST",           OPT_HOST_MOBMQ,               VALUE_STR, NULL, DF_NORM },
  { "MOBMQ_HTML_URI",       OPT_URI_MOBMQ_HTML,           VALUE_STR, NULL, DF_NORM },
  { "MOBMQ_PHP_URI",        OPT_URI_MOBMQ_PHP,            VALUE_STR, NULL, DF_NORM },
  { "SUPPORT_HOST",         OPT_HOST_SUPPORT,             VALUE_STR, NULL, DF_NORM },
  { "SUPPORT_URI",          OPT_URI_SUPPORT,              VALUE_STR, NULL, DF_NORM },
  { "TICKET_HOST",          OPT_HOST_TICKET,              VALUE_STR, NULL, DF_NORM },
  { "TICKET_URI",           OPT_URI_TICKET,               VALUE_STR, NULL, DF_NORM },
  { "VERSION_HOST",         OPT_HOST_VERSION,             VALUE_STR, NULL, DF_NORM },
  { "VERSION_URI",          OPT_URI_VERSION,              VALUE_STR, NULL, DF_NORM },
  { "WIKI_HOST",            OPT_HOST_WIKI,                VALUE_STR, NULL, DF_NORM },
  { "WIKI_URI",             OPT_URI_WIKI,                 VALUE_STR, NULL, DF_NORM },
};

static int  bdjoptQueueIndex (nlistidx_t idx, int musiqc);
static void bdjoptCreateNewConfigs (void);

void
bdjoptInit (void)
{
  char          path [MAXPATHLEN];
  const char    *pli;
  const char    *tstr;

  if (bdjopt != NULL) {
    bdjoptCleanup ();
  }

  bdjopt = mdmalloc (sizeof (bdjopt_t));
  bdjopt->currprofile = 0;
  bdjopt->bdjoptList = NULL;
  bdjopt->listallocated = false;

  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    bdjopt->tag [i] = NULL;
    bdjopt->shorttag [i] = NULL;
    bdjopt->fname [i] = NULL;
    bdjopt->df [i] = NULL;
    bdjopt->dfkeys [i] = NULL;
    bdjopt->dfcount [i] = 0;
    bdjopt->distvers [i] = 1;
  }

  bdjopt->dfkeys [OPTTYPE_URI] = bdjopturidfkeys;
  bdjopt->dfkeys [OPTTYPE_GLOBAL] = bdjoptglobaldfkeys;
  bdjopt->dfkeys [OPTTYPE_PROFILE] = bdjoptprofiledfkeys;
  bdjopt->dfkeys [OPTTYPE_QUEUE] = bdjoptqueuedfkeys;
  bdjopt->dfkeys [OPTTYPE_MACHINE] = bdjoptmachinedfkeys;
  bdjopt->dfkeys [OPTTYPE_MACH_PROF] = bdjoptmachprofdfkeys;
  bdjopt->dfcount [OPTTYPE_URI] = sizeof (bdjopturidfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_GLOBAL] = sizeof (bdjoptglobaldfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_PROFILE] = sizeof (bdjoptprofiledfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_QUEUE] = sizeof (bdjoptqueuedfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_MACHINE] = sizeof (bdjoptmachinedfkeys) / sizeof (datafilekey_t);
  bdjopt->dfcount [OPTTYPE_MACH_PROF] = sizeof (bdjoptmachprofdfkeys) / sizeof (datafilekey_t);
  bdjopt->tag [OPTTYPE_URI] = "bdjopt-uri";
  bdjopt->tag [OPTTYPE_GLOBAL] = "bdjopt-g";
  bdjopt->tag [OPTTYPE_PROFILE] = "bdjopt-p";
  bdjopt->tag [OPTTYPE_QUEUE] = "bdjopt-q";
  bdjopt->tag [OPTTYPE_MACHINE] = "bdjopt-m";
  bdjopt->tag [OPTTYPE_MACH_PROF] = "bdjopt-mp";
  bdjopt->shorttag [OPTTYPE_URI] = "uri";
  bdjopt->shorttag [OPTTYPE_GLOBAL] = "g";
  bdjopt->shorttag [OPTTYPE_PROFILE] = "p";
  bdjopt->shorttag [OPTTYPE_QUEUE] = "q";
  bdjopt->shorttag [OPTTYPE_MACHINE] = "m";
  bdjopt->shorttag [OPTTYPE_MACH_PROF] = "mp";

  bdjopt->currprofile = sysvarsGetNum (SVL_PROFILE_IDX);

  /* uri */
  pathbldMakePath (path, sizeof (path), BDJ_URIFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  bdjopt->fname [OPTTYPE_URI] = mdstrdup (path);

  /* global */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  bdjopt->fname [OPTTYPE_GLOBAL] = mdstrdup (path);

  /* profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_PROFILE] = mdstrdup (path);

  /* queue */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_QUEUE] = mdstrdup (path);

  /* per machine */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME);
  bdjopt->fname [OPTTYPE_MACHINE] = mdstrdup (path);

  /* per machine per profile */
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  bdjopt->fname [OPTTYPE_MACH_PROF] = mdstrdup (path);

  if (! fileopFileExists (bdjopt->fname [OPTTYPE_PROFILE])) {
    bdjoptCreateNewConfigs ();
  }

  bdjopt->df [OPTTYPE_URI] = datafileAllocParse (
      bdjopt->tag [OPTTYPE_URI], DFTYPE_KEY_VAL,
      bdjopt->fname [OPTTYPE_URI],
      bdjopt->dfkeys [OPTTYPE_URI], bdjopt->dfcount [OPTTYPE_URI],
      DF_NO_OFFSET, NULL);
  bdjopt->distvers [OPTTYPE_URI] = datafileDistVersion (bdjopt->df [OPTTYPE_URI]);

  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    if (i == OPTTYPE_URI || i == OPTTYPE_QUEUE) {
      continue;
    }
    bdjopt->df [i] = datafileAllocParse (bdjopt->tag [i], DFTYPE_KEY_VAL,
        bdjopt->fname [i], bdjopt->dfkeys [i], bdjopt->dfcount [i],
        DF_NO_OFFSET, bdjopt->df [OPTTYPE_URI]);
    bdjopt->distvers [i] = datafileDistVersion (bdjopt->df [i]);
  }

  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    int   offset;

    /* clear any existing df file, as it gets re-used */
    datafileFree (bdjopt->df [OPTTYPE_QUEUE]);
    bdjopt->df [OPTTYPE_QUEUE] = NULL;
    snprintf (path, sizeof (path), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);
    offset = bdjopt->dfcount [OPTTYPE_QUEUE] * i;
    bdjopt->df [OPTTYPE_QUEUE] = datafileAllocParse (
        bdjopt->tag [OPTTYPE_QUEUE], DFTYPE_KEY_VAL, path,
        bdjopt->dfkeys [OPTTYPE_QUEUE], bdjopt->dfcount [OPTTYPE_QUEUE],
        offset, bdjopt->df [OPTTYPE_URI]);
    bdjopt->distvers [OPTTYPE_QUEUE] = datafileDistVersion (bdjopt->df [OPTTYPE_QUEUE]);
  }

  bdjopt->bdjoptList = datafileGetList (bdjopt->df [OPTTYPE_URI]);
  if (bdjopt->bdjoptList == NULL) {
    bdjopt->bdjoptList = nlistAlloc ("bdjopt-list", LIST_ORDERED, NULL);
    bdjopt->listallocated = true;
  }

  /* 4.0.10 OPT_M_SCALE added */
  if (bdjoptGetNum (OPT_M_SCALE) <= 0) {
    bdjoptSetNum (OPT_M_SCALE, 1);
  }

  /* added 4.3.2.4, make sure it has a default */
  if (nlistGetNum (bdjopt->bdjoptList, OPT_G_DANCESEL_METHOD) < 0) {
    nlistSetNum (bdjopt->bdjoptList, OPT_G_DANCESEL_METHOD,
        DANCESEL_METHOD_WINDOWED);
  }

  /* added 4.4.8, make sure it is set */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_G_OLDORGPATH) == NULL) {
    nlistSetStr (bdjopt->bdjoptList, OPT_G_OLDORGPATH,
        nlistGetStr (bdjopt->bdjoptList, OPT_G_ORGPATH));
  }

  /* added 4.4.8, make sure it is set */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC_NM) == NULL) {
    nlistSetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC_NM, "");
  }

  /* added 4.6.0, make sure it is set */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_P_MQ_INFO_SEP) == NULL) {
    nlistSetStr (bdjopt->bdjoptList, OPT_P_MQ_INFO_SEP, "/");
  }

  /* added 4.8.3, make sure it is set */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_P_PLAYER_UI_SEP) == NULL) {
    nlistSetStr (bdjopt->bdjoptList, OPT_P_PLAYER_UI_SEP, ":");
  }

  /* added 4.10.5, make sure it is set */
  if (nlistGetNum (bdjopt->bdjoptList, OPT_P_SHOW_SPD_CONTROL) < 0) {
    nlistSetNum (bdjopt->bdjoptList, OPT_P_SHOW_SPD_CONTROL, false);
  }

  /* added 4.11.0, make sure it is set */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_P_UI_MARK_TEXT) == NULL) {
    /* left five-eights block */
    nlistSetStr (bdjopt->bdjoptList, OPT_P_UI_MARK_TEXT, "\xe2\x96\x8B");
  }

  /* 4.11.0, added OPT_P_UI_ROWSEL_COL, OPT_P_UI_ROW_HL_COL. */
  /* these do not need defaults, as their defaults are based off of the */
  /* accent color */

  /* 4.11.1 check the OPT_M_PLAYER_INTFC for VLC */
  /* if it is VLC, check the VLC version and switch the player interface */
  /* if necessary */
  /* check for either libplivlc or libplivlc4 */
  pli = nlistGetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC);
  if (pli != NULL && strncmp (pli, "libplivlc", 9) == 0) {
    const char  *vlcpath;

    vlcpath = sysvarsGetStr (SV_PATH_VLC);
    if (! vlccheckdone &&
        (isLinux () || isWindows ()) &&
        (fileopFileExists (vlcpath) || fileopIsDirectory (vlcpath))) {
      char    tbuff [MAXPATHLEN];
      char    *data;

      /* the path and environment variables are already set up */
      pathbldMakePath (tbuff, sizeof (tbuff),
          "vlcversion", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_DIR_EXEC);
      data = osRunProgram (tbuff, NULL);
      if (data != NULL) {
        sysvarsSetNum (SVL_VLC_VERSION, atoi (data));
      }
      dataFree (data);
      vlccheckdone = true;
    }

    /* check for a change in VLC version, and adjust the interface */
    /* as needed */
    /* note that sysvars will have already checked macos */

    if (sysvarsGetNum (SVL_VLC_VERSION) == 3 &&
        strcmp (pli, "libplivlc4") == 0) {
      nlistSetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC, "libplivlc");
      /* CONTEXT: config: integrated VLC 3 */
      nlistSetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC_NM, _("Integrated VLC 3"));
    }
    if (sysvarsGetNum (SVL_VLC_VERSION) == 4 &&
        strcmp (pli, "libplivlc") == 0) {
      nlistSetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC, "libplivlc4");
      /* CONTEXT: config: integrated VLC 4 */
      nlistSetStr (bdjopt->bdjoptList, OPT_M_PLAYER_INTFC_NM, _("Integrated VLC 4"));
    }
  }

  /* added 4.11.3, make sure it is set/converted properly */
  if (nlistGetNum (bdjopt->bdjoptList, OPT_P_MOBMQ_TYPE) < 0) {
    nlistSetNum (bdjopt->bdjoptList, OPT_P_MOBMQ_TYPE, MOBMQ_TYPE_OFF);
    if (nlistGetNum (bdjopt->bdjoptList, OPT_P_MOBILEMARQUEE) > 0) {
      nlistSetNum (bdjopt->bdjoptList, OPT_P_MOBMQ_TYPE, MOBMQ_TYPE_LOCAL);
    }
    /* mobmq-tag and mobmq-key can be left unset */
  }

  /* added for macos ui */
  if (nlistGetStr (bdjopt->bdjoptList, OPT_P_MQ_BG_COL) == NULL) {
    nlistSetStr (bdjopt->bdjoptList, OPT_P_MQ_BG_COL, "#f6f5f4");
  }

  /* added 4.11.7, audio-adjust and restore-original will not be shown */
  if (nlistGetNum (bdjopt->bdjoptList, OPT_G_AUD_ADJ_DISP) < 0) {
    nlistSetNum (bdjopt->bdjoptList, OPT_G_AUD_ADJ_DISP, false);
  }

  /* 4.12.3, moved font and theme to per-machine from per-machine-per-profile */
  tstr = nlistGetStr (bdjopt->bdjoptList, OPT_M_UI_FONT);
  if (tstr == NULL) {
    tstr = nlistGetStr (bdjopt->bdjoptList, OPT_MP_UIFONT);
    nlistSetStr (bdjopt->bdjoptList, OPT_M_UI_FONT, tstr);
  }
  tstr = nlistGetStr (bdjopt->bdjoptList, OPT_M_UI_THEME);
  if (tstr == NULL) {
    tstr = nlistGetStr (bdjopt->bdjoptList, OPT_MP_UI_THEME);
    nlistSetStr (bdjopt->bdjoptList, OPT_M_UI_THEME, tstr);
  }
  tstr = nlistGetStr (bdjopt->bdjoptList, OPT_M_MQ_FONT);
  if (tstr == NULL) {
    tstr = nlistGetStr (bdjopt->bdjoptList, OPT_MP_MQFONT);
    nlistSetStr (bdjopt->bdjoptList, OPT_M_MQ_FONT, tstr);
  }
  tstr = nlistGetStr (bdjopt->bdjoptList, OPT_M_MQ_THEME);
  if (tstr == NULL) {
    tstr = nlistGetStr (bdjopt->bdjoptList, OPT_MP_MQ_THEME);
    nlistSetStr (bdjopt->bdjoptList, OPT_M_MQ_THEME, tstr);
  }
  tstr = nlistGetStr (bdjopt->bdjoptList, OPT_M_LISTING_FONT);
  if (tstr == NULL) {
    tstr = nlistGetStr (bdjopt->bdjoptList, OPT_MP_LISTING_FONT);
    nlistSetStr (bdjopt->bdjoptList, OPT_M_LISTING_FONT, tstr);
  }

  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    int64_t   val;

    /* 4.13.0 prep for crossfade support */
    val = bdjoptGetNumPerQueue (OPT_Q_XFADE, i);
    if (val < 0) {
      bdjoptSetNumPerQueue (OPT_Q_XFADE, 0, i);
    }

    /* 4.12.9 start-wait-time */
    if (bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, i) < 0) {
      bdjoptSetNumPerQueue (OPT_Q_START_WAIT_TIME, 0, i);
    }
  }

  /* added 4.12.6, use-work-movement */
  if (nlistGetNum (bdjopt->bdjoptList, OPT_G_USE_WORK_MOVEMENT) < 0) {
    nlistSetNum (bdjopt->bdjoptList, OPT_G_USE_WORK_MOVEMENT, false);
  }
}

void
bdjoptCleanup (void)
{
  if (bdjopt != NULL) {
    for (int i = 0; i < OPTTYPE_MAX; ++i) {
      datafileFree (bdjopt->df [i]);
      dataFree (bdjopt->fname [i]);
      bdjopt->fname [i] = NULL;
    }
    if (bdjopt->listallocated) {
      nlistFree (bdjopt->bdjoptList);
      bdjopt->listallocated = false;
    }
    mdfree (bdjopt);
  }
  bdjopt = NULL;
}

const char *
bdjoptGetStr (nlistidx_t idx)
{
  const char  *value = NULL;

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

const char *
bdjoptGetStrPerQueue (nlistidx_t idx, int musicq)
{
  nlistidx_t  nidx;
  const char  *value = NULL;

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

  /* special case for the manageui playback queue */
  /* play-when-queued is always on */
  if (musicq == MUSICQ_MNG_PB && idx == OPT_Q_PLAY_WHEN_QUEUED) {
    value = true;
    return value;
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
bdjoptDeleteProfile (void)
{
  char  tbuff [MAXPATHLEN];

  /* data/profileNN */
  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff, DIROP_ALL);
  }
  /* data/<hostname>/profileNN */
  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff, DIROP_ALL);
  }
  /* img/profileNN */
  /* 4.8.1: fix: this must be a relative path */
  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff, DIROP_ALL);
  }
}

void
bdjoptSave (void)
{
  if (bdjopt == NULL) {
    return;
  }

  for (int i = 0; i < OPTTYPE_MAX; ++i) {
    if (i == OPTTYPE_URI || i == OPTTYPE_QUEUE) {
      continue;
    }
    datafileSave (bdjopt->df [i], NULL, bdjopt->bdjoptList,
        DF_NO_OFFSET, bdjopt->distvers [i]);
  }

  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    int   offset;
    char  path [MAXPATHLEN];

    snprintf (path, sizeof (path), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);
    offset = bdjopt->dfcount [OPTTYPE_QUEUE] * i;

    /* do not try to save a queue options file if it does not exist. */
    /* the values will be all mucked up.  this is the job of the updater. */
    if (fileopFileExists (path)) {
      datafileSave (bdjopt->df [OPTTYPE_QUEUE], path, bdjopt->bdjoptList,
          offset, bdjopt->distvers [OPTTYPE_QUEUE]);
    }
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
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
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
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  df = datafileAllocParse (bdjopt->tag [OPTTYPE_PROFILE], DFTYPE_KEY_VAL,
      tbuff, bdjopt->dfkeys [OPTTYPE_PROFILE],
      bdjopt->dfcount [OPTTYPE_PROFILE], DF_NO_OFFSET, NULL);
  dflist = datafileGetList (df);
  pname = mdstrdup (nlistGetStr (dflist, OPT_P_PROFILENAME));
  datafileFree (df);
  return pname;
}

/* the conversion routines are public so that the check suite can test them */

void
bdjoptConvBPM (datafileconv_t *conv)
{
  bdjbpm_t   nbpm = BPM_BPM;
  const char *sval = NULL;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

    if (strcmp (conv->str, "BPM") == 0) {
      nbpm = BPM_BPM;
    }
    if (strcmp (conv->str, "MPM") == 0) {
      nbpm = BPM_MPM;
    }
    conv->num = nbpm;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
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
  int         nclock = TM_CLOCK_LOCAL;
  const char  *sval = NULL;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

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
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
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
bdjoptConvFadeType (datafileconv_t *conv)
{
  bdjfadetype_t   fadetype = FADETYPE_TRIANGLE;
  const char      *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

    fadetype = FADETYPE_TRIANGLE;
    if (strcmp (conv->str, "exponentialsine") == 0) {
      fadetype = FADETYPE_EXPONENTIAL_SINE;
    }
    if (strcmp (conv->str, "halfsine") == 0) {
      fadetype = FADETYPE_HALF_SINE;
    }
    if (strcmp (conv->str, "quadratic") == 0) {
      fadetype = FADETYPE_QUADRATIC;
    }
    if (strcmp (conv->str, "quartersine") == 0) {
      fadetype = FADETYPE_QUARTER_SINE;
    }
    if (strcmp (conv->str, "invertedparabola") == 0) {
      fadetype = FADETYPE_INVERTED_PARABOLA;
    }
    conv->num = fadetype;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    sval = "triangle";
    switch (conv->num) {
      case FADETYPE_EXPONENTIAL_SINE: { sval = "exponentialsine"; break; }
      case FADETYPE_HALF_SINE: { sval = "halfsine"; break; }
      case FADETYPE_INVERTED_PARABOLA: { sval = "invertedparabola"; break; }
      case FADETYPE_QUADRATIC: { sval = "quadratic"; break; }
      case FADETYPE_QUARTER_SINE: { sval = "quartersine"; break; }
      case FADETYPE_TRIANGLE: { sval = "triangle"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptConvWriteTags (datafileconv_t *conv)
{
  bdjwritetags_t  wtag = WRITE_TAGS_NONE;
  const char      *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

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
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    sval = "NONE";
    switch (conv->num) {
      case WRITE_TAGS_ALL: { sval = "ALL"; break; }
      case WRITE_TAGS_BDJ_ONLY: { sval = "BDJ"; break; }
      case WRITE_TAGS_NONE: { sval = "NONE"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptConvMarqueeShow (datafileconv_t *conv)
{
  bdjmarqueeshow_t  mqshow = MARQUEE_SHOW_VISIBLE;
  const char        *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

    mqshow = MARQUEE_SHOW_VISIBLE;
    if (strcmp (conv->str, "off") == 0) {
      mqshow = MARQUEE_SHOW_OFF;
    }
    if (strcmp (conv->str, "minimize") == 0) {
      mqshow = MARQUEE_SHOW_MINIMIZE;
    }
    if (strcmp (conv->str, "visible") == 0) {
      mqshow = MARQUEE_SHOW_VISIBLE;
    }
    conv->num = mqshow;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    sval = "visible";
    switch (conv->num) {
      case MARQUEE_SHOW_OFF: { sval = "off"; break; }
      case MARQUEE_SHOW_MINIMIZE: { sval = "minimize"; break; }
      case MARQUEE_SHOW_VISIBLE: { sval = "visible"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptConvDanceselMethod (datafileconv_t *conv)
{
  int         method = DANCESEL_METHOD_WINDOWED;
  const char  *sval = NULL;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

    if (strcmp (conv->str, "windowed") == 0) {
      method = DANCESEL_METHOD_WINDOWED;
    }
    conv->num = method;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    switch (conv->num) {
      case DANCESEL_METHOD_WINDOWED: { sval = "windowed"; break; }
    }
    conv->str = sval;
  }
}

void
bdjoptConvMobMQType (datafileconv_t *conv)
{
  int         type = MOBMQ_TYPE_LOCAL;
  const char  *sval = NULL;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;

    if (strcmp (conv->str, "off") == 0) {
      type = MOBMQ_TYPE_OFF;
    }
    if (strcmp (conv->str, "local") == 0) {
      type = MOBMQ_TYPE_LOCAL;
    }
    if (strcmp (conv->str, "internet") == 0) {
      type = MOBMQ_TYPE_INTERNET;
    }
    conv->num = type;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    switch (conv->num) {
      case MOBMQ_TYPE_OFF: { sval = "off"; break; }
      case MOBMQ_TYPE_LOCAL: { sval = "local"; break; }
      case MOBMQ_TYPE_INTERNET: { sval = "internet"; break; }
    }
    conv->str = sval;
  }
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
  /* note that special playback queues such as the manageui playback queue */
  /* will get their settings from the main music queue */
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
bdjoptCreateNewConfigs (void)
{
  char      path [MAXPATHLEN];

  if (bdjopt == NULL) {
    return;
  }

  if (bdjopt->fname [OPTTYPE_PROFILE] == NULL) {
    return;
  }

  /* profile */
  sysvarsSetNum (SVL_PROFILE_IDX, 0);
  pathbldMakePath (path, sizeof (path),
      BDJ_CONFIG_BASEFN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_PROFILE_IDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_PROFILE]);

  /* queue */
  for (int i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    char  fpath [MAXPATHLEN];
    char  tpath [MAXPATHLEN];

    sysvarsSetNum (SVL_PROFILE_IDX, 0);
    pathbldMakePath (path, sizeof (path),
        BDJ_CONFIG_BASEFN, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
    snprintf (fpath, sizeof (fpath), "%s.q%d%s",
        path, i, BDJ4_CONFIG_EXT);

    sysvarsSetNum (SVL_PROFILE_IDX, bdjopt->currprofile);
    snprintf (tpath, sizeof (tpath), "%s.q%d%s",
        bdjopt->fname [OPTTYPE_QUEUE], i, BDJ4_CONFIG_EXT);

    filemanipCopy (fpath, tpath);
  }

  /* per machine per profile */
  sysvarsSetNum (SVL_PROFILE_IDX, 0);
  pathbldMakePath (path, sizeof (path), BDJ_CONFIG_BASEFN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  sysvarsSetNum (SVL_PROFILE_IDX, bdjopt->currprofile);
  filemanipCopy (path, bdjopt->fname [OPTTYPE_MACH_PROF]);
}
