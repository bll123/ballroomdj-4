/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
/*
 * the update process to
 * update all bdj4 data files to the latest version
 * also handles some settings for new installations.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>

#include "audiofile.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "datafile.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "instutil.h"
#include "log.h"
#include "localeutil.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"
#include "volreg.h"

#define UPDATER_TMP_FILE "tmpupdater"

enum {
  UPD_NOT_DONE,
  UPD_SKIP,
  UPD_COMPLETE,
};

/* Fix audio file tags: FIX_AF_...  */
/* Fix database       : FIX_DB_...  */
/* Fix playlist       : FIX_PL_...  */
enum {
  UPD_FIRST_VERS,
  UPD_CONVERTED,        // was the original installation converted ?
  /* all fix flags go below upd-converted */

  /* fix-af-tags applies any fixes identified by the parsetags routine. */
  /* These are leftovers and conversions from bdj3 */
  /* and fix-af-tags is only run for converted installations. */
  /* a) variousartists removed */
  /* b) mangled (by mutagen) musizbrainz tag */
  UPD_FIX_AF_TAGS,
  /* 2023-3-13 4.3.0.2 */
  /* fix-db-adddate sets any missing date-added fields in the database */
  UPD_FIX_DB_ADDDATE,
  /* 2023-5-22 4.3.2.4 */
  /* fix-dance-mpm dances.txt bpm/mpm settings are re-written in mpm */
  UPD_FIX_DANCE_MPM,
  /* 2023-5-22 4.3.2.4 */
  /* fix-db-mpm: the database bpm/mpm settings are re-written in mpm */
  UPD_FIX_DB_MPM,
  /* 2023-5-22 4.3.2.4 */
  /* fix-af-mpm: the audio files bpm/mpm settings are re-written in mpm */
  UPD_FIX_AF_MPM,
  /* 2023-5-22 4.3.2.4 */
  /* fix-pl-mpm: the playlist bpm/mpm settings are re-written in mpm */
  UPD_FIX_PL_MPM,
  /* 2023-5-22 4.3.2.4 */
  /* set-mpm changes the configure/general/bpm setting to mpm */
  /* only want to do this once */
  UPD_SET_MPM,
  /* 2023-6-24 4.3.3 */
  /* there are bad disc numbers present in some data, and some were */
  /* converted to negative numbers */
  UPD_FIX_DB_DISCNUM,
  UPD_MAX,
};
enum {
  UPD_FIRST = UPD_CONVERTED + 1,
};


