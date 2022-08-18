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
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "ui.h"
#include "validate.h"

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
manageCheckAndCreatePlaylist (const char *name, pltype_t pltype)
{
  char  onm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (onm)) {
    playlist_t    *pl;

    pl = playlistAlloc (NULL);
    playlistCreate (pl, name, pltype);
    playlistSave (pl, NULL);
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
    /* CONTEXT: manageui: failure status message */
    snprintf (tbuff, sizeof (tbuff), _("Copy already exists."));
    uiLabelSetText (statusMsg, tbuff);
    rc = false;
  }
  if (rc) {
    manageCopyPlaylistFiles (oname, newname);
  }
  return rc;
}

bool
managePlaylistExists (const char *name)
{
  char  tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  return fileopFileExists (tbuff);
}

int
manageValidateName (uientry_t *entry, void *udata)
{
  UIWidget    *statusMsg = udata;
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
manageDeletePlaylist (UIWidget *statusMsg, const char *name)
{
  char  tnm [MAXPATHLEN];
  char  tbuff [MAXPATHLEN];


  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  fileopDelete (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  fileopDelete (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  fileopDelete (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  fileopDelete (tnm);

  snprintf (tbuff, sizeof (tbuff), "%s deleted.", name);
  if (statusMsg != NULL) {
    uiLabelSetText (statusMsg, tbuff);
  }
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

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);
}
