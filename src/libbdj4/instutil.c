/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "sysvars.h"
#include "templateutil.h"
#include "webclient.h"

instati_t instati [INST_ATI_MAX] = {
  [INST_ATI_BDJ4] = { "libatibdj4", false },
  [INST_ATI_MUTAGEN] = { "libatimutagen", true },
};

typedef struct {
  const char      *webresponse;
  size_t          webresplen;
} instweb_t;

static void instutilCopyHttpSVGFile (const char *fn);
static void instutilWebResponseCallback (void *userdata, const char *resp, size_t len);

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
      snprintf (buff, sizeof (buff), "%s/bin/bdj4.exe", target);
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
  slistidx_t  iteridx;
  const char  *fname;
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
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
      strlcpy (from, fname, sizeof (from));
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);

      /* use pathbldMakePath without specifying PATHBLD_MP_DREL_DATA */
      /* as templateFileCopy builds the path with that prefix */

      if (pathInfoExtCheck (pi, ".g")) {
        strlcpy (to, tbuff, sizeof (to));
      } else if (pathInfoExtCheck (pi, ".p")) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".txt")) {
        pathbldMakePath (to, sizeof (to), fname, "", PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".m")) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_HOSTNAME);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      } else {
        /* unknown extension */
        fprintf (stderr, "unknown extension for bdjconfig %.*s\n", (int) pi->elen, pi->extension);
        pathInfoFree (pi);
        continue;
      }
    } else if (pathInfoExtCheck (pi, BDJ4_CONFIG_EXT) ||
        pathInfoExtCheck (pi, BDJ4_CSS_EXT) ||
        pathInfoExtCheck (pi, BDJ4_SEQUENCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PL_DANCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PLAYLIST_EXT) ) {

      strlcpy (tbuff, fname, sizeof (tbuff));

      fkey = -1;
      if (strncmp (pi->basename, "automatic", pi->blen) == 0) {
        fkey = LOCALE_KEY_AUTO;
      }
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        fkey = LOCALE_KEY_STDROUNDS;
      }
      if (strncmp (pi->basename, "QueueDance", pi->blen) == 0) {
        fkey = LOCALE_KEY_QDANCE;
      }

      strlcpy (tbuff, fname, sizeof (tbuff));
      strlcpy (from, fname, sizeof (from));

      /* localization specific filenames */
      if (fkey != -1) {
        const char  *tval;

        tval = localeGetStr (fkey);
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%.*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      if (strncmp (pi->basename, "ds-", 3) == 0) {
        pathbldMakePath (to, sizeof (to), tbuff, "", PATHBLD_MP_USEIDX);
      } else {
        strlcpy (to, tbuff, sizeof (to));
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
    snprintf (tbuff, sizeof (tbuff), "rsync -aS '%s' '%s'", from, "http");
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

/* scan the music directory and determine which */
/* ati interface has the best support */
/* prefer bdj4 internal ati where possible */
void
instutilScanMusicDir (const char *musicdir, const char *rundir,
    char *ati, size_t atisz)
{
  slist_t     *mlist;
  slistidx_t  iteridx;
  const char  *fn;
  char        tbuff [MAXPATHLEN];
  int         tagtype;
  int         filetype;
  int         flags [AFILE_TYPE_MAX];
  int         supported [INST_ATI_MAX][AFILE_TYPE_MAX];
  int         suppval [INST_ATI_MAX];
  int         max;
  int         idx;

  for (int j = 0; j < AFILE_TYPE_MAX; ++j) {
    flags [j] = 0;
  }

  mlist = dirlistRecursiveDirList (musicdir, DIRLIST_FILES);
  slistStartIterator (mlist, &iteridx);
  while ((fn = slistIterateKey (mlist, &iteridx)) != NULL) {
    audiotagDetermineTagType (fn, &tagtype, &filetype);
    flags [filetype] = 1;
  }
  slistFree (mlist);

  /* recall that sysvars is not set up correctly */
  /* pathbld_mp_dir_exec must be set properly */
  /* the dynamically loaded libraries need this path */
  snprintf (tbuff, sizeof (tbuff), "%s/bin", rundir);
  sysvarsSetStr (SV_BDJ4_DIR_EXEC, tbuff);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "dir-exec set to %s", tbuff);

  for (int i = 0; i < INST_ATI_MAX; ++i) {
    atiGetSupportedTypes (instati [i].name, supported [i]);
  }

  for (int i = 0; i < INST_ATI_MAX; ++i) {
    suppval [i] = 0;

    for (int j = 0; j < AFILE_TYPE_MAX; ++j) {
      /* don't worry about unknown types */
      if (j == AFILE_TYPE_UNKNOWN) {
        continue;
      }
      if (flags [j] > 0) {
        if (supported [i][j] == ATI_READ_WRITE) {
          suppval [i] += 2;
        }
        if (supported [i][j] == ATI_READ) {
          suppval [i] += 1;
        }
      }
    }
  }

  /* want bdj4 internal ati if possible */
  idx = INST_ATI_BDJ4;
  max = suppval [idx];
  for (int i = 0; i < INST_ATI_MAX; ++i) {
    if (suppval [i] > max) {
      max = suppval [i];
      idx = i;
    }
  }

  strlcpy (ati, instati [idx].name, atisz);
}

void
instutilAppendNameToTarget (char *buff, size_t sz, int macosonly)
{
  pathinfo_t  *pi;
  const char  *nm;
  int         rc;

  if (macosonly && ! isMacOS ()) {
    return;
  }

  pi = pathInfo (buff);
  nm = BDJ4_NAME;
  if (isMacOS ()) {
    nm = BDJ4_MACOS_DIR;
  }

  rc = strncmp (pi->filename, nm, pi->flen) == 0 &&
      pi->flen == strlen (nm);
  if (! rc) {
    stringTrimChar (buff, '/');
    strlcat (buff, "/", sz);
    strlcat (buff, nm, sz);
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
  char        tbuff [MAXPATHLEN];
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
  char        tbuff [MAXPATHLEN];
  bool        exists;

  snprintf (tbuff, sizeof (tbuff), "%s%s/locale", dir, macospfx);
  exists = fileopIsDirectory (tbuff);

  return exists;
}

void
instutilRegister (const char *data)
{
  instweb_t     instweb;
  webclient_t   *webclient;
  char          uri [200];
  char          tbuff [4096];

  instweb.webresponse = NULL;
  instweb.webresplen = 0;
  webclient = webclientAlloc (&instweb, instutilWebResponseCallback);
  snprintf (uri, sizeof (uri), "%s/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), sysvarsGetStr (SV_URI_REGISTER));

  snprintf (tbuff, sizeof (tbuff),
      "key=%s"
      "&version=%s&build=%s&builddate=%s&releaselevel=%s"
      "&osname=%s&osdisp=%s&osvers=%s&osbuild=%s"
      "&pythonvers=%s"
      "&user=%s&host=%s"
      "&systemlocale=%s&locale=%s"
      "%s",
      "9873453",  // key
      sysvarsGetStr (SV_BDJ4_VERSION),
      sysvarsGetStr (SV_BDJ4_BUILD),
      sysvarsGetStr (SV_BDJ4_BUILDDATE),
      sysvarsGetStr (SV_BDJ4_RELEASELEVEL),
      sysvarsGetStr (SV_OSNAME),
      sysvarsGetStr (SV_OSDISP),
      sysvarsGetStr (SV_OSVERS),
      sysvarsGetStr (SV_OSBUILD),
      sysvarsGetStr (SV_PYTHON_DOT_VERSION),
      sysvarsGetStr (SV_USER),
      sysvarsGetStr (SV_HOSTNAME),
      sysvarsGetStr (SV_LOCALE_SYSTEM),
      sysvarsGetStr (SV_LOCALE),
      data
      );
  webclientPost (webclient, uri, tbuff);
  webclientClose (webclient);
}

void
instutilOldVersionString (sysversinfo_t *versinfo, char *buff, size_t sz)
{
  char    *rlvl;

  strlcat (buff, versinfo->version, sz);
  rlvl = versinfo->releaselevel;
  if (strcmp (versinfo->releaselevel, "production") == 0) {
    rlvl = "";
  }
  if (*rlvl) {
    strlcat (buff, "-", sz);
    strlcat (buff, rlvl, sz);
  }
  if (*versinfo->dev) {
    strlcat (buff, "-", sz);
    strlcat (buff, versinfo->dev, sz);
  }
  if (*versinfo->builddate) {
    strlcat (buff, "-", sz);
    strlcat (buff, versinfo->builddate, sz);
  }
  if (*versinfo->build) {
    strlcat (buff, "-", sz);
    strlcat (buff, versinfo->build, sz);
  }
}

void
instutilInstallCleanTmp (const char *rundir)
{
  if (isWindows ()) {
    char    *cttext;
    size_t  sz = 0;
    char    tbuff [MAXPATHLEN];
    char    tfn [MAXPATHLEN];
    FILE    *fh;

    strlcpy (tbuff, rundir, sizeof (tbuff));
    pathDisplayPath (tbuff, sizeof (tbuff));
    snprintf (tfn, sizeof (tfn), "%s/bdj4clean.bat",
        sysvarsGetStr (SV_DIR_CONFIG));
    cttext = filedataReadAll (tfn, &sz);
    if (cttext == NULL) {
      cttext = mdstrdup ("");
    }
    if (strstr (cttext, tbuff) == NULL) {
      char    *btmpl;
      char    *tmpl;

      btmpl = filedataReadAll ("install/win-cleantmp.bat", &sz);
      tmpl = regexReplaceLiteral (btmpl, "#BDJ4DIR#", tbuff);
      dataFree (btmpl);

      fh = fopen (tfn, "a");
      mdextfopen (fh);
      if (fh != NULL) {
        fwrite (tmpl, strlen (tmpl), 1, fh);
        mdextfclose (fh);
        fclose (fh);
      }
      dataFree (tmpl);
    }
    dataFree (cttext);
  }
  if (isMacOS ()) {
    const char  *targv [5];
    int         targc = 0;
    char        *cttext;
    size_t      sz = 0;
    char        tfn [MAXPATHLEN];
    char        tstr [MAXPATHLEN];
    FILE        *fh;

    targv [targc++] = sysvarsGetStr (SV_PATH_CRONTAB);
    targv [targc++] = "-l";
    targv [targc++] = NULL;
    snprintf (tfn, sizeof (tfn), "/tmp/bdj4-ict.txt");
    osProcessStart (targv, OS_PROC_WAIT, NULL, tfn);
    cttext = filedataReadAll (tfn, &sz);
    if (cttext == NULL) {
      cttext = mdstrdup ("");
    }
    snprintf (tstr, sizeof (tstr),
        "@reboot %s/bin/bdj4 --bdj4cleantmp\n", rundir);
    if (strstr (cttext, tstr) == NULL) {
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

static void
instutilWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  instweb_t *instweb = userdata;

  instweb->webresponse = resp;
  instweb->webresplen = len;
  return;
}

