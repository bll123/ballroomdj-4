/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "podcastutil.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "ui.h"
#include "uiutils.h"

bool
manageCreatePlaylistCopy (uiwcont_t *errorMsg,
    const char *oname, const char *newname)
{
  char  tbuff [BDJ4_PATH_MAX];
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
manageDeletePlaylist (musicdb_t *musicdb, const char *name)
{
  pltype_t    pltype;

  pltype = playlistGetType (name);
  if (pltype == PLTYPE_PODCAST) {
    /* remove all song in the podcast from the database */
    podcastutilDelete (musicdb, name);
  }

  playlistDelete (name);
}

void
manageDeleteStatus (uiwcont_t *statusMsg, const char *name)
{
  char  tbuff [BDJ4_PATH_MAX];

  /* CONTEXT: manageui: status message when deleting a song list/playlist/sequence */
  snprintf (tbuff, sizeof (tbuff), _("%s deleted."), name);
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
