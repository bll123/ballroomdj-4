/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "ati.h"
#include "audiofile.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "datafile.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "osenv.h"
#include "osprocess.h"
#include "osdirutil.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "sysvars.h"
#include "templateutil.h"

instati_t instati [INST_ATI_MAX] = {
  [INST_ATI_BDJ4] = { "libatibdj4" },
};

static void instutilCopyHttpSVGFile (const char *fn);

void
instutilCreateLauncher (const char *name, const char *maindir,
    const char *workdir, int profilenum)
{
  char        path [BDJ4_PATH_MAX];
  char        buff [BDJ4_PATH_MAX];
  char        tbuff [BDJ4_PATH_MAX];
  char        ibuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;

  if (profilenum < 0) {
    profilenum = 0;
  }

  if (isWindows ()) {
    char    abuff [BDJ4_PATH_MAX];

    /* shortcut location and name */
    pathbldMakePath (ibuff, sizeof (ibuff),
        "bin/bdj4winmksc", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_DIR_MAIN);
    targv [targc++] = ibuff;

    /* if onedrive is in use, the Desktop folder may be within the */
    /* onedrive folder. */
    snprintf (path, sizeof (path), "%s/OneDrive/Desktop",
        sysvarsGetStr (SV_HOME));
    if (fileopIsDirectory (path)) {
      snprintf (path, sizeof (path), "%s/OneDrive/Desktop/%s.lnk",
          sysvarsGetStr (SV_HOME), name);
    } else {
      /* if the onedrive path is not found, assume the usual path */
      snprintf (path, sizeof (path), "%s/Desktop/%s.lnk",
          sysvarsGetStr (SV_HOME), name);
    }
    pathDisplayPath (path, sizeof (path));
    targv [targc++] = path;

    /* executable */
    snprintf (buff, sizeof (buff), "%s/bin/bdj4.exe", maindir);
    pathDisplayPath (buff, sizeof (buff));
    targv [targc++] = buff;

    /* working dir */
    stpecpy (tbuff, tbuff + sizeof (tbuff), workdir);
    pathDisplayPath (tbuff, sizeof (tbuff));
    targv [targc++] = tbuff;

    if (profilenum > 0) {
      /* arguments */
      snprintf (abuff, sizeof (abuff), " --profile %d ", profilenum);
      targv [targc++] = abuff;
    }
    targv [targc++] = NULL;
    osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

    /* 4.8.1 create a shortcut within the BDJ4 folder so that BDJ4 can be */
    /* started when the BDJ4 folder is open */
    /* unfortunately, it cannot be made using relative paths */

    /* targv[0] is valid */
    targc = 1;

    snprintf (path, sizeof (path), "%s/%s.lnk", workdir, name);
    pathDisplayPath (path, sizeof (path));
    targv [targc++] = path;

    /* executable */
    snprintf (buff, sizeof (buff), "%s/bin/bdj4.exe", maindir);
    pathDisplayPath (buff, sizeof (buff));
    targv [targc++] = buff;

    /* working dir */
    snprintf (tbuff, sizeof (tbuff), "%s", workdir);
    pathDisplayPath (tbuff, sizeof (tbuff));
    targv [targc++] = tbuff;

    if (profilenum > 0) {
      /* abuff is already populated */
      targv [targc++] = abuff;
    }
    targv [targc++] = NULL;
    osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  }

  if (isLinux () || isMacOS ()) {
    if (isMacOS () &&
        strcmp (name, BDJ4_NAME) == 0 &&
        profilenum == 0) {
      /* can never allow the macos script to be run for the main profile-0 */
      return;
    }
    if (isLinux ()) {
      pathbldMakePath (ibuff, sizeof (ibuff),
          "install/linux-mk-desktop-launcher.sh", "", PATHBLD_MP_DIR_MAIN);
      targv [targc++] = ibuff;
    }
    if (isMacOS ()) {
      pathbldMakePath (buff, sizeof (buff),
          "install/macos-mk-app.sh", "", PATHBLD_MP_DIR_MAIN);
      targv [targc++] = buff;
    }
    targv [targc++] = name;
    targv [targc++] = maindir;
    targv [targc++] = workdir;
    snprintf (tbuff, sizeof (tbuff), "%d", profilenum);
    targv [targc++] = tbuff;
    targv [targc++] = NULL;
    osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  }
}

