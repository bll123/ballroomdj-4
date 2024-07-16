/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvars.h"
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
#include "pathinfo.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"
#include "volreg.h"

static const char * const LINUX_STARTUP_SCRIPT = "scripts/linux/bdjstartup.sh";
static const char * const LINUX_SHUTDOWN_SCRIPT = "scripts/linux/bdjshutdown.sh";

enum {
  UPD_NOT_DONE,
  UPD_SKIP,
  UPD_COMPLETE,
  UPD_NO_FORCE,
  UPD_FORCE,
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
  /* 2023-12 4.4.8 replaced fix-db-add-date */
  /* fix-db-add-date will be set to skip by default, the user will need */
  /* to edit the updater file to set this value to 'force' */
  /* the name was changed to disambiguate from the old name */
  UPD_FIX_DB_DATE_ADDED,
  /* 2023-5-22 4.3.2.4 */
  /* set-mpm changes the configure/general/bpm setting to mpm */
  /* only want to do this once */
  UPD_SET_MPM,
  /* 2023-6-24 4.3.3 */
  /* there are bad disc numbers present in some data, and some were */
  /* converted to negative numbers */
  UPD_FIX_DB_DISCNUM,
  /* 2024-4-26 4.9.0 */
  /* fix any bad date-added in the new format */
  UPD_FIX_DB_DATE_ADDED_B,
  UPD_MAX,
};
enum {
  UPD_FIRST = UPD_CONVERTED + 1,
};


static datafilekey_t upddfkeys[] = {
  { "CONVERTED",        UPD_CONVERTED,      VALUE_NUM, NULL, DF_NORM },
  { "FIRSTVERSION",     UPD_FIRST_VERS,     VALUE_STR, NULL, DF_NORM },
  { "FIX_AF_TAGS",      UPD_FIX_AF_TAGS,    VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_DATE_ADDED", UPD_FIX_DB_DATE_ADDED, VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_DATE_ADD_B", UPD_FIX_DB_DATE_ADDED_B, VALUE_NUM, NULL, DF_NORM },
  { "FIX_DB_DISCNUM",   UPD_FIX_DB_DISCNUM, VALUE_NUM, NULL, DF_NORM },
};
enum {
  UPD_DF_COUNT = (sizeof (upddfkeys) / sizeof (datafilekey_t))
};

static void updaterCleanFiles (void);
static void updaterCleanProcess (bool macosonly, bool windowsonly, bool linuxonly, const char *basedir, nlist_t *cleanlist);
static void updaterCleanlistFree (void *trx);
static void updaterCleanRegex (const char *basedir, slist_t *filelist, nlist_t *cleanlist);
static int  updaterGetStatus (nlist_t *updlist, int key);
static void updaterCopyIfNotPresent (const char *fn, const char *ext, const char *newfn);
static void updaterCopyProfileIfNotPresent (const char *fn, const char *ext, int forceflag);
static void updaterCopyVersionCheck (const char *fn, const char *ext, int currvers);
static void updaterCopyProfileVersionCheck (const char *fn, const char *ext, int currvers);
static void updaterCopyHTMLVersionCheck (const char *fn, const char *ext, int currvers);
static void updaterCopyCSSVersionCheck (const char *fn, const char *ext, int currvers);
static void updaterRenameProfileFile (const char *oldfn, const char *fn, const char *ext);
static time_t updaterGetSongCreationTime (song_t *song);