static datafilekey_t upddfkeys[] = {
  { "CONVERTED",        UPD_CONVERTED,      VALUE_NUM, NULL, DF_NORM },
  { "FIRSTVERSION",     UPD_FIRST_VERS,     VALUE_STR, NULL, DF_NORM },
  { "FIX_AF_MPM",       UPD_FIX_AF_MPM,     VALUE_NUM, NULL, DF_NORM },
  { "FIX_AF_TAGS",      UPD_FIX_AF_TAGS,    VALUE_NUM, NULL, DF_NORM },
  { "FIX_DANCE_MPM",    UPD_FIX_DANCE_MPM,  VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_ADD_DATE",  UPD_FIX_DB_ADDDATE, VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_DISCNUM",   UPD_FIX_DB_DISCNUM, VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_MPM",       UPD_FIX_DB_MPM,     VALUE_NUM, NULL, DF_NORM },
  { "FIX_PL_MPM",       UPD_FIX_PL_MPM,     VALUE_NUM, NULL, DF_NORM },
};
enum {
  UPD_DF_COUNT = (sizeof (upddfkeys) / sizeof (datafilekey_t))
};

static void updaterCleanFiles (void);
static void updaterCleanProcess (bool macosonly, bool windowsonly, bool linuxonly, const char *basedir, nlist_t *cleanlist);
static void updaterCleanlistFree (void *trx);
static void updaterCleanRegex (const char *basedir, slist_t *filelist, nlist_t *cleanlist);
static int  updaterGetStatus (nlist_t *updlist, int key);
static void updaterCopyIfNotPresent (const char *fn, const char *ext);
static void updaterCopyVersionCheck (const char *fn, const char *ext, int dftype, int currvers);
static void updaterCopyHTMLVersionCheck (const char *fn, const char *ext, int currvers);
static int  updaterGetMPMValue (song_t *song);

int
main (int argc, char *argv [])
{
  bool        newinstall = false;
  bool        converted = false;
  int         c = 0;
  int         option_index = 0;
  char        *musicdir = NULL;
  char        homemusicdir [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        *tval = NULL;
  bool        isbdj4 = false;
  bool        bdjoptchanged = false;
  int         haveitunes = 0;
  int         statusflags [UPD_MAX];
  int         counters [UPD_MAX];
  bool        processflags [UPD_MAX];
  bool        processaf = false;
  bool        processdb = false;
  bool        forcewritetags = false;
  bool        updlistallocated = false;
  datafile_t  *df;
  nlist_t     *updlist = NULL;
  musicdb_t   *musicdb = NULL;
  long        flags;
  int         origbpmtype;

  static struct option bdj_options [] = {
    { "newinstall", no_argument,        NULL,   'n' },
    { "converted",  no_argument,        NULL,   'c' },
    { "musicdir",   required_argument,  NULL,   'm' },
    { "writetags",  no_argument,        NULL,   'W' },
    { "bdj4updater",no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   'B' },
    /* ignored */
    { "nodetach",   no_argument,        NULL,   0 },
    { "wait",       no_argument,        NULL,   0 },
    { "debugself",  no_argument,        NULL,   0 },
    { "scale",      required_argument,  NULL,   0 },
    { "theme",      required_argument,  NULL,   0 },
    { "origcwd",      required_argument,  NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("updt");
#endif

  optind = 0;
  while ((c = getopt_long_only (argc, argv, "Bncm", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'n': {
        newinstall = true;
        break;
      }
      case 'c': {
        converted = true;
        break;
      }
      case 'W': {
        forcewritetags = true;
        break;
      }
      case 'm': {
        if (optarg) {
          musicdir = mdstrdup (optarg);
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "updater: not started with launcher\n");
    exit (1);
  }

  for (int i = 0; i < UPD_MAX; ++i) {
    processflags [i] = false;
    counters [i] = 0;
  }

  flags = BDJ4_INIT_NO_LOCK | BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "updt", ROUTE_NONE, &flags);
  logSetLevel (LOG_INSTALL, LOG_IMPORTANT | LOG_BASIC | LOG_INFO, "updt");
  logSetLevel (LOG_DBG, LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST, "updt");
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== updater started");

  pathbldMakePath (tbuff, sizeof (tbuff),
      "updater", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  df = datafileAllocParse ("updater", DFTYPE_KEY_VAL, tbuff,
      upddfkeys, UPD_DF_COUNT, DF_NO_OFFSET, NULL);
  updlist = datafileGetList (df);
  if (updlist == NULL) {
    updlist = nlistAlloc ("updater-updlist", LIST_ORDERED, NULL);
    updlistallocated = true;
  }
  listSetVersion (updlist, 3);

  logMsg (LOG_INSTALL, LOG_IMPORTANT, "converted: %d", converted);

  tval = nlistGetStr (updlist, UPD_FIRST_VERS);
  if (tval == NULL || ! *tval) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "first installation");
    nlistSetStr (updlist, UPD_FIRST_VERS, sysvarsGetStr (SV_BDJ4_VERSION));
    nlistSetNum (updlist, UPD_CONVERTED, converted);
  } else {
    converted = nlistGetNum (updlist, UPD_CONVERTED);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "converted (from-file): %d", converted);
  }

  /* Always remove the volreg.txt and flag files on an update.  */
  /* This helps prevents any issues with the volume. */
  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fileopDelete (tbuff);
  volregClearBDJ4Flag ();
  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_BDJ3_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);
  fileopDelete (tbuff);

  logMsg (LOG_INSTALL, LOG_INFO, "cleaned volreg/flag");

  /* always figure out where the home music dir is */
  /* this is used on new installs to set the music dir */
  /* also needed to check for the itunes dir every time */
  /* 4.3.3: the installer now sets the music dir, do not reset it */
  strlcpy (homemusicdir, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (homemusicdir));
  logMsg (LOG_INSTALL, LOG_INFO, "homemusicdir: %s", homemusicdir);

  for (int i = UPD_FIRST; i < UPD_MAX; ++i) {
    statusflags [i] = updaterGetStatus (updlist, i);
  }

  if (newinstall) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "new install or re-install");
    tval = bdjoptGetStr (OPT_M_VOLUME_INTFC);
    if (tval == NULL || ! *tval || strcmp (tval, "libvolnull") == 0) {
      if (isWindows ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolwin");
      }
      if (isMacOS ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolmac");
        /* logging on mac os is really slow */
        bdjoptSetNum (OPT_G_DEBUGLVL, 0);
      }
      if (isLinux ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolpa");
      }
      bdjoptchanged = true;
    }

    tval = bdjoptGetStr (OPT_M_STARTUPSCRIPT);
    if (isLinux () && (tval == NULL || ! *tval)) {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "scripts/linux/bdjstartup", ".sh", PATHBLD_MP_DIR_MAIN);
      bdjoptSetStr (OPT_M_STARTUPSCRIPT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "scripts/linux/bdjshutdown", ".sh", PATHBLD_MP_DIR_MAIN);
      bdjoptSetStr (OPT_M_SHUTDOWNSCRIPT, tbuff);
      bdjoptchanged = true;
    }

    tval = bdjoptGetStr (OPT_M_DIR_MUSIC);
    if (tval == NULL || ! *tval) {
      bdjoptSetStr (OPT_M_DIR_MUSIC, homemusicdir);
      bdjoptchanged = true;
    }

    if (! converted) {
      const char  *uifont = NULL;

      logMsg (LOG_INSTALL, LOG_IMPORTANT, "not converted");
      bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, false);
      nlistSetNum (updlist, UPD_FIX_AF_TAGS, statusflags [UPD_FIX_AF_TAGS]);

      /* need some decent default fonts for windows and macos */
      if (isWindows () || isMacOS ()) {
        uifont = "Arial Regular 11";
        if (isMacOS ()) {
          uifont = "Arial Regular 17";
        }
        bdjoptSetStr (OPT_MP_UIFONT, uifont);

        /* windows does not have a narrow font pre-installed */
        uifont = "Arial Regular 11";
        if (isMacOS ()) {
          uifont = "Arial Narrow Regular 16";
        }
        bdjoptSetStr (OPT_MP_MQFONT, uifont);

        uifont = "Arial Regular 10";
        if (isMacOS ()) {
          uifont = "Arial Regular 16";
        }
        bdjoptSetStr (OPT_MP_LISTING_FONT, uifont);
      }

      /* new install and no conversion, so none of these issues are present */
      for (int i = UPD_FIRST; i < UPD_MAX; ++i) {
        statusflags [i] = UPD_SKIP;
      }
    }

    logMsg (LOG_INSTALL, LOG_INFO, "finish new install");
  }

  /* always check and see if itunes exists, unless a conversion was run */
  /* support for old versions of itunes is minimal */

  if (! converted) {
    tval = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
    if (tval == NULL || ! *tval) {
      snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
          homemusicdir, ITUNES_NAME, ITUNES_MEDIA_NAME);
      if (fileopIsDirectory (tbuff)) {
        bdjoptSetStr (OPT_M_DIR_ITUNES_MEDIA, tbuff);
        bdjoptchanged = true;
        ++haveitunes;
      }
      snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
          homemusicdir, ITUNES_NAME, ITUNES_XML_NAME);
      if (fileopFileExists (tbuff)) {
        bdjoptSetStr (OPT_M_ITUNES_XML_FILE, tbuff);
        bdjoptchanged = true;
        ++haveitunes;
      } else {
        /* this is an ancient itunes name, no idea if it needs to be supported */
        snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
            homemusicdir, ITUNES_NAME, "iTunes Library.xml");
        if (fileopFileExists (tbuff)) {
          bdjoptSetStr (OPT_M_ITUNES_XML_FILE, tbuff);
          bdjoptchanged = true;
          ++haveitunes;
        }
      }
    } /* if itunes dir is not set */
  } /* if not converted */

  /* a new installation, and the itunes folder and xml file */
  /* have been found. */
  if (newinstall && haveitunes == 2) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "have itunes");
    /* set the music dir to the itunes media folder */
    bdjoptSetStr (OPT_M_DIR_MUSIC, bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA));
    /* set the organization path to the itunes standard */
    /* album-artist / album / disc-tracknum0 title */
    bdjoptSetStr (OPT_G_ORGPATH,
        "{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0% }{%TITLE%}");
    bdjoptchanged = true;
  }

  if (musicdir != NULL) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "musicdir: %s", musicdir);
    bdjoptSetStr (OPT_M_DIR_MUSIC, musicdir);
  }

  {
    /* 4.1.0 change name of audiotag dylib (prep) */
    tval = bdjoptGetStr (OPT_M_AUDIOTAG_INTFC);
    if (tval != NULL && strcmp (tval, "libaudiotagmutagen") == 0) {
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.1.0 : chg name of audiotag dylib");
      bdjoptSetStr (OPT_M_AUDIOTAG_INTFC, "libatimutagen");
      bdjoptchanged = true;
    }
  }

  {
    const char  *pfx = "macOS-";
    size_t      len = strlen (pfx);
    char        tbuff [MAXPATHLEN];

    /* 4.3.3 change name of macos theme  */
    tval = bdjoptGetStr (OPT_MP_UI_THEME);
    if (tval != NULL && strncmp (tval, pfx, len) == 0) {
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.3.3 : chg name of ui theme");
      snprintf (tbuff, sizeof (tbuff), "%s-solid", tval + len);
      bdjoptSetStr (OPT_MP_UI_THEME, tbuff);
      bdjoptchanged = true;
      snprintf (tbuff, sizeof (tbuff), "%s/.themes/macOS-Mojave-dark",
          sysvarsGetStr (SV_HOME));
      fileopDelete (tbuff);
      snprintf (tbuff, sizeof (tbuff), "%s/.themes/macOS-Mojave-light",
          sysvarsGetStr (SV_HOME));
      fileopDelete (tbuff);
    }
  }

  {
    origbpmtype = bdjoptGetNum (OPT_G_BPM);

    if (statusflags [UPD_SET_MPM] == UPD_NOT_DONE) {
      /* 4.3.2.4 : change BPM to default to MPM */
      bdjoptSetNum (OPT_G_BPM, BPM_MPM);
      bdjoptchanged = true;
      nlistSetNum (updlist, UPD_SET_MPM, UPD_COMPLETE);
    }

    /* 4.3.2.4 : change dancesel method to windowed */
    bdjoptSetNum (OPT_G_DANCESEL_METHOD, DANCESEL_METHOD_WINDOWED);
    bdjoptchanged = true;
  }

  if (bdjoptchanged) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "save bdjopt");
    bdjoptSave ();
  }

  /* clean up any old files and directories */

  logMsg (LOG_INSTALL, LOG_INFO, "start clean");

  updaterCleanFiles ();

  logMsg (LOG_INSTALL, LOG_INFO, "end clean");

  /* datafile updates */

  {
    /* 4.0.5 2023-1-4 itunes-fields */
    /*   'lastupdate' name removed completely (not needed) */
    /*   as itunes has not been implemented yet, it is safe to completely */
    /*   overwrite version 1. */
    updaterCopyVersionCheck (ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, DFTYPE_LIST, 2);
  }

  {
    /* 4.2.0 2023-3-5 autoselection.txt */
    /* updated values (version 3) */
    /* 4.3.2.4 2023-5-26 */
    /* added windowed values (version 4) */
    updaterCopyVersionCheck (AUTOSEL_FN, BDJ4_CONFIG_EXT, DFTYPE_KEY_VAL, 4);
  }

  {
    /* 4.1.0 2023-1-5 audioadjust.txt */
    updaterCopyIfNotPresent (AUDIOADJ_FN, BDJ4_CONFIG_EXT);
    /* 4.3.0.4 2023-4-4 (version number bump) audioadjust.txt */
    updaterCopyVersionCheck (AUDIOADJ_FN, BDJ4_CONFIG_EXT, DFTYPE_KEY_VAL, 4);
  }

  {
    /* 4.0.10 2023-1-29 gtk-static.css */
    /*    This is a new file; simply check and see if it does not exist. */
    updaterCopyIfNotPresent ("gtk-static", BDJ4_CSS_EXT);
  }

  {
    /* 4.1.0 2023-2-9 nl renamed, just re-install if not there */
    updaterCopyIfNotPresent (_("QueueDance"), BDJ4_PLAYLIST_EXT);
    updaterCopyIfNotPresent (_("QueueDance"), BDJ4_PL_DANCE_EXT);
  }

  {
    /* 2023-1-21 : The QueueDance playlist had bad data in it. */
    /* 2023-1-21 : The StandardRounds playlist had bad data in it. */
    /* 2023-4-17 : QueueDance updated so that the mm filter would */
    /*             display the queuedance playlist properly. */
    /* 2023-5-23 : 4.3.2.4 */
    /*             Updated internal key names */
    updaterCopyVersionCheck (_("QueueDance"), BDJ4_PL_DANCE_EXT, DFTYPE_KEY_VAL, 4);
    updaterCopyVersionCheck (_("standardrounds"), BDJ4_PL_DANCE_EXT, DFTYPE_KEY_VAL, 3);
    updaterCopyVersionCheck (_("automatic"), BDJ4_PL_DANCE_EXT, DFTYPE_KEY_VAL, 2);
  }

  {
    /* 4.3.2: 2023-4-27 : Mobile marquee HTML file has minor updates. */
    updaterCopyHTMLVersionCheck ("mobilemq", BDJ4_HTML_EXT, 2);
  }

  /* The datafiles must be loaded for the MPM update process */

  if (bdjvarsdfloadInit () < 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  logMsg (LOG_INSTALL, LOG_INFO, "loaded data files A");

  /* 4.3.2.4 */
  /* this requires that the data files be loaded */
  if (statusflags [UPD_FIX_DANCE_MPM] == UPD_NOT_DONE) {
    dance_t     *odances;
    dance_t     *ndances = NULL;
    slist_t     *ndancelist = NULL;
    ilistidx_t  oiteridx;
    ilistidx_t  didx, ndidx;
    char        from [MAXPATHLEN];
    char        tbuff [MAXPATHLEN];

    logMsg (LOG_INSTALL, LOG_INFO, "-- 4.3.2.4 : update dance mpm");

    /* need a copy of the new dances.txt file */
    snprintf (from, sizeof (from), "%s%s", DANCE_FN, BDJ4_CONFIG_EXT);
    snprintf (tbuff, sizeof (tbuff), "%s%s", UPDATER_TMP_FILE, BDJ4_CONFIG_EXT);
    templateFileCopy (from, tbuff);

    pathbldMakePath (tbuff, sizeof (tbuff),
        UPDATER_TMP_FILE, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
    ndances = danceAlloc (tbuff);
    ndancelist = danceGetDanceList (ndances);

    /* convert the bpm values */
    odances = bdjvarsdfGet (BDJVDF_DANCES);
    danceStartIterator (odances, &oiteridx);
    while ((didx = danceIterate (odances, &oiteridx)) >= 0) {
      int   olowmpm, ohighmpm, otimesig, otype;
      int   nlowmpm, nhighmpm, ntimesig, ntype;

      if (origbpmtype == BPM_BPM) {
        olowmpm = danceGetNum (odances, didx, DANCE_MPM_LOW);
        ohighmpm = danceGetNum (odances, didx, DANCE_MPM_HIGH);
        otimesig = danceGetNum (odances, didx, DANCE_TIMESIG);
        otype = danceGetNum (odances, didx, DANCE_TYPE);

        ndidx = slistGetNum (ndancelist, danceGetStr (odances, didx, DANCE_DANCE));
        if (ndidx >= 0) {
          /* if the dance exists in the new dances.txt, copy the data over */
          nlowmpm = danceGetNum (ndances, ndidx, DANCE_MPM_LOW);
          nhighmpm = danceGetNum (ndances, ndidx, DANCE_MPM_HIGH);
          /* time signature was changed for tango, argentine tango and merengue */
          ntimesig = danceGetNum (ndances, ndidx, DANCE_TIMESIG);
          /* type was fixed for en_US, changed for some dances */
          ntype = danceGetNum (ndances, ndidx, DANCE_TYPE);
        } else {
          /* otherwise convert the data that is present */
          nlowmpm = olowmpm;
          nhighmpm = ohighmpm;
          ntimesig = otimesig;
          ntype = otype;
          /* dance does not exist, convert the BPM value */
          nhighmpm = danceConvertBPMtoMPM (didx, nhighmpm, DANCE_FORCE_CONV);
          nlowmpm = danceConvertBPMtoMPM (didx, nlowmpm, DANCE_FORCE_CONV);
        }

        /* this is a bit difficult if there is a user-entered dance */
        /* and no prior known dances have been processed */
        if (ohighmpm > 0 && ndidx >= 0 && ohighmpm < nhighmpm + 10) {
          /* handle case where label is bpm, values are mpm */
          /* keep the user's settings */
          nlowmpm = olowmpm;
          nhighmpm = ohighmpm;
          ntimesig = otimesig;
          ntype = otype;
        }

        danceSetNum (odances, didx, DANCE_MPM_LOW, nlowmpm);
        danceSetNum (odances, didx, DANCE_MPM_HIGH, nhighmpm);
        danceSetNum (odances, didx, DANCE_TIMESIG, ntimesig);
        danceSetNum (odances, didx, DANCE_TYPE, ntype);
      } else {
        /* already set to mpm */
        statusflags [UPD_FIX_PL_MPM] = UPD_SKIP;
        statusflags [UPD_FIX_DB_MPM] = UPD_SKIP;
        statusflags [UPD_FIX_AF_MPM] = UPD_SKIP;
      }
    }

    /* 4.3.2.4: 2023-5-22 : Update dances.txt to dist version 2 */

    danceSave (odances, NULL, 2);
    danceFree (ndances);
    fileopDelete (tbuff);
    if (statusflags [UPD_FIX_DANCE_MPM] == UPD_NOT_DONE) {
      nlistSetNum (updlist, UPD_FIX_DANCE_MPM, UPD_COMPLETE);
    }
    logMsg (LOG_INSTALL, LOG_INFO, "-- 4.3.2.4 : update dance mpm complete");
  }

  /* now re-load the data files */

  bdjvarsdfloadCleanup ();

  if (bdjvarsdfloadInit () < 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  logMsg (LOG_INSTALL, LOG_INFO, "loaded data files B");

  /* playlist updates */

  {
    slist_t         *pllist;
    slistidx_t      pliteridx;
    const char      *plnm;
    dance_t         *dances = NULL;

    dances = bdjvarsdfGet (BDJVDF_DANCES);

    pllist = playlistGetPlaylistList (PL_LIST_ALL, NULL);
    slistStartIterator (pllist, &pliteridx);
    while ((plnm = slistIterateKey (pllist, &pliteridx)) != NULL) {
      if (statusflags [UPD_FIX_PL_MPM] == UPD_NOT_DONE) {
        playlist_t    *pl;
        ilistidx_t    iteridx;
        ilistidx_t    didx;
        bool          doplsave = false;

        pl = playlistLoad (plnm, NULL);

        danceStartIterator (dances, &iteridx);
        while ((didx = danceIterate (dances, &iteridx)) >= 0) {
          int     tval;

          tval = playlistGetDanceNum (pl, didx, PLDANCE_MPM_LOW);
          if (tval > 0) {
            tval = danceConvertBPMtoMPM (didx, tval, DANCE_FORCE_CONV);
            playlistSetDanceNum (pl, didx, PLDANCE_MPM_LOW, tval);
            doplsave = true;
          }
          tval = playlistGetDanceNum (pl, didx, PLDANCE_MPM_HIGH);
          if (tval > 0) {
            tval = danceConvertBPMtoMPM (didx, tval, DANCE_FORCE_CONV);
            playlistSetDanceNum (pl, didx, PLDANCE_MPM_HIGH, tval);
            doplsave = true;
          }
        }

        if (doplsave) {
          logMsg (LOG_INSTALL, LOG_INFO, "-- 4.3.2.4 : update pl %s", plnm);
          playlistSave (pl, NULL);
          counters [UPD_FIX_PL_MPM] += 1;
        }
        playlistFree (pl);
      }
    }
    slistFree (pllist);

    if (statusflags [UPD_FIX_PL_MPM] == UPD_NOT_DONE) {
      logMsg (LOG_INSTALL, LOG_INFO, "-- 4.3.2.4 : update pl mpm complete");
      nlistSetNum (updlist, UPD_FIX_PL_MPM, UPD_COMPLETE);
    }
  }

  /* All database processing must be done last after the updates to the */
  /* datafiles are done. */

  /* audio file conversions */

  /* fix audio file tags check; this is only run if: */
  /* - it has never been run */
  /* - not dev release */
  /* - write-tag is on */
  /* - bdj3-compat-tags is off */

  processflags [UPD_FIX_AF_TAGS] =
      converted &&
      statusflags [UPD_FIX_AF_TAGS] == UPD_NOT_DONE &&
      strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") != 0 &&
      bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE &&
      bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS) == false;
  if (processflags [UPD_FIX_AF_TAGS]) {
    logMsg (LOG_INSTALL, LOG_INFO, "-- fix af tags");
    processaf = true;
  } else {
    if (statusflags [UPD_FIX_AF_TAGS] == UPD_NOT_DONE) {
      if (strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") != 0 &&
          bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS) == false) {
        nlistSetNum (updlist, UPD_FIX_AF_TAGS, UPD_SKIP);
      }
    }
  }

  processflags [UPD_FIX_AF_MPM] =
      statusflags [UPD_FIX_AF_MPM] == UPD_NOT_DONE &&
      (forcewritetags || bdjoptGetNum (OPT_G_WRITETAGS) == WRITE_TAGS_ALL);
  if (processflags [UPD_FIX_AF_MPM]) {
    logMsg (LOG_INSTALL, LOG_INFO, "-- 4.3.2.4 : fix af mpm");
    processaf = true;
  } else {
    if (statusflags [UPD_FIX_AF_MPM] == UPD_NOT_DONE) {
      nlistSetNum (updlist, UPD_FIX_AF_MPM, UPD_SKIP);
    }
  }

  if (statusflags [UPD_FIX_DB_ADDDATE] == UPD_NOT_DONE) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.3.0.2 : process db : fix db add date");
    processflags [UPD_FIX_DB_ADDDATE] = true;
    processdb = true;
  }
  if (statusflags [UPD_FIX_DB_MPM] == UPD_NOT_DONE) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.3.2.4 : process db : fix db mpm");
    processflags [UPD_FIX_DB_MPM] = true;
    processdb = true;
  }
  if (statusflags [UPD_FIX_DB_DISCNUM] == UPD_NOT_DONE) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.3.3 : process db : fix db discnum");
    processflags [UPD_FIX_DB_DISCNUM] = true;
    processdb = true;
  }

  if (processaf || processdb) {
    mstime_t    dbmt;

    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
    mstimestart (&dbmt);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Database read: started");
    musicdb = dbOpen (tbuff);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Database read: %d items in %" PRId64 " ms", dbCount(musicdb), (int64_t) mstimeend (&dbmt));
  }

  if (processaf) {
    slistidx_t  dbiteridx;
    song_t      *song;
    dbidx_t     dbidx;
    dance_t     *dances;

    logMsg (LOG_INSTALL, LOG_IMPORTANT, "processing audio files");
    dances = bdjvarsdfGet (BDJVDF_DANCES);

    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      char        *ffn;
      void        *data;
      const char  *tkey;
      int         rewrite;
      bool        updmpm = false;
      bool        process = false;
      int         newbpmval = LIST_VALUE_INVALID;
      slist_t     *taglist;
      slist_t     *newtaglist;
      slistidx_t  siteridx;

      ffn = songutilFullFileName (songGetStr (song, TAG_FILE));

      if (processflags [UPD_FIX_AF_TAGS]) {
        pathinfo_t    *pi;

        pi = pathInfo (ffn);
        if (pathInfoExtCheck (pi, ".mp3") ||
            pathInfoExtCheck (pi, ".flac") ||
            pathInfoExtCheck (pi, ".ogg") ||
            pathInfoExtCheck (pi, ".m4a") ||
            pathInfoExtCheck (pi, ".opus")) {
          process = true;
        }
        pathInfoFree (pi);
      }

      if (processflags [UPD_FIX_AF_MPM]) {
        int     obpmval;
        int     didx;
        int     thighmpm;

        obpmval = songGetNum (song, TAG_BPM);
        didx = songGetNum (song, TAG_DANCE);
        thighmpm = danceGetNum (dances, didx, DANCE_MPM_HIGH);
        newbpmval = obpmval;
        if (didx >= 0 && obpmval > 0 && obpmval > thighmpm + 10) {
          newbpmval = updaterGetMPMValue (song);
          if (newbpmval > 0) {
            counters [UPD_FIX_AF_MPM] += 1;
            process = true;
            updmpm = true;
          }
        }
      }

      if (! process) {
        mdfree (ffn);
        continue;
      }

      data = audiotagReadTags (ffn);
      taglist = audiotagParseData (ffn, data, &rewrite);

      newtaglist = slistAlloc ("new-taglist", LIST_UNORDERED, NULL);
      slistSetSize (newtaglist, slistGetCount (taglist));
      slistStartIterator (taglist, &siteridx);
      while ((tkey = slistIterateKey (taglist, &siteridx)) != NULL) {
        slistSetStr (newtaglist, tkey, slistGetStr (taglist, tkey));
      }
      slistSort (newtaglist);

      if (updmpm) {
        char    tmp [40];

        snprintf (tmp, sizeof (tmp), "%d", newbpmval);
        slistSetStr (newtaglist, tagdefs [TAG_BPM].tag, tmp);
      }

      if ((processflags [UPD_FIX_AF_TAGS] &&
          rewrite != AF_REWRITE_NONE) ||
          (processflags [UPD_FIX_AF_MPM] && updmpm)) {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "write audio tags: %d %s", dbidx, ffn);
        audiotagWriteTags (ffn, taglist, newtaglist, rewrite, AT_KEEP_MOD_TIME);
      }

      dataFree (data);
      slistFree (taglist);
      slistFree (newtaglist);
      mdfree (ffn);
    }
  }

  if (processdb) {
    slistidx_t  dbiteridx;
    song_t      *song;
    dbidx_t     dbidx;

    dbStartBatch (musicdb);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "processing database");
    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      bool      dowrite = false;

      if (processflags [UPD_FIX_DB_ADDDATE]) {
        char      *ffn;

        /* 2023-3-13 : the database add date could be missing due to */
        /* bugs in restore-original and restore audio file data */
        if (songGetStr (song, TAG_DBADDDATE) == NULL) {
          time_t    ctime;

          ffn = songutilFullFileName (songGetStr (song, TAG_FILE));
          ctime = fileopCreateTime (ffn);
          tmutilToDate (ctime * 1000, tbuff, sizeof (tbuff));
          songSetStr (song, TAG_DBADDDATE, tbuff);
          dowrite = true;
          counters [UPD_FIX_DB_ADDDATE] += 1;
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "fix dbadddate: %s", ffn);
          mdfree (ffn);
        }
      }

      if (processflags [UPD_FIX_DB_MPM]) {
        int   newbpmval;

        newbpmval = updaterGetMPMValue (song);
        if (newbpmval > 0) {
          songSetNum (song, TAG_BPM, newbpmval);
          dowrite = true;
          counters [UPD_FIX_DB_MPM] += 1;
        }
      }

      if (processflags [UPD_FIX_DB_DISCNUM]) {
        int   val;

        val = songGetNum (song, TAG_DISCNUMBER);
        if (val < -1 || val > 5000) {
          songSetNum (song, TAG_DISCNUMBER, 1);
          dowrite = true;
          counters [UPD_FIX_DB_DISCNUM] += 1;
        }
      }

      if (dowrite) {
        dbWriteSong (musicdb, song);
      }
    }
    dbEndBatch (musicdb);
  }

  if (processaf || processdb) {
    if (processflags [UPD_FIX_AF_TAGS]) {
      nlistSetNum (updlist, UPD_FIX_AF_TAGS, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_AF_MPM]) {
      nlistSetNum (updlist, UPD_FIX_AF_MPM, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_DB_ADDDATE]) {
      nlistSetNum (updlist, UPD_FIX_DB_ADDDATE, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_DB_MPM]) {
      nlistSetNum (updlist, UPD_FIX_DB_MPM, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_DB_DISCNUM]) {
      nlistSetNum (updlist, UPD_FIX_DB_DISCNUM, UPD_COMPLETE);
    }
    dbClose (musicdb);
  }

  if (counters [UPD_FIX_PL_MPM] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: pl-mpm: %d", counters [UPD_FIX_PL_MPM]);
  }
  if (counters [UPD_FIX_AF_MPM] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: af-mpm: %d", counters [UPD_FIX_AF_MPM]);
  }
  if (counters [UPD_FIX_DB_ADDDATE] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-adddate: %d", counters [UPD_FIX_DB_ADDDATE]);
  }
  if (counters [UPD_FIX_DB_MPM] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-mpm: %d", counters [UPD_FIX_DB_MPM]);
  }
  if (counters [UPD_FIX_DB_DISCNUM] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-discnum: %d", counters [UPD_FIX_DB_DISCNUM]);
  }

  datafileSave (df, NULL, updlist, DF_NO_OFFSET, datafileDistVersion (df));
  datafileFree (df);
  if (updlistallocated) {
    slistFree (updlist);
  }

  bdj4shutdown (ROUTE_NONE, NULL);
  bdjoptCleanup ();
  dataFree (musicdir);
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static void
updaterCleanFiles (void)
{
  FILE    *fh;
  char    pattern [MAXPATHLEN];
  char    fullpattern [MAXPATHLEN];
  char    fname [MAXPATHLEN];
  char    *basedir = NULL;
  int     count;
  nlist_t *cleanlist;
  bool    macosonly = false;
  bool    linuxonly = false;
  bool    windowsonly = false;
  bool    processflag = false;

  basedir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);

  /* look for development directories and do not run if any are found */
  /* this works for the relative case */
  if (fileopIsDirectory ("dev") ||
      fileopIsDirectory ("test-music-orig") ||
      fileopIsDirectory ("packages")) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "looks like the dev directory-a: skip");
    return;
  }

  /* more checks for development directories */
  snprintf (fname, sizeof (fname), "%s/dev", basedir);
  if (fileopIsDirectory (fname)) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "looks like the dev directory-b: skip");
    return;
  }
  snprintf (fname, sizeof (fname), "%s/test-music-orig", basedir);
  if (fileopIsDirectory (fname)) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "looks like the dev directory-c: skip");
    return;
  }
  snprintf (fname, sizeof (fname), "%s/packages", basedir);
  if (fileopIsDirectory (fname)) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "looks like the dev directory-d: skip");
    return;
  }

  pathbldMakePath (fname, sizeof (fname),
      "cleanuplist", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);

  cleanlist = nlistAlloc ("clean-regex", LIST_UNORDERED, updaterCleanlistFree);

  count = 0;
  fh = fileopOpen (fname, "r");
  if (fh == NULL) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "unable to open %s", fname);
    return;
  }

  while (fgets (pattern, sizeof (pattern), fh) != NULL) {
    bdjregex_t  *rx;

    if (*pattern == '#') {
      continue;
    }

    stringTrim (pattern);
    stringTrimChar (pattern, '/');

    if (*pattern == '\0') {
      continue;
    }
    // logMsg (LOG_INSTALL, LOG_IMPORTANT, "pattern: %s", pattern); //

    /* on any change of directory or flag, process what has been queued */
    if (strcmp (pattern, "::macosonly") == 0 ||
        strcmp (pattern, "::linuxonly") == 0 ||
        strcmp (pattern, "::windowsonly") == 0 ||
        strcmp (pattern, "::allos") == 0 ||
        strcmp (pattern, "::datatopdir") == 0) {
      processflag = true;
    }

    if (processflag) {
      updaterCleanProcess (macosonly, windowsonly, linuxonly, basedir, cleanlist);
      nlistFree (cleanlist);
      cleanlist = nlistAlloc ("clean-regex", LIST_UNORDERED, updaterCleanlistFree);
      processflag = false;
    }

    if (strcmp (pattern, "::macosonly") == 0) {
      macosonly = true;
      linuxonly = false;
      windowsonly = false;
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- macos only");
    }
    if (strcmp (pattern, "::linuxonly") == 0) {
      macosonly = false;
      linuxonly = true;
      windowsonly = false;
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- linux only");
    }
    if (strcmp (pattern, "::windowsonly") == 0) {
      macosonly = false;
      linuxonly = false;
      windowsonly = true;
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- windows only");
    }
    if (strcmp (pattern, "::allos") == 0) {
      macosonly = false;
      linuxonly = false;
      windowsonly = false;
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- all os");
    }
    if (strcmp (pattern, "::datatopdir") == 0) {
      basedir = sysvarsGetStr (SV_BDJ4_DIR_DATATOP);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- data-top-dir");
    }

    if (strcmp (pattern, "::macosonly") == 0 ||
        strcmp (pattern, "::linuxonly") == 0 ||
        strcmp (pattern, "::windowsonly") == 0 ||
        strcmp (pattern, "::allos") == 0 ||
        strcmp (pattern, "::datatopdir") == 0) {
      continue;
    }

    snprintf (fullpattern, sizeof (fullpattern), "%s/%s", basedir, pattern);
    // logMsg (LOG_INSTALL, LOG_IMPORTANT, "clean %s", fullpattern); //
    rx = regexInit (fullpattern);
    nlistSetData (cleanlist, count, rx);
    ++count;
  }
  fclose (fh);

  /* process what has been queued */
  updaterCleanProcess (macosonly, windowsonly, linuxonly, basedir, cleanlist);

  nlistFree (cleanlist);
  cleanlist = NULL;
}