void
instutilCopyTemplates (void)
{
  slist_t     *dirlist;
  slistidx_t  iteridx;
  const char  *fname;
  char        from [BDJ4_PATH_MAX];
  char        to [BDJ4_PATH_MAX];
  char        tbuff [BDJ4_PATH_MAX];
  pathinfo_t  *pi;
  ilistidx_t  fkey = -1;

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
    if (strcmp (fname, "localization.txt") == 0) {
      continue;
    }

    stpecpy (from, from + sizeof (from), fname);

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
      stpecpy (from, from + sizeof (from), fname);
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);

      /* use pathbldMakePath without specifying PATHBLD_MP_DREL_DATA */
      /* as templateFileCopy builds the path with that prefix */

      if (pathInfoExtCheck (pi, ".g")) {
        stpecpy (to, to + sizeof (to), tbuff);
      } else if (pathInfoExtCheck (pi, ".p")) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".txt")) {
        pathbldMakePath (to, sizeof (to), fname, "", PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".m")) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_HOSTNAME);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      } else if (fname [strlen (fname) - 1] == '~') {
        pathInfoFree (pi);
        continue;
      } else {
        /* unknown extension */
        fprintf (stderr, "unknown extension for bdjconfig %.*s\n", (int) pi->elen, pi->extension);
        pathInfoFree (pi);
        continue;
      }
    } else if (pathInfoExtCheck (pi, BDJ4_CONFIG_EXT) ||
        pathInfoExtCheck (pi, BDJ4_SEQUENCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PL_DANCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PLAYLIST_EXT) ) {

      fkey = -1;
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        fkey = LOCALE_KEY_STDROUNDS;
      }
      if (strncmp (pi->basename, "QueueDance", pi->blen) == 0) {
        fkey = LOCALE_KEY_QDANCE;
      }

      stpecpy (tbuff, tbuff + sizeof (tbuff), fname);
      stpecpy (from, from + sizeof (from), fname);

      /* localization specific filenames */
      if (fkey != -1) {
        const char  *tval;

// ### this is fetched from the localization.txt file.
// should be ok
        tval = localeGetStr (fkey);
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%.*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      if (strncmp (pi->basename, "ds-", 3) == 0) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_USEIDX);
      } else {
        stpecpy (to, to + sizeof (to), tbuff);
      }
    } else {
      /* unknown extension */
      pathInfoFree (pi);
      continue;
    }

    templateFileCopy (from, to);
    pathInfoFree (pi);
  }
  slistFree (dirlist);
}

void
instutilCopyHttpFiles (void)
{
  char    from [BDJ4_PATH_MAX];
  char    to [BDJ4_PATH_MAX];
  char    tmp [BDJ4_PATH_MAX];
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

  pathbldMakePath (from, sizeof (from),
      "http/ca.crt", "", PATHBLD_MP_DIR_MAIN);
  pathbldMakePath (to, sizeof (to),
      "ca.crt", "", PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);

  pathbldMakePath (from, sizeof (from),
      "http/server.crt", "", PATHBLD_MP_DIR_MAIN);
  pathbldMakePath (to, sizeof (to),
      "server.crt", "", PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);

  pathbldMakePath (from, sizeof (from),
      "http/server.key", "", PATHBLD_MP_DIR_MAIN);
  pathbldMakePath (to, sizeof (to),
      "server.key", "", PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);

  pathbldMakePath (tmp, sizeof (tmp), "", "", PATHBLD_MP_DIR_MAIN);
  snprintf (from, sizeof (from), "%s/http/mrc", tmp);
  pathbldMakePath (to, sizeof (to),
      "mrc", "", PATHBLD_MP_DREL_HTTP);
  *tbuff = '\0';
  if (isWindows ()) {
    /* the directory must exist for robocopy to work */
    diropMakeDir (to);
    pathDisplayPath (from, sizeof (from));
    pathDisplayPath (to, sizeof (to));
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
  } else {
    snprintf (tbuff, sizeof (tbuff), "rsync -aS '%s' '%s'", from, "http");
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);
}

