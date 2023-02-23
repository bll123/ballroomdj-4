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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <getopt.h>

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

#define UPDATER_TMP_FILE "tmpupdater.txt"

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
  UPD_FIX_AF_TAGS,
  UPD_MAX,
};

static datafilekey_t upddfkeys[] = {
  { "CONVERTED",        UPD_CONVERTED,      VALUE_NUM, NULL, -1 },
  { "FIRSTVERSION",     UPD_FIRST_VERS,     VALUE_STR, NULL, -1 },
  { "FIX_AF_TAGS",      UPD_FIX_AF_TAGS,    VALUE_NUM, NULL, -1 },
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
static void updaterCopyVersionCheck (const char *fn, const char *ext, int currvers);

int
main (int argc, char *argv [])
{
  bool        newinstall = false;
  bool        converted = false;
  int         c = 0;
  int         option_index = 0;
  char        *musicdir = NULL;
  char        *home = NULL;
  char        homemusicdir [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  int         value;
  char        *tval = NULL;
  bool        isbdj4 = false;
  bool        bdjoptchanged = false;
  int         haveitunes = 0;
  int         statusflags [UPD_MAX];
  int         processflags [UPD_MAX];
  bool        processaf = false;
  bool        processdb = false;
  datafile_t  *df;
  nlist_t     *updlist = NULL;
  musicdb_t   *musicdb = NULL;
  long        flags;

  static struct option bdj_options [] = {
    { "newinstall", no_argument,        NULL,   'n' },
    { "converted",  no_argument,        NULL,   'c' },
    { "musicdir",   required_argument,  NULL,   'm' },
    { "bdj4updater",no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   'B' },
    /* ignored */
    { "nodetach",   no_argument,        NULL,   0 },
    { "wait",       no_argument,        NULL,   0 },
    { "debugself",  no_argument,        NULL,   0 },
    { "scale",      required_argument,  NULL,   0 },
    { "theme",      required_argument,  NULL,   0 },
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
    processflags [i] = 0;
  }

  flags = BDJ4_INIT_NO_LOCK | BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "updt", ROUTE_NONE, &flags);
  logSetLevel (LOG_INSTALL, LOG_IMPORTANT | LOG_BASIC | LOG_MAIN, "updt");
  logSetLevel (LOG_DBG, LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST, "updt");
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== updater started");

  pathbldMakePath (tbuff, sizeof (tbuff),
      "updater", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  df = datafileAllocParse ("updater", DFTYPE_KEY_VAL, tbuff,
      upddfkeys, UPD_DF_COUNT);
  updlist = datafileGetList (df);

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

  logMsg (LOG_INSTALL, LOG_MAIN, "cleaned volreg/flag");

  /* always figure out where the home music dir is */
  /* this is used on new intalls to set the music dir */
  /* also needed to check for the itunes dir every time */
  home = sysvarsGetStr (SV_HOME);
  if (isLinux ()) {
    const char  *targv [5];
    int         targc = 0;
    char        data [200];
    char        *prog;

    prog = sysvarsGetStr (SV_PATH_XDGUSERDIR);
    if (*prog) {
      *data = '\0';
      targc = 0;
      targv [targc++] = prog;
      targv [targc++] = "MUSIC";
      targv [targc++] = NULL;
      osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, sizeof (data), NULL);
      stringTrim (data);
      stringTrimChar (data, '/');

      /* xdg-user-dir returns the home folder if the music dir does */
      /* not exist */
      if (*data && strcmp (data, home) != 0) {
        strlcpy (homemusicdir, data, sizeof (homemusicdir));
      } else {
        snprintf (homemusicdir, sizeof (homemusicdir), "%s/Music", home);
      }
    }
  }
  if (isWindows ()) {
    char    *data;

    snprintf (homemusicdir, sizeof (homemusicdir), "%s/Music", home);
    data = osRegistryGet (
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Music");
    if (data != NULL && *data) {
      /* windows returns the path with %USERPROFILE% */
      strlcpy (homemusicdir, home, sizeof (homemusicdir));
      strlcat (homemusicdir, data + 13, sizeof (homemusicdir));
    } else {
      data = osRegistryGet (
          "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
          "My Music");
      if (data != NULL && *data) {
        /* windows returns the path with %USERPROFILE% */
        strlcpy (homemusicdir, home, sizeof (homemusicdir));
        strlcat (homemusicdir, data + 13, sizeof (homemusicdir));
      }
    }
  }
  if (isMacOS ()) {
    snprintf (homemusicdir, sizeof (homemusicdir), "%s/Music", home);
  }
  pathNormPath (homemusicdir, sizeof (homemusicdir));

  logMsg (LOG_INSTALL, LOG_MAIN, "homemusicdir: %s", homemusicdir);

  value = updaterGetStatus (updlist, UPD_FIX_AF_TAGS);

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

      bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, false);
      value = UPD_SKIP;
      nlistSetNum (updlist, UPD_FIX_AF_TAGS, value);

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
    }
    logMsg (LOG_INSTALL, LOG_MAIN, "finish new install");
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
    } // if itunes dir is not set
  } // if not converted

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
    bdjoptSetStr (OPT_M_DIR_MUSIC, musicdir);
  }

  {
    /* 4.1.0 change name of audiotag dylib (prep) */
    tval = bdjoptGetStr (OPT_M_AUDIOTAG_INTFC);
    if (strcmp (tval, "libaudiotagmutagen") == 0) {
      bdjoptSetStr (OPT_M_AUDIOTAG_INTFC, "libatimutagen");
      bdjoptchanged = true;
    }
  }

  if (bdjoptchanged) {
    bdjoptSave ();
  }

  /* clean up any old files and directories */

  logMsg (LOG_INSTALL, LOG_MAIN, "start clean");

  updaterCleanFiles ();

  logMsg (LOG_INSTALL, LOG_MAIN, "end clean");

  /* datafile updates */

  {
    /* 4.0.5 2023-1-4 itunes-fields */
    /*   had the incorrect 'lastupdate' name removed completely (not needed) */
    /*   as itunes has not been implemented yet, it is safe to completely */
    /*   overwrite version 1. */
    updaterCopyVersionCheck (ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, 2);
  }

  {
    /* 4.1.0 2023-1-5 audioadjust.txt */
    updaterCopyIfNotPresent (AUDIOADJ_FN, BDJ4_CONFIG_EXT);
    /* 4.1.1 2023-2-18 (version number bump) audioadjust.txt */
    updaterCopyVersionCheck (AUDIOADJ_FN, BDJ4_CONFIG_EXT, 3);
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

  /* The datafiles must now be loaded. */

  if (bdjvarsdfloadInit () < 0) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  {
    playlist_t  *pl;
    dance_t     *dances;
    slist_t     *dnclist;
    int         didx;
    long        mpt;

    /* 2023-1-21 : The QueueDance playlist had bad data in it. */
    /* 2023-1-21 : The StandardRounds playlist had bad data in it. */
    /* check for VW w/a time limit of 0:15 and if there, reset it, */
    /* and reset the BPM limits on waltz. */

    /* the music db is not needed to load the playlist */

    dances = bdjvarsdfGet (BDJVDF_DANCES);
    dnclist = danceGetDanceList (dances);

    didx = slistGetNum (dnclist, _("Viennese Waltz"));
    pl = playlistLoad (_("QueueDance"), NULL);
    mpt = playlistGetDanceNum (pl, didx, PLDANCE_MAXPLAYTIME);
    if (mpt == 15000) {
      playlistSetDanceNum (pl, didx, PLDANCE_MAXPLAYTIME, 0);
      didx = slistGetNum (dnclist, _("Waltz"));
      playlistSetDanceNum (pl, didx, PLDANCE_BPM_HIGH, 0);
      playlistSetDanceNum (pl, didx, PLDANCE_BPM_LOW, 0);
      playlistSave (pl, _("QueueDance"));
      logMsg (LOG_INSTALL, LOG_MAIN, "queuedance playlist updated");
    }
    playlistFree (pl);

    didx = slistGetNum (dnclist, _("Viennese Waltz"));
    pl = playlistLoad (_("standardrounds"), NULL);
    mpt = playlistGetDanceNum (pl, didx, PLDANCE_MAXPLAYTIME);
    if (mpt == 15000) {
      playlistSetDanceNum (pl, didx, PLDANCE_MAXPLAYTIME, 0);
      didx = slistGetNum (dnclist, _("Waltz"));
      playlistSetDanceNum (pl, didx, PLDANCE_BPM_HIGH, 0);
      playlistSetDanceNum (pl, didx, PLDANCE_BPM_LOW, 0);
      playlistSave (pl, _("standardrounds"));
      logMsg (LOG_INSTALL, LOG_MAIN, "standardrounds playlist updated");
    }
    playlistFree (pl);
  }

  /* All database processing must be done last after the updates to the */
  /* datafiles are done. */

  /* audio file conversions */

  /* fix audio file tags check; this is only run if: */
  /* - it has never been run */
  /* - not dev release */
  /* - write-tag is on */
  /* - bdj3-compat-tags is off */

  statusflags [UPD_FIX_AF_TAGS] = value;
  processflags [UPD_FIX_AF_TAGS] =
      converted &&
      value == UPD_NOT_DONE &&
      strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") != 0 &&
      bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE &&
      bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS) == false;
  if (processflags [UPD_FIX_AF_TAGS]) { processaf = true; }

  if (processaf || processdb) {
    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
    musicdb = dbOpen (tbuff);
  }

  if (processaf) {
    char        *ffn;
    slistidx_t  dbiteridx;
    song_t      *song;
    dbidx_t     dbidx;
    bool        process;
    char        *data;
    slist_t     *taglist;
    int         rewrite;

    logMsg (LOG_INSTALL, LOG_IMPORTANT, "processing audio files");
    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      ffn = songutilFullFileName (songGetStr (song, TAG_FILE));
      process = false;

      if (processflags [UPD_FIX_AF_TAGS]) {
        pathinfo_t    *pi;

        pi = pathInfo (ffn);
        if (pathInfoExtCheck (pi, ".mp3") ||
            pathInfoExtCheck (pi, ".flac") ||
            pathInfoExtCheck (pi, ".ogg")) {
          process = true;
        }
        pathInfoFree (pi);
      }

      if (! process) {
        mdfree (ffn);
        continue;
      }

      data = audiotagReadTags (ffn);
      taglist = audiotagParseData (ffn, data, &rewrite);

      if (processflags [UPD_FIX_AF_TAGS] && rewrite) {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "fix audio tags: %d %s", dbidx, ffn);
        audiotagWriteTags (ffn, taglist, taglist, rewrite, AT_KEEP_MOD_TIME);
      }

      mdfree (data);
      slistFree (taglist);
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
      ;
    }
    dbEndBatch (musicdb);
  }

  if (processaf || processdb) {
    if (processflags [UPD_FIX_AF_TAGS]) {
      nlistSetNum (updlist, UPD_FIX_AF_TAGS, UPD_COMPLETE);
    }
    dbClose (musicdb);
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      "updater", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  datafileSaveKeyVal ("updater", tbuff, upddfkeys, UPD_DF_COUNT, updlist, 0);
  datafileFree (df);

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
  char    *basedir;
  int     count;
  nlist_t *cleanlist;
  bool    macosonly = false;
  bool    linuxonly = false;
  bool    windowsonly = false;
  bool    processflag = false;

  /* look for development directories and do not run if any are found */
  if (fileopIsDirectory ("dev") ||
      fileopIsDirectory ("test-music-orig") ||
      fileopIsDirectory ("packages")) {
    return;
  }

  pathbldMakePath (fname, sizeof (fname),
      "cleanuplist", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  basedir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  cleanlist = nlistAlloc ("clean-regex", LIST_UNORDERED, updaterCleanlistFree);

  count = 0;
  fh = fileopOpen (fname, "r");
  if (fh != NULL) {
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
      // logMsg (LOG_INSTALL, LOG_IMPORTANT, "clean %s", fullpattern);
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
      if (regexMatch (rx, fn)) {
        // fprintf (stderr, "  match %s\n", fn);
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
    logMsg (LOG_INSTALL, LOG_MAIN, "%s%s installed", fn, ext);
  }
}

static void
updaterCopyVersionCheck (const char *fn, const char *ext, int currvers)
{
  datafile_t  *tmpdf;
  int         version;
  slist_t     *slist;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff), fn, ext, PATHBLD_MP_DREL_DATA);
  tmpdf = datafileAllocParse (fn, DFTYPE_LIST, tbuff, NULL, 0);
  slist = datafileGetList (tmpdf);
  version = slistGetVersion (slist);
  if (version == 1) {
    char  tmp [MAXPATHLEN];

    snprintf (tmp, sizeof (tmp), "%s%s", fn, ext);
    templateFileCopy (tmp, tmp);
    logMsg (LOG_INSTALL, LOG_MAIN, "%s updated", fn);
  }
  datafileFree (tmpdf);
}