static void
updaterCleanProcess (bool macosonly, bool windowsonly, bool linuxonly,
    const char *basedir, nlist_t *cleanlist)
{
  bool    cleanflag = false;

  cleanflag =
      (macosonly == false && linuxonly == false && windowsonly == false) ||
      (macosonly == true && isMacOS ()) ||
      (linuxonly == true && isLinux ()) ||
      (windowsonly == true && isWindows ());
  if (cleanflag) {
    slist_t   *filelist;

    filelist = dirlistRecursiveDirList (basedir,
        DIRLIST_FILES | DIRLIST_DIRS | DIRLIST_LINKS);
    updaterCleanRegex (basedir, filelist, cleanlist);
    slistFree (filelist);
  }
}


static void
updaterCleanlistFree (void *trx)
{
  bdjregex_t  *rx = trx;

  regexFree (rx);
}

static void
updaterCleanRegex (const char *basedir, slist_t *filelist, nlist_t *cleanlist)
{
  slistidx_t  fiteridx;
  nlistidx_t  cliteridx;
  nlistidx_t  key;
  char        *fn;
  bdjregex_t  *rx;

  slistStartIterator (filelist, &fiteridx);
  while ((fn = slistIterateKey (filelist, &fiteridx)) != NULL) {
    nlistStartIterator (cleanlist, &cliteridx);
    while ((key = nlistIterateKey (cleanlist, &cliteridx)) >= 0) {
      rx = nlistGetData (cleanlist, key);
      // logMsg (LOG_INSTALL, LOG_IMPORTANT, "  check %s", fn); //
      if (regexMatch (rx, fn)) {
        // logMsg (LOG_INSTALL, LOG_IMPORTANT, "  match %s", fn); //
        if (osIsLink (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete link %s", fn);
          fileopDelete (fn);
        } else if (fileopIsDirectory (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete dir %s", fn);
          diropDeleteDir (fn);
        } else if (fileopFileExists (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete %s", fn);
          fileopDelete (fn);
        }
      }
    }
  }
}

static int
updaterGetStatus (nlist_t *updlist, int key)
{
  int   value;

  value = nlistGetNum (updlist, key);
  if (value < 0) {
    value = UPD_NOT_DONE;
    nlistSetNum (updlist, key, value);
  }

  return value;
}

static void
updaterCopyIfNotPresent (const char *fn, const char *ext)
{
  char    tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff), fn, ext, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (tbuff)) {
    snprintf (tbuff, sizeof (tbuff), "%s%s", fn, ext);
    templateFileCopy (tbuff, tbuff);
    logMsg (LOG_INSTALL, LOG_INFO, "%s%s installed", fn, ext);
  }
}

static void
updaterCopyVersionCheck (const char *fn, const char *ext,
    int dftype, int currvers)
{
  datafile_t  *tmpdf;
  int         version;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff), fn, ext, PATHBLD_MP_DREL_DATA);
  tmpdf = datafileAllocParse (fn, dftype, tbuff, NULL, 0, DF_NO_OFFSET, NULL);
  version = datafileDistVersion (tmpdf);
  logMsg (LOG_INSTALL, LOG_INFO, "version check %s : %d < %d", fn, version, currvers);
  if (version < currvers) {
    char  tmp [MAXPATHLEN];

    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateFileCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
  }
  datafileFree (tmpdf);
}

static void
updaterCopyHTMLVersionCheck (const char *fn, const char *ext,
    int currvers)
{
  int         version;
  char        from [MAXPATHLEN];
  char        tmp [MAXPATHLEN];
  FILE        *fh;

  pathbldMakePath (from, sizeof (from), fn, ext, PATHBLD_MP_DREL_HTTP);
  fh = fileopOpen (from, "r");
  if (fh == NULL) {
    return;
  }
  *tmp = '\0';
  (void) ! fgets (tmp, sizeof (tmp), fh);
  (void) ! fgets (tmp, sizeof (tmp), fh);
  fclose (fh);
  if (sscanf (tmp, "<!-- VERSION %d", &version) != 1) {
    version = 1;
  }

  logMsg (LOG_INSTALL, LOG_INFO, "version check %s : %d < %d", fn, version, currvers);
  if (version < currvers) {
    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateHttpCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
  }
}

static int
updaterGetMPMValue (song_t *song)
{
  ilistidx_t  didx;
  int         tval = LIST_VALUE_INVALID;

  didx = songGetNum (song, TAG_DANCE);
  tval = songGetNum (song, TAG_BPM);
  tval = danceConvertBPMtoMPM (didx, tval, DANCE_FORCE_CONV);
  return tval;
}