void
instutilGetMusicDir (char *homemusicdir, size_t sz)
{
  const char    *home;
  char          *hp = homemusicdir;
  char          *hend = homemusicdir + sz;

  home = sysvarsGetStr (SV_HOME);

  if (isLinux ()) {
    const char  *targv [5];
    int         targc = 0;
    char        data [BDJ4_PATH_MAX];
    char        *prog;
    bool        found = false;

    prog = sysvarsGetStr (SV_PATH_XDGUSERDIR);
    if (prog != NULL && *prog) {
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
        stpecpy (hp, hend, data);
        found = true;
      }
    }
    if (! found) {
      char    tmp [BDJ4_PATH_MAX];

      *tmp = '\0';
      osGetEnv ("XDG_MUSIC_DIR", tmp, sizeof (tmp));
      if (*tmp) {
        stpecpy (hp, hend, tmp);
      } else {
        snprintf (homemusicdir, sz, "%s/Music", home);
      }
    }
  }

  if (isWindows ()) {
    char    data [BDJ4_PATH_MAX];

    snprintf (homemusicdir, sz, "%s/Music", home);
    osRegistryGet (
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Music", data, sizeof (data));
    if (*data) {
      /* windows returns the path with %USERPROFILE% */
      hp = stpecpy (hp, hend, home);
      hp = stpecpy (hp, hend, data + 13);
    } else {
      osRegistryGet (
          "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
          "My Music", data, sizeof (data));
      if (*data) {
        /* windows returns the path with %USERPROFILE% */
        hp = stpecpy (hp, hend, home);
        hp = stpecpy (hp, hend, data + 13);
      }
    }
  }

  if (isMacOS ()) {
    /* macos does not localize the directory name */
    /* it has some weird localization process for the finder application */
    snprintf (homemusicdir, sz, "%s/Music", home);
  }

  pathNormalizePath (homemusicdir, sz);
}

void
instutilAppendNameToTarget (char *buff, size_t sz, const char *name, int macosonly)
{
  pathinfo_t  *pi;
  const char  *nm;
  const char  *suffix;
  char        fname [100];
  int         rc;
  char        *p = buff;
  char        *end = buff + sz;

  if (macosonly && ! isMacOS ()) {
    return;
  }

  pi = pathInfo (buff);
  nm = name;
  if (name == NULL || ! *name) {
    nm = BDJ4_NAME;
  }
  suffix = "";
  if (isMacOS ()) {
    suffix = MACOS_APP_EXT;
  }

  snprintf (fname, sizeof (fname), "%s%s", nm, suffix);
  rc = strncmp (pi->filename, fname, pi->flen) == 0 &&
      pi->flen == strlen (fname);
  if (! rc) {
    stringTrimChar (buff, '/');
    p = buff + strlen (buff);
    p = stpecpy (p, end, "/");
    p = stpecpy (p, end, fname);
  }
  pathInfoFree (pi);
}

/* checks for an existing BDJ4 installation */
/* on windows, the alternate installation does not have a bin/ directory */
/* check for the VERSION.txt file */
/* the VERSION.txt exists for both an alternate installation and for */
/* a standard installation */
bool
instutilCheckForExistingInstall (const char *dir, const char *macospfx)
{
  char        tbuff [BDJ4_PATH_MAX];
  bool        exists;

  snprintf (tbuff, sizeof (tbuff), "%s%s/VERSION.txt",
      dir, macospfx);
  exists = fileopFileExists (tbuff);

  return exists;
}

/* checks for a standard installation */
/* alternate installations do not have a locale/ dir, check for that */
bool
instutilIsStandardInstall (const char *dir, const char *macospfx)
{
  char        tbuff [BDJ4_PATH_MAX];
  bool        exists;

  snprintf (tbuff, sizeof (tbuff), "%s%s/locale", dir, macospfx);
  exists = fileopIsDirectory (tbuff);

  return exists;
}

