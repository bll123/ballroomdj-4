/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "datafile.h"
#include "dirlist.h"
#include "filemanip.h"
#include "instutil.h"
#include "log.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "sysvars.h"
#include "templateutil.h"

static void instutilCopyHttpSVGFile (const char *fn);

void
instutilCreateShortcut (const char *name, const char *maindir,
    const char *target, int profilenum)
{
  char        path [MAXPATHLEN];
  char        buff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        tmp [40];
  const char  *targv [10];
  int         targc = 0;

  if (profilenum < 0) {
    profilenum = 0;
  }

  if (isWindows ()) {
    if (! chdir ("install")) {
      targv [targc++] = ".\\winshortcut.bat";
      snprintf (path, sizeof (path), "%s%s.lnk",
          "%USERPROFILE%\\Desktop\\", name);
      targv [targc++] = path;
      pathbldMakePath (buff, sizeof (buff),
          "bdj4", ".exe", PATHBLD_MP_DIR_EXEC);
      pathDisplayPath (buff, sizeof (buff));
      targv [targc++] = buff;
      strlcpy (tbuff, target, sizeof (tbuff));
      pathDisplayPath (tbuff, sizeof (tbuff));
      targv [targc++] = tbuff;
      snprintf (tmp, sizeof (tmp), "%d", profilenum);
      targv [targc++] = tmp;
      targv [targc++] = NULL;
      osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
      (void) ! chdir (maindir);
    }
  }

  if (isLinux ()) {
    snprintf (buff, sizeof (buff),
        "./install/linuxshortcut.sh '%s' '%s' '%s' %d",
        name, maindir, target, profilenum);
    (void) ! system (buff);
  }
}

void
instutilCopyTemplates (void)
{
  slist_t     *dirlist;
  slist_t     *renamelist;
  slistidx_t  iteridx;
  const char  *fname;
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  datafile_t  *srdf = NULL;
  datafile_t  *qddf = NULL;
  datafile_t  *autodf = NULL;
  pathinfo_t  *pi;


  renamelist = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-sr", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0, DF_NO_OFFSET, NULL);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-auto", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0, DF_NO_OFFSET, NULL);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-qd", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL, tbuff, NULL, 0, DF_NO_OFFSET, NULL);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "", PATHBLD_MP_DIR_TEMPLATE);

  dirlist = dirlistBasicDirList (tbuff, NULL);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (strcmp (fname, "qrcode") == 0) {
      continue;
    }
    if (strcmp (fname, "qrcode.html") == 0) {
      continue;
    }
    if (strcmp (fname, "html-list.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "helpdata.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "volintfc.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "playerintfc.txt") == 0) {
      continue;
    }

    strlcpy (from, fname, sizeof (from));

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      templateHttpCopy (from, "bdj4remote.html");
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      templateHttpCopy (from, fname);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, BDJ4_HTML_EXT)) {
      pathInfoFree (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      templateHttpCopy (fname, fname);
      pathInfoFree (pi);
      continue;
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      pathbldMakePath (from, sizeof (from), fname, "", PATHBLD_MP_DIR_TEMPLATE);
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA);
      } else if (pathInfoExtCheck (pi, ".p")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".txt")) {
        pathbldMakePath (to, sizeof (to),
            fname, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".m")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      } else {
        /* unknown extension */
        fprintf (stderr, "unknown extension for bdjconfig %.*s\n", (int) pi->elen, pi->extension);
        pathInfoFree (pi);
        continue;
      }

      logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", from);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);
      filemanipBackup (to, 1);
      filemanipCopy (from, to);
      pathInfoFree (pi);
      continue;
    } else if (pathInfoExtCheck (pi, BDJ4_CONFIG_EXT) ||
        pathInfoExtCheck (pi, BDJ4_CSS_EXT) ||
        pathInfoExtCheck (pi, BDJ4_SEQUENCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PL_DANCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PLAYLIST_EXT) ) {

      renamelist = NULL;
      if (strncmp (pi->basename, "automatic", pi->blen) == 0) {
        renamelist = datafileGetList (autodf);
      }
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        renamelist = datafileGetList (srdf);
      }
      if (strncmp (pi->basename, "QueueDance", pi->blen) == 0) {
        renamelist = datafileGetList (qddf);
      }

      strlcpy (tbuff, fname, sizeof (tbuff));
      if (renamelist != NULL) {
        const char  *tval;

        tval = slistGetStr (renamelist, sysvarsGetStr (SV_LOCALE_SHORT));
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%.*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      strlcpy (from, tbuff, sizeof (from));
      if (strncmp (pi->basename, "ds-", 3) == 0) {
        snprintf (to, sizeof (to), "profile00/%s", tbuff);
      } else {
        snprintf (to, sizeof (to), "%s", tbuff);
      }
    } else {
      /* unknown extension */
      pathInfoFree (pi);
      continue;
    }

    logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", from);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);
    templateFileCopy (from, to);
    pathInfoFree (pi);
  }
  slistFree (dirlist);

  datafileFree (srdf);
  datafileFree (autodf);
  datafileFree (qddf);
}

