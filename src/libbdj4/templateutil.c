/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"
#include "templateutil.h"

static void templateCopy (const char *fromdir, const char *fromfn, const char *to, const char *color);

void
templateImageCopy (const char *color)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        to [BDJ4_PATH_MAX];
  slist_t     *dirlist;
  slistidx_t  iteridx;
  const char  *fname;

  pathbldMakePath (tbuff, sizeof (tbuff), "img", "", PATHBLD_MP_DIR_TEMPLATE);

  dirlist = dirlistBasicDirList (tbuff, BDJ4_IMG_SVG_EXT);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    pathbldMakePath (to, sizeof (to), "", "", PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    diropMakeDir (to);
    pathbldMakePath (to, sizeof (to), fname, "", PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    templateCopy (tbuff, fname, to, color);
  }
  slistFree (dirlist);
}

void
templateFileCopy (const char *fromfn, const char *tofn)
{
  char    fromdir [BDJ4_PATH_MAX];
  char    to [BDJ4_PATH_MAX];

  pathbldMakePath (fromdir, sizeof (fromdir), "", "", PATHBLD_MP_DIR_TEMPLATE);
  pathbldMakePath (to, sizeof (to), tofn, "", PATHBLD_MP_DREL_DATA);
  templateCopy (fromdir, fromfn, to, NULL);
}

void
templateProfileCopy (const char *fromfn, const char *tofn)
{
  char    fromdir [BDJ4_PATH_MAX];
  char    to [BDJ4_PATH_MAX];

  pathbldMakePath (fromdir, sizeof (fromdir), "", "", PATHBLD_MP_DIR_TEMPLATE);
  pathbldMakePath (to, sizeof (to), tofn, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  templateCopy (fromdir, fromfn, to, NULL);
}

void
templateHttpCopy (const char *fromfn, const char *tofn)
{
  char    from [BDJ4_PATH_MAX];
  char    to [BDJ4_PATH_MAX];

  pathbldMakePath (from, sizeof (from), "", "", PATHBLD_MP_DIR_TEMPLATE);
  pathbldMakePath (to, sizeof (to), tofn, "", PATHBLD_MP_DREL_HTTP);
  templateCopy (from, fromfn, to, NULL);
}

void
templateDisplaySettingsCopy (void)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        from [BDJ4_PATH_MAX];
  char        to [BDJ4_PATH_MAX];
  slist_t     *dirlist;
  slistidx_t  iteridx;
  const char  *fname;

  pathbldMakePath (tbuff, sizeof (tbuff), "", "", PATHBLD_MP_DIR_TEMPLATE);

  dirlist = dirlistBasicDirList (tbuff, BDJ4_CONFIG_EXT);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (strncmp (fname, "ds-", 3) != 0) {
      continue;
    }

    pathbldMakePath (from, sizeof (from), "", "", PATHBLD_MP_DIR_TEMPLATE);
    pathbldMakePath (to, sizeof (to), fname, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
    templateCopy (from, fname, to, NULL);
  }
  slistFree (dirlist);
}

/* internal routines */

static void
templateCopy (const char *fromdir, const char *fromfn, const char *to, const char *color)
{
  char      tbuff [BDJ4_PATH_MAX];

  snprintf (tbuff, sizeof (tbuff), "%s/%s/%s", fromdir,
      sysvarsGetStr (SV_LOCALE), fromfn);
  if (! fileopFileExists (tbuff)) {
    snprintf (tbuff, sizeof (tbuff), "%s/%s/%s", fromdir,
        sysvarsGetStr (SV_LOCALE_SHORT), fromfn);
    if (! fileopFileExists (tbuff)) {
      snprintf (tbuff, sizeof (tbuff), "%s/%s", fromdir, fromfn);
      if (! fileopFileExists (tbuff)) {
        fprintf (stderr, "  not found: %s %s\n", fromdir, fromfn);
        return;
      }
    }
  }

  logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", tbuff);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);

  if (color == NULL ||
      strcmp (color, BDJ4_DFLT_DARK_ACCENT_COLOR) == 0) {
    filemanipBackup (to, 1);
    if (filemanipCopy (tbuff, to) < 0) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: copy failed %s %s", tbuff, to);
      fprintf (stderr, "ERR: copy failed: %s %s\n", tbuff, to);
    }
  } else {
    char    *data;
    FILE    *fh;
    size_t  len;
    char    *ndata;

    logMsg (LOG_INSTALL, LOG_IMPORTANT, "with-color: %s", color);

    filemanipBackup (to, 1);
    data = filedataReadAll (tbuff, &len);
    if (data != NULL) {
      fh = fileopOpen (to, "w");
      if (fh != NULL) {
        ndata = regexReplaceLiteral (data, BDJ4_DFLT_DARK_ACCENT_COLOR, color);
        len = strlen (ndata);
        fwrite (ndata, len, 1, fh);
        mdextfclose (fh);
        fclose (fh);
        dataFree (ndata);
      }
      dataFree (data);
    }
  }
}