int
main (int argc, char *argv [])
{
  bool        newinstall = false;
  bool        converted = false;
  char        homemusicdir [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  const char  *tval = NULL;
  bool        bdjoptchanged = false;
  int         haveitunes = 0;
  int         statusflags [UPD_MAX];
  int32_t     counters [UPD_MAX];
  bool        processflags [UPD_MAX];
  bool        processaf= false;
  bool        processdb = false;
  bool        updlistallocated = false;
  datafile_t  *df;
  nlist_t     *updlist = NULL;
  musicdb_t   *musicdb = NULL;
  uint32_t    flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("updt");
#endif

  for (int i = 0; i < UPD_MAX; ++i) {
    processflags [i] = false;
    counters [i] = 0;
  }

  flags = BDJ4_INIT_NO_LOCK | BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "updt", ROUTE_NONE, &flags);
  if ((flags & BDJ4_ARG_UPD_NEW) == BDJ4_ARG_UPD_NEW) {
    newinstall = true;
  }
  if ((flags & BDJ4_ARG_UPD_CONVERT) == BDJ4_ARG_UPD_CONVERT) {
    converted = true;
  }

  logSetLevel (LOG_INSTALL, LOG_IMPORTANT | LOG_BASIC | LOG_INFO, "updt");
  logSetLevel (LOG_DBG, LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST, "updt");
  logStartProgram ("updater");

  pathbldMakePath (tbuff, sizeof (tbuff),
      READONLY_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_MAIN);
  if (fileopFileExists (tbuff)) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "readonly install");
    updaterCleanFiles ();
    goto finish;
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      "updater", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  df = datafileAllocParse ("updater", DFTYPE_KEY_VAL, tbuff,
      upddfkeys, UPD_DF_COUNT, DF_NO_OFFSET, NULL);
  updlist = datafileGetList (df);
  if (updlist == NULL || nlistGetCount (updlist) == 0) {
    updlist = nlistAlloc ("updater-updlist", LIST_ORDERED, NULL);
    updlistallocated = true;
  }
  nlistSetVersion (updlist, 3);

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
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (tbuff);
  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);
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

    tval = bdjoptGetStr (OPT_M_STARTUP_SCRIPT);
    if (isLinux () && (tval == NULL || ! *tval)) {
      bdjoptSetStr (OPT_M_STARTUP_SCRIPT, LINUX_STARTUP_SCRIPT);
      bdjoptSetStr (OPT_M_SHUTDOWN_SCRIPT, LINUX_SHUTDOWN_SCRIPT);
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
      bdjoptchanged = true;
      nlistSetNum (updlist, UPD_FIX_AF_TAGS, statusflags [UPD_FIX_AF_TAGS]);

      /* need some decent default fonts for windows and macos */
      if (isWindows () || isMacOS ()) {
        uifont = "Arial 11";
        if (isMacOS ()) {
          uifont = "Arial Regular 16";
        }
        bdjoptSetStr (OPT_MP_UIFONT, uifont);

        /* windows does not have a narrow font pre-installed */
        uifont = "Arial 11";
        if (isMacOS ()) {
          uifont = "Arial Narrow Regular 15";
        }
        bdjoptSetStr (OPT_MP_MQFONT, uifont);

        uifont = "Arial 10";
        if (isMacOS ()) {
          uifont = "Arial Regular 15";
        }
        bdjoptSetStr (OPT_MP_LISTING_FONT, uifont);

        bdjoptchanged = true;
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

  {
    /* 4.4.2.2 change name of audiotag dylib (prep) */
    /* 4.4.3 remove mutagen */
    tval = bdjoptGetStr (OPT_M_AUDIOTAG_INTFC);
    if (tval != NULL &&
        (strcmp (tval, "libaudiotagmutagen") == 0 ||
        strcmp (tval, "libatimutagen") == 0)) {
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.4.2.2 : chg name of audiotag dylib");
      bdjoptSetStr (OPT_M_AUDIOTAG_INTFC, "libatibdj4");
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
    if (statusflags [UPD_SET_MPM] == UPD_NOT_DONE) {
      /* 4.3.2.4 : change BPM to default to MPM */
      bdjoptSetNum (OPT_G_BPM, BPM_MPM);
      nlistSetNum (updlist, UPD_SET_MPM, UPD_COMPLETE);
    }

    /* 4.3.2.4 : change dancesel method to windowed */
    bdjoptSetNum (OPT_G_DANCESEL_METHOD, DANCESEL_METHOD_WINDOWED);
    bdjoptchanged = true;
  }

  {
    /* 4.8.0 : change linux scripts to use a relative path */
    if (isLinux ()) {
      tval = bdjoptGetStr (OPT_M_STARTUP_SCRIPT);
      pathbldMakePath (tbuff, sizeof (tbuff),
          LINUX_STARTUP_SCRIPT, "", PATHBLD_MP_DIR_MAIN);
      if (strcmp (tbuff, tval) == 0) {
        bdjoptSetStr (OPT_M_STARTUP_SCRIPT, LINUX_STARTUP_SCRIPT);
        bdjoptchanged = true;
      }
      tval = bdjoptGetStr (OPT_M_SHUTDOWN_SCRIPT);
      pathbldMakePath (tbuff, sizeof (tbuff),
          LINUX_SHUTDOWN_SCRIPT, "", PATHBLD_MP_DIR_MAIN);
      if (strcmp (tbuff, tval) == 0) {
        bdjoptSetStr (OPT_M_SHUTDOWN_SCRIPT, LINUX_SHUTDOWN_SCRIPT);
        bdjoptchanged = true;
      }
    }
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
    char to [MAXPATHLEN];

    /* 4.11.0 http/curl-ca-bundle.crt was not being updated. */
    /* always update it. */
    pathbldMakePath (tbuff, sizeof (tbuff),
        "curl-ca-bundle.crt", "", PATHBLD_MP_DIR_TEMPLATE);
    pathbldMakePath (to, sizeof (to),
        "curl-ca-bundle.crt", "", PATHBLD_MP_DREL_HTTP);
    filemanipCopy (tbuff, to);
  }

  {
    /* 4.0.5 2023-1-4 itunes-fields */
    /*   'lastupdate' name removed completely (not needed) */
    /*   as itunes has not been implemented yet, it is safe to completely */
    /*   overwrite version 1. */
    /* 4.8.3.1 2024-4-19 updated to version 3 */
    /* 4.9.0 2024-4-22 updated to version 4 */
    updaterCopyVersionCheck (ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, 4);
  }

  {
    /* 4.2.0 2023-3-5 autoselection.txt */
    /* 4.3.2.4 2023-5-26 */
    /* 4.3.3.2 2023-7-29 */
    /* added windowed values (version 4) */
    /* added fastprior (version 5) */
    /* 4.6.0 2024-2-18 */
    /* added tagweight (version 6) */
    updaterCopyVersionCheck (AUTOSEL_FN, BDJ4_CONFIG_EXT, 6);
  }

  {
    /* 4.1.0 2023-1-5 audioadjust.txt */
    updaterCopyIfNotPresent (AUDIOADJ_FN, BDJ4_CONFIG_EXT, NULL);
    /* 4.3.0.4 2023-4-4 (version number bump) audioadjust.txt */
    updaterCopyVersionCheck (AUDIOADJ_FN, BDJ4_CONFIG_EXT, 4);
  }

  {
    /* 4.10.0 2023-1-29 gtk-static.css */
    /*    This is a new file; simply check and see if it does not exist. */
    /* 4.5.0 2024-2-10 updated switch */
    /* 4.11.0 2024-6-18 virtlist */
    updaterCopyIfNotPresent ("gtk-static", BDJ4_CSS_EXT, NULL);
    updaterCopyCSSVersionCheck ("gtk-static", BDJ4_CSS_EXT, 6);
  }

  {
    /* 4.1.0 2023-2-9 nl renamed, just re-install if not there */
    /* 4.4.1.2 2023-10-5 nl renamed, just re-install if not there */
    updaterCopyIfNotPresent ("QueueDance", BDJ4_PLAYLIST_EXT, _("QueueDance"));
    updaterCopyIfNotPresent ("QueueDance", BDJ4_PL_DANCE_EXT, _("QueueDance"));
  }

  {
    /* 2023-1-21 : The QueueDance playlist had bad data in it. */
    /* 2023-1-21 : The StandardRounds playlist had bad data in it. */
    /* 2023-4-17 : QueueDance updated so that the mm filter would */
    /*             display the queuedance playlist properly. */
    /* 2023-5-23 : 4.3.2.4 */
    /*             Updated internal key names */
    /* 2023-12-22 : 4.4.8 */
    /*             Cleanup, fix en-us */
    /* 2024-6-17 : 4.10.5 remove automatic */
    updaterCopyVersionCheck (_("QueueDance"), BDJ4_PLAYLIST_EXT, 3);
    updaterCopyVersionCheck (_("QueueDance"), BDJ4_PL_DANCE_EXT, 5);
    updaterCopyVersionCheck (_("standardrounds"), BDJ4_PLAYLIST_EXT, 3);
    updaterCopyVersionCheck (_("standardrounds"), BDJ4_PL_DANCE_EXT, 4);
  }

  {
    /* 4.3.2: 2023-4-27 : Mobile marquee HTML file has minor updates. */
    /* 4.4.8: 2023-12-22 : fix version check, update version */
    updaterCopyHTMLVersionCheck ("mobilemq", BDJ4_HTML_EXT, 3);
  }

  {
    /* 4.8.0 sortopt.txt updated to version 2 */
    /* 4.8.3.1 sortopt.txt updated to version 3 */
    updaterCopyVersionCheck (SORTOPT_FN, BDJ4_CONFIG_EXT, 3);
  }

  /* The datafiles must be loaded for the MPM update process */

  if (bdjvarsdfloadInit () < 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  logMsg (LOG_INSTALL, LOG_INFO, "loaded data files A");

  {
    int     origprofile;

    /* 4.4.2 2023-10-18.  Bug in the updater created extra bad profiles */
    /*   check each profile to see if ds-mm.txt exists (also bdjconfig.txt) */
    /*   if not, remove the profile */

    origprofile = sysvarsGetNum (SVL_PROFILE_IDX);

    for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
      sysvarsSetNum (SVL_PROFILE_IDX, i);
      pathbldMakePath (tbuff, sizeof (tbuff), "ds-mm", BDJ4_CONFIG_EXT,
          PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      if (! fileopFileExists (tbuff)) {
        bdjoptDeleteProfile ();
      }
      pathbldMakePath (tbuff, sizeof (tbuff), "bdjconfig", BDJ4_CONFIG_EXT,
          PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      if (! fileopFileExists (tbuff)) {
        bdjoptDeleteProfile ();
      }
    }
    sysvarsSetNum (SVL_PROFILE_IDX, origprofile);
  }

  {
    /* 4.4.0 2023-9-12 audio-id data selection */
    /* 4.11.0 2024-7-8 audio-id tag identifier change */
    updaterCopyProfileIfNotPresent ("ds-audioid-list", BDJ4_CONFIG_EXT, UPD_NO_FORCE);
    updaterCopyProfileVersionCheck ("ds-audioid-list", BDJ4_CONFIG_EXT, 2);
    updaterCopyProfileIfNotPresent ("ds-audioid", BDJ4_CONFIG_EXT, UPD_NO_FORCE);
    /* ez renamed to sbs internally */
    updaterRenameProfileFile ("ds-ezsongsel", "ds-sbssongsel", BDJ4_CONFIG_EXT);
    updaterRenameProfileFile ("ds-ezsonglist", "ds-sbssonglist", BDJ4_CONFIG_EXT);
  }

  /* profile updates */

  {
    int     origprofile;

    origprofile = sysvarsGetNum (SVL_PROFILE_IDX);

    for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
      sysvarsSetNum (SVL_PROFILE_IDX, i);
      if (bdjoptProfileExists ()) {
        /* 4.4.1 2023-9-30 there is a new image file, make sure it gets installed */
        pathbldMakePath (tbuff, sizeof (tbuff), "button_filter", BDJ4_IMG_SVG_EXT,
            PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
        if (! fileopFileExists (tbuff)) {
          templateImageCopy (bdjoptGetStr (OPT_P_UI_ACCENT_COL));
        }

        /* 4.4.2.2 2023-11-5 fix bad installations of .q files */
        for (int j = 0; j < BDJ4_QUEUE_MAX; ++j) {
          const char  *tval;

          tval = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, j);
          if (tval == NULL || ! *tval) {
            char    tbuff [MAXPATHLEN];

            snprintf (tbuff, sizeof (tbuff), "%s.q%d", "bdjconfig", j);
            updaterCopyProfileIfNotPresent (tbuff, BDJ4_CONFIG_EXT, UPD_FORCE);
          }
        }
      }
    }
    sysvarsSetNum (SVL_PROFILE_IDX, origprofile);
  }

  {
    /* 4.4.2 2023-10-18 added another queue */
    updaterCopyProfileIfNotPresent ("bdjconfig.q4", BDJ4_CONFIG_EXT, UPD_NO_FORCE);
  }

  {
    /* 4.4.9 2024-1-25.  Install new ds-marquee.txt */
    updaterCopyProfileIfNotPresent ("ds-marquee", BDJ4_CONFIG_EXT, UPD_NO_FORCE);
  }

  {
    /* 4.8.1 2024-3-24 ds-quickedit.txt */
    updaterCopyProfileIfNotPresent (DS_QUICKEDIT_FN, BDJ4_CONFIG_EXT, UPD_NO_FORCE);
  }

  {
    /* 4.8.3 2024-4-16. Install new ds-player.txt */
    /* 4.10.1 2024-5-20. Rename to ds-currsong.txt */
    updaterRenameProfileFile ("ds-player", "ds-currsong", BDJ4_CONFIG_EXT);
    updaterCopyProfileIfNotPresent ("ds-currsong", BDJ4_CONFIG_EXT, UPD_NO_FORCE);
  }

  /* now re-load the data files */

  bdjvarsdfloadCleanup ();

  if (bdjvarsdfloadInit () < 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  logMsg (LOG_INSTALL, LOG_INFO, "loaded data files B");

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
      bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE;
  if (processflags [UPD_FIX_AF_TAGS]) {
    logMsg (LOG_INSTALL, LOG_INFO, "-- fix af tags");
    processaf = true;
  } else {
    if (statusflags [UPD_FIX_AF_TAGS] == UPD_NOT_DONE) {
      if (strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") != 0) {
        nlistSetNum (updlist, UPD_FIX_AF_TAGS, UPD_SKIP);
      }
    }
  }

  if (statusflags [UPD_FIX_DB_DATE_ADDED] == UPD_FORCE) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.4.8 : process db : fix db add date");
    processflags [UPD_FIX_DB_DATE_ADDED] = true;
    processdb = true;
  }
  if (statusflags [UPD_FIX_DB_DATE_ADDED_B] == UPD_NOT_DONE) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "-- 4.9.0 : process db : fix db add date B (new format)");
    processflags [UPD_FIX_DB_DATE_ADDED_B] = true;
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
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Database read: %" PRId32 " items in %" PRId64 " ms", dbCount(musicdb), (int64_t) mstimeend (&dbmt));
  }

  if (processaf) {
    slistidx_t  dbiteridx;
    song_t      *song;
    dbidx_t     dbidx;

    logMsg (LOG_INSTALL, LOG_IMPORTANT, "processing audio files");

    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      char        ffn [MAXPATHLEN];
      const char  *tkey;
      int         rewrite;
      bool        process = false;
      slist_t     *taglist;
      slist_t     *newtaglist;
      slistidx_t  siteridx;

      audiosrcFullPath (songGetStr (song, TAG_URI), ffn, sizeof (ffn), 0, NULL);

      if (processflags [UPD_FIX_AF_TAGS]) {
        pathinfo_t    *pi;

        pi = pathInfo (ffn);
        if (pathInfoExtCheck (pi, ".mp3") ||
            pathInfoExtCheck (pi, ".flac") ||
            pathInfoExtCheck (pi, ".ogg") ||
            pathInfoExtCheck (pi, ".m4a") ||
            pathInfoExtCheck (pi, ".ogx") ||
            pathInfoExtCheck (pi, ".opus")) {
          process = true;
        }
        pathInfoFree (pi);
      }

      if (! process) {
        continue;
      }

      taglist = audiotagParseData (ffn, &rewrite);

      newtaglist = slistAlloc ("new-taglist", LIST_UNORDERED, NULL);
      slistSetSize (newtaglist, slistGetCount (taglist));
      slistStartIterator (taglist, &siteridx);
      while ((tkey = slistIterateKey (taglist, &siteridx)) != NULL) {
        slistSetStr (newtaglist, tkey, slistGetStr (taglist, tkey));
      }
      slistSort (newtaglist);

      if (processflags [UPD_FIX_AF_TAGS] &&
          rewrite != AF_REWRITE_NONE) {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "write audio tags: %" PRId32 " %s", dbidx, ffn);
        audiotagWriteTags (ffn, taglist, newtaglist, rewrite, AT_KEEP_MOD_TIME);
      }

      slistFree (taglist);
      slistFree (newtaglist);
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

      /* 2023-3-13 : the database add date could be missing due to */
      /* bugs in restore-original and restore audio file data */
      /* 2023-12 : the database add date could have been munged */
      /* by bugs in the db-updater, update all add-dates */


      if (processflags [UPD_FIX_DB_DATE_ADDED_B]) {
        time_t      addval, luval;

        addval = songGetNum (song, TAG_DBADDDATE);
        luval = songGetNum (song, TAG_LAST_UPDATED);
        if (addval >= luval || addval < 0) {
          time_t    ctime;

          ctime = updaterGetSongCreationTime (song);
          songSetNum (song, TAG_DBADDDATE, ctime);
          dowrite = true;
          counters [UPD_FIX_DB_DATE_ADDED_B] += 1;
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "fix dbadddate-B: %s", songGetStr (song, TAG_URI));
        }
      }

      /* fix-db-date-added is a "forced" fix only */
      if (processflags [UPD_FIX_DB_DATE_ADDED] ||
            songGetNum (song, TAG_DBADDDATE) < 0) {
        time_t    ctime;

        ctime = updaterGetSongCreationTime (song);
        songSetNum (song, TAG_DBADDDATE, ctime);
        dowrite = true;
        counters [UPD_FIX_DB_DATE_ADDED] += 1;
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "fix dbadddate: %s", songGetStr (song, TAG_URI));
      }

      if (processflags [UPD_FIX_DB_DISCNUM]) {
        int   val;

        val = songGetNum (song, TAG_DISCNUMBER);
        if (val != LIST_VALUE_INVALID && (val < -1 || val > 5000)) {
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
    if (processflags [UPD_FIX_DB_DATE_ADDED]) {
      nlistSetNum (updlist, UPD_FIX_DB_DATE_ADDED, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_DB_DATE_ADDED_B]) {
      nlistSetNum (updlist, UPD_FIX_DB_DATE_ADDED_B, UPD_COMPLETE);
    }
    if (processflags [UPD_FIX_DB_DISCNUM]) {
      nlistSetNum (updlist, UPD_FIX_DB_DISCNUM, UPD_COMPLETE);
    }
    dbClose (musicdb);
  }

  if (counters [UPD_FIX_DB_DATE_ADDED] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-date-added: %" PRId32, counters [UPD_FIX_DB_DATE_ADDED]);
  }
  if (counters [UPD_FIX_DB_DATE_ADDED_B] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-date-add-B: %" PRId32, counters [UPD_FIX_DB_DATE_ADDED]);
  }
  if (counters [UPD_FIX_DB_DISCNUM] > 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "count: db-discnum: %" PRId32, counters [UPD_FIX_DB_DISCNUM]);
  }

  datafileSave (df, NULL, updlist, DF_NO_OFFSET, datafileDistVersion (df));
  datafileFree (df);
  if (updlistallocated) {
    nlistFree (updlist);
  }