void
instutilCopyHttpFiles (void)
{
  char    from [MAXPATHLEN];
  char    to [MAXPATHLEN];
  char    tmp [MAXPATHLEN];
  char    tbuff [200];

  instutilCopyHttpSVGFile ("led_on");
  instutilCopyHttpSVGFile ("led_off");

  pathbldMakePath (from, sizeof (from),
      "http/favicon.ico", "", PATHBLD_MP_DIR_MAIN);
  pathbldMakePath (to, sizeof (to),
      "favicon.ico", "", PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);

  pathbldMakePath (from, sizeof (from),
      "http/ballroomdj4", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_MAIN);
  pathbldMakePath (to, sizeof (to),
      "ballroomdj4", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);

  pathbldMakePath (tmp, sizeof (tmp), "", "", PATHBLD_MP_DIR_MAIN);
  snprintf (from, sizeof (from), "%s/http/mrc", tmp);
  pathbldMakePath (to, sizeof (to),
      "mrc", "", PATHBLD_MP_DREL_HTTP);
  *tbuff = '\0';
  if (isWindows ()) {
    pathDisplayPath (from, sizeof (from));
    pathDisplayPath (to, sizeof (to));
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
  } else {
    snprintf (tbuff, sizeof (tbuff), "cp -r '%s' '%s'", from, "http");
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);
}

void
instutilGetMusicDir (char *homemusicdir, size_t sz)
{
  const char    *home;

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
        strlcpy (homemusicdir, data, sz);
      } else {
        snprintf (homemusicdir, sz, "%s/Music", home);
      }
    }
  }

  if (isWindows ()) {
    char    *data;

    snprintf (homemusicdir, sz, "%s/Music", home);
    data = osRegistryGet (
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Music");
    if (data != NULL && *data) {
      /* windows returns the path with %USERPROFILE% */
      strlcpy (homemusicdir, home, sz);
      strlcat (homemusicdir, data + 13, sz);
    } else {
      data = osRegistryGet (
          "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
          "My Music");
      if (data != NULL && *data) {
        /* windows returns the path with %USERPROFILE% */
        strlcpy (homemusicdir, home, sz);
        strlcat (homemusicdir, data + 13, sz);
      }
    }
  }

  if (isMacOS ()) {
    snprintf (homemusicdir, sz, "%s/Music", home);
  }

  pathNormalizePath (homemusicdir, sz);
}

/* internal routines */

static void
instutilCopyHttpSVGFile (const char *fn)
{
  char    from [MAXPATHLEN];
  char    to [MAXPATHLEN];

  pathbldMakePath (from, sizeof (from),
      fn, BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  pathbldMakePath (to, sizeof (to),
      fn, BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);
}