void
instutilOldVersionString (sysversinfo_t *versinfo, char *buff, size_t sz)
{
  char    *rlvl;
  char    *p = buff + strlen (buff);
  char    *end = buff + sz;

  p = stpecpy (p, end, versinfo->version);
  rlvl = versinfo->releaselevel;
  if (strcmp (versinfo->releaselevel, "production") == 0) {
    rlvl = "";
  }
  if (*rlvl) {
    p = stpecpy (p, end, "-");
    p = stpecpy (p, end, rlvl);
  }
  if (*versinfo->dev) {
    p = stpecpy (p, end, "-");
    p = stpecpy (p, end, versinfo->dev);
  }
  if (*versinfo->builddate) {
    p = stpecpy (p, end, "-");
    p = stpecpy (p, end, versinfo->builddate);
  }
  if (*versinfo->build) {
    p = stpecpy (p, end, "-");
    p = stpecpy (p, end, versinfo->build);
  }
}

void
instutilInstallCleanTmp (const char *rundir)
{
  if (isWindows ()) {
    char    tfn [BDJ4_PATH_MAX];

    /* windows now uses utility/bdj4winstartup */
    snprintf (tfn, sizeof (tfn), "%s/bdj4clean.bat",
        sysvarsGetStr (SV_DIR_CONFIG));
    fileopDelete (tfn);
  }

  /* 2024-10-13 */
  /* it turns out, with empirical testing that linux supports @reboot also, */
  /* despite a lack of documentation. */
  if (isMacOS () || isLinux ()) {
    const char  *targv [5];
    int         targc = 0;
    char        *cttext = NULL;
    size_t      sz = 0;
    char        tfn [BDJ4_PATH_MAX];
    char        tstr [BDJ4_PATH_MAX];
    FILE        *fh = NULL;
    const char  *tmp;

    tmp = sysvarsGetStr (SV_PATH_CRONTAB);
    if (tmp != NULL && *tmp) {
      targv [targc++] = tmp;
      targv [targc++] = "-l";
      targv [targc++] = NULL;
      snprintf (tfn, sizeof (tfn), "/tmp/bdj4-ict.txt");
      osProcessStart (targv, OS_PROC_WAIT | OS_PROC_NOSTDERR, NULL, tfn);
      cttext = filedataReadAll (tfn, &sz);
      if (cttext == NULL) {
        cttext = mdstrdup ("");
      }
      snprintf (tstr, sizeof (tstr),
          "@reboot %s/bin/bdj4 --bdj4cleantmp\n", rundir);
      if (strstr (cttext, "bdj4cleantmp") == NULL) {
        fh = fopen (tfn, "a");
        mdextfopen (fh);
        if (fh != NULL) {
          fputs (tstr, fh);
          mdextfclose (fh);
          fclose (fh);
        }
        targc = 0;
        targv [targc++] = sysvarsGetStr (SV_PATH_CRONTAB);
        targv [targc++] = tfn;
        targv [targc++] = NULL;
        osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
      }
      fileopDelete (tfn);
      dataFree (cttext);
    }
  }
}

void
instutilCopyIcons (void)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        to [BDJ4_PATH_MAX];
  slist_t     *dirlist;
  slistidx_t  iteridx;
  const char  *fname;

  if (isWindows ()) {
    return;
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "", "", PATHBLD_MP_DIR_IMG);

  dirlist = dirlistBasicDirList (tbuff, BDJ4_IMG_SVG_EXT);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (strncmp (fname, "bdj4_icon_sq", 12) == 0) {
      continue;
    }
    if (strncmp (fname, "bdj4_icon", 9) != 0) {
      continue;
    }
    pathbldMakePath (tbuff, sizeof (tbuff), fname, "", PATHBLD_MP_DREL_IMG);
    pathbldMakePath (to, sizeof (to), "", "", PATHBLD_MP_DIR_ICON);
    diropMakeDir (to);
    pathbldMakePath (to, sizeof (to), fname, "", PATHBLD_MP_DIR_ICON);
    filemanipCopy (tbuff, to);
  }
  slistFree (dirlist);
}

/* internal routines */

static void
instutilCopyHttpSVGFile (const char *fn)
{
  char    from [BDJ4_PATH_MAX];
  char    to [BDJ4_PATH_MAX];

  pathbldMakePath (from, sizeof (from),
      fn, BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  pathbldMakePath (to, sizeof (to),
      fn, BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  filemanipCopy (from, to);
}

void
instutilCreateDataDirectories (void)
{
  char      path [BDJ4_PATH_MAX];

  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DREL_DATA);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME);
  diropMakeDir (path);
  pathbldMakePath (path, sizeof (path), "", "",
      PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  diropMakeDir (path);
}

