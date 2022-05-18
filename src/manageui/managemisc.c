#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "filemanip.h"
#include "fileop.h"
#include "manageui.h"
#include "pathbld.h"
#include "playlist.h"
#include "ui.h"

void
manageSetStatusMsg (UIWidget *statusMsg, const char *msg)
{
  uiLabelSetText (statusMsg, msg);
}


static void manageCopyPlaylistFiles (const char *oldname, const char *newname);

void
manageRenamePlaylistFiles (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);
}

void
manageCheckAndCreatePlaylist (const char *name,
    const char *suppfname, pltype_t pltype)
{
  char  onm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (onm) &&
      /* CONTEXT: name of the special song list for raffle songs */
      strcmp (name, _("Raffle Songs")) != 0) {
    playlist_t    *pl;

    pl = playlistAlloc (NULL);
    playlistCreate (pl, name, pltype, suppfname);
    playlistSave (pl);
    playlistFree (pl);
  }
}

bool
manageCreatePlaylistCopy (UIWidget *statusMsg,
    const char *oname, const char *newname)
{
  char  tbuff [MAXPATHLEN];
  bool  rc = true;

  pathbldMakePath (tbuff, sizeof (tbuff),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (fileopFileExists (tbuff)) {
    /* CONTEXT: failure status message */
    snprintf (tbuff, sizeof (tbuff), _("Copy already exists."));
    manageSetStatusMsg (statusMsg, tbuff);
    rc = false;
  }
  if (rc) {
    manageCopyPlaylistFiles (oname, newname);
  }
  return rc;
}

/* internal routines */

static void
manageCopyPlaylistFiles (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);
}