finish:
  bdj4shutdown (ROUTE_NONE, NULL);
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

  /* make sure any old volreg.txt files are gone */
  snprintf (fname, sizeof (fname), "%s/%s%s",
      sysvarsGetStr (SV_DIR_CONFIG), VOLREG_FN, BDJ4_CONFIG_EXT);
  fileopDelete (fname);
  snprintf (fname, sizeof (fname), "%s/%s%s.bak.1",
      sysvarsGetStr (SV_DIR_CONFIG), VOLREG_FN, BDJ4_CONFIG_EXT);
  fileopDelete (fname);

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
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "pattern: %s", pattern); //

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
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "clean %s", fullpattern); //
    rx = regexInit (fullpattern);
    nlistSetData (cleanlist, count, rx);
    ++count;
  }
  mdextfclose (fh);
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
  const char  *fn;
  bdjregex_t  *rx;

  slistStartIterator (filelist, &fiteridx);
  while ((fn = slistIterateKey (filelist, &fiteridx)) != NULL) {
    nlistStartIterator (cleanlist, &cliteridx);
    while ((key = nlistIterateKey (cleanlist, &cliteridx)) >= 0) {
      rx = nlistGetData (cleanlist, key);
      if (regexMatch (rx, fn)) {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "  match %s", fn); //
        if (osIsLink (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete link %s", fn);
          fileopDelete (fn);
          break;
        } else if (fileopIsDirectory (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete dir %s", fn);
          diropDeleteDir (fn, DIROP_ALL);
          break;
        } else if (fileopFileExists (fn)) {
          logMsg (LOG_INSTALL, LOG_IMPORTANT, "delete %s", fn);
          fileopDelete (fn);
          break;
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
updaterCopyIfNotPresent (const char *fn, const char *ext, const char *newfn)
{
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  const char  *tfn;

  tfn = fn;
  if (newfn != NULL) {
    tfn = newfn;
  }
  pathbldMakePath (to, sizeof (to), tfn, ext, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (to)) {
    snprintf (from, sizeof (from), "%s%s", fn, ext);
    snprintf (to, sizeof (to), "%s%s", tfn, ext);
    templateFileCopy (from, to);
    logMsg (LOG_INSTALL, LOG_INFO, "%s%s installed", tfn, ext);
  }
}

static void
updaterCopyProfileIfNotPresent (const char *fn, const char *ext, int forceflag)
{
  char    tbuff [MAXPATHLEN];
  int     origprofile;

  origprofile = sysvarsGetNum (SVL_PROFILE_IDX);

  for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
    sysvarsSetNum (SVL_PROFILE_IDX, i);
    if (bdjoptProfileExists ()) {
      pathbldMakePath (tbuff, sizeof (tbuff), fn, ext,
          PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      if (! fileopFileExists (tbuff) || forceflag == UPD_FORCE) {
        snprintf (tbuff, sizeof (tbuff), "%s%s", fn, ext);
        templateProfileCopy (tbuff, tbuff);
        logMsg (LOG_INSTALL, LOG_INFO, "%s%s installed", fn, ext);
      }
    } /* if the profile exists */
  } /* for all profiles */
  sysvarsSetNum (SVL_PROFILE_IDX, origprofile);
}

static void
updaterCopyVersionCheck (const char *fn, const char *ext, int currvers)
{
  int         version;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff), fn, ext, PATHBLD_MP_DREL_DATA);
  version = datafileReadDistVersion (tbuff);
  logMsg (LOG_INSTALL, LOG_INFO, "version check %s%s : %d < %d", fn, ext, version, currvers);
  if (version < currvers) {
    char  tmp [MAXPATHLEN];

    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateFileCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
  }
}

static void
updaterCopyProfileVersionCheck (const char *fn, const char *ext, int currvers)
{
  char    tbuff [MAXPATHLEN];
  int     origprofile;
  int     version;

  origprofile = sysvarsGetNum (SVL_PROFILE_IDX);

  for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
    sysvarsSetNum (SVL_PROFILE_IDX, i);
    if (bdjoptProfileExists ()) {
      pathbldMakePath (tbuff, sizeof (tbuff), fn, ext,
          PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      version = datafileReadDistVersion (tbuff);
      logMsg (LOG_INSTALL, LOG_INFO, "version check %d %s%s : %d < %d", i, fn, ext, version, currvers);
      if (version < currvers) {
        char  tmp [MAXPATHLEN];

        snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
        templateProfileCopy (tmp, tmp);
        logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
      }
    } /* if the profile exists */
  } /* for all profiles */
  sysvarsSetNum (SVL_PROFILE_IDX, origprofile);
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
  mdextfclose (fh);
  fclose (fh);
  if (sscanf (tmp, "<!-- VERSION %d", &version) != 1) {
    version = 1;
  }
  if (version > 2000) {
    version = 1;
  }

  logMsg (LOG_INSTALL, LOG_INFO, "version check %s%s : %d < %d", fn, ext, version, currvers);
  if (version < currvers) {
    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateHttpCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
  }
}

static void
updaterCopyCSSVersionCheck (const char *fn, const char *ext, int currvers)
{
  int         version;
  char        from [MAXPATHLEN];
  char        tmp [MAXPATHLEN];
  FILE        *fh;

  pathbldMakePath (from, sizeof (from), fn, ext, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (from, "r");
  if (fh == NULL) {
    return;
  }
  *tmp = '\0';
  (void) ! fgets (tmp, sizeof (tmp), fh);
  mdextfclose (fh);
  fclose (fh);
  if (sscanf (tmp, "/* version %d", &version) != 1) {
    version = 1;
  }

  logMsg (LOG_INSTALL, LOG_INFO, "version check %s%s : %d < %d", fn, ext, version, currvers);
  if (version < currvers) {
    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateFileCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_INFO, "%s updated", fn);
  }
}

static void
updaterRenameProfileFile (const char *oldfn, const char *fn, const char *ext)
{
  char    from [MAXPATHLEN];
  char    to [MAXPATHLEN];
  int     origprofile;

  origprofile = sysvarsGetNum (SVL_PROFILE_IDX);

  for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
    sysvarsSetNum (SVL_PROFILE_IDX, i);
    if (bdjoptProfileExists ()) {
      pathbldMakePath (from, sizeof (from), oldfn, ext,
          PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      if (fileopFileExists (from)) {
        pathbldMakePath (to, sizeof (to), fn, ext,
            PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
        if (! fileopFileExists (to)) {
          filemanipMove (from, to);
        }
      }
    } /* if the profile exists */
  } /* for all profiles */
  sysvarsSetNum (SVL_PROFILE_IDX, origprofile);
}

static time_t
updaterGetSongCreationTime (song_t *song)
{
  char      ffn [MAXPATHLEN];
  char      tbuff [MAXPATHLEN];
  time_t    ctime;

  audiosrcFullPath (songGetStr (song, TAG_URI), ffn, sizeof (ffn), 0, NULL);
  ctime = fileopCreateTime (ffn);
  if (audiosrcOriginalExists (ffn)) {
    snprintf (tbuff, sizeof (tbuff), "%s%s", ffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
    ctime = fileopCreateTime (tbuff);
  }

  return ctime;
}
