/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "uiutils.h"

bool
manageCreatePlaylistCopy (uiwcont_t *errorMsg,
    const char *oname, const char *newname)
{
  char  tbuff [MAXPATHLEN];
  bool  rc = true;

  if (playlistExists (newname)) {
    /* CONTEXT: manageui: failure status message */
    snprintf (tbuff, sizeof (tbuff), _("Copy already exists."));
    uiLabelSetText (errorMsg, tbuff);
    rc = false;
  }
  if (rc) {
    playlistCopy (oname, newname);
  }
  return rc;
}

void
manageDeletePlaylist (uiwcont_t *statusMsg, const char *name)
{
  char  tbuff [MAXPATHLEN];

  playlistDelete (name);

  snprintf (tbuff, sizeof (tbuff), "%s deleted.", name);
  uiLabelSetText (statusMsg, tbuff);
}

/* gets the entry value, trims spaces before and after. */
/* if the entry is empty, replace it with newname. */
char *
manageGetEntryValue (uiwcont_t *uientry)
{
  const char  *val;
  char        *tval;

  val = uiEntryGetValue (uientry);
  while (*val == ' ') {
    ++val;
  }
  tval = mdstrdup (val);
  stringTrimChar (tval, ' ');
  return tval;
}
