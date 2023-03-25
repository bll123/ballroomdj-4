/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "filemanip.h"
#include "fileop.h"
#include "manageui.h"
#include "mdebug.h"
#include "pathbld.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "ui.h"
#include "validate.h"

bool
manageCreatePlaylistCopy (uiwcont_t *statusMsg,
    const char *oname, const char *newname)
{
  char  tbuff [MAXPATHLEN];
  bool  rc = true;

  if (playlistExists (newname)) {
    /* CONTEXT: manageui: failure status message */
    snprintf (tbuff, sizeof (tbuff), _("Copy already exists."));
    uiLabelSetText (statusMsg, tbuff);
    rc = false;
  }
  if (rc) {
    playlistCopy (oname, newname);
  }
  return rc;
}

int
manageValidateName (uientry_t *entry, void *udata)
{
  uiwcont_t  *statusMsg = udata;
  int         rc;
  const char  *str;
  char        tbuff [200];
  const char  *valstr;

  rc = UIENTRY_OK;
  if (statusMsg != NULL) {
    uiLabelSetText (statusMsg, "");
  }
  str = uiEntryGetValue (entry);
  valstr = validate (str, VAL_NOT_EMPTY | VAL_NO_SLASHES);
  if (valstr != NULL) {
    if (statusMsg != NULL) {
      snprintf (tbuff, sizeof (tbuff), valstr, str);
      uiLabelSetText (statusMsg, tbuff);
    }
    rc = UIENTRY_ERROR;
  }

  return rc;
}

void
manageDeletePlaylist (uiwcont_t *statusMsg, const char *name)
{
  char  tbuff [MAXPATHLEN];

  playlistDelete (name);

  snprintf (tbuff, sizeof (tbuff), "%s deleted.", name);
  if (statusMsg != NULL) {
    uiLabelSetText (statusMsg, tbuff);
  }
}

char *
manageTrimName (const char *name)
{
  char  *tname;

  while (*name == ' ') {
    ++name;
  }
  tname = mdstrdup (name);
  stringTrimChar (tname, ' ');
  return tname;
}
