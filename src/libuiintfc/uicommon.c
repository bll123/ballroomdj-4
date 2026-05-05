/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 *
 * various helper routines
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <ctype.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "songfav.h"
#include "sysvars.h"
#include "ui.h"
#include "uicommon.h"
#include "validate.h"

static _Atomic(bool) favclassinit = false;

int
uicommonValidatePlaylistNameClr (uiwcont_t *entry, const char *label, void *udata)
{
  uiwcont_t   *msgwidget = udata;

  uiLabelSetText (msgwidget, "");
  return uicommonValidatePlaylistName (entry, label, udata);
}

int
uicommonValidatePlaylistName (uiwcont_t *entry, const char *label, void *udata)
{
  uiwcont_t   *msgwidget = udata;
  int         rc;
  const char  *str;
  char        tbuff [200];
  int         flags = VAL_NOT_EMPTY | VAL_NO_SLASHES;
  bool        val;

  rc = UIENTRY_OK;
  str = uiEntryGetValue (entry);
  if (isWindows ()) {
    flags |= VAL_NO_WINCHARS;
  }
  val = validate (tbuff, sizeof (tbuff), label, str, flags);
  if (val == false) {
    uiLabelSetText (msgwidget, tbuff);
    rc = UIENTRY_ERROR;
  }

  return rc;
}

void
uicommonAddFavoriteClasses (void)
{
  int             count;
  const char      *name;
  const char      *color;
  songfav_t       *songfav;
  datafileconv_t  conv;

  if (favclassinit) {
    return;
  }

  songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  count = songFavoriteGetCount (songfav);
  for (int idx = 0; idx < count; ++idx) {
    char    tbuff [100];

    name = songFavoriteGetStr (songfav, idx, SONGFAV_NAME);
    color = songFavoriteGetStr (songfav, idx, SONGFAV_COLOR);
    if (name == NULL || ! *name || color == NULL || ! *color) {
      continue;
    }
    snprintf (tbuff, sizeof (tbuff), "bdj-sb.%s", name);
    uiAddColorClass (tbuff, color);
    uiLabelAddClass (name, color);
  }

  conv.invt = VALUE_STR;
  conv.str = "imported";
  songFavoriteConv (&conv);
  name = songFavoriteGetStr (songfav, conv.num, SONGFAV_NAME);
  color = songFavoriteGetStr (songfav, conv.num, SONGFAV_COLOR);
  if (name != NULL && *name && color != NULL && *color) {
    uiLabelAddClass (name, color);
  }

  conv.invt = VALUE_STR;
  conv.str = "removed";
  songFavoriteConv (&conv);
  name = songFavoriteGetStr (songfav, conv.num, SONGFAV_NAME);
  color = songFavoriteGetStr (songfav, conv.num, SONGFAV_COLOR);
  if (name != NULL && *name && color != NULL && *color) {
    uiLabelAddClass (name, color);
  }

  favclassinit = true;
}

void
uicommonProgressStatus (uiwcont_t *statusMsg, int count, int tot)
{
  char        tbuff [200];

  if (statusMsg == NULL) {
    return;
  }

  if (count < 0 && tot < 0) {
    *tbuff = '\0';
  } else if (count == 0 && tot == 0) {
    /* CONTEXT: please wait... status message */
    snprintf (tbuff, sizeof (tbuff), _("Please wait\xe2\x80\xa6"));
  } else {
    /* CONTEXT: please wait... (count/total) status message */
    snprintf (tbuff, sizeof (tbuff), _("Please wait\xe2\x80\xa6 (%1$d/%2$d)"), count, tot);
  }
  uiLabelSetText (statusMsg, tbuff);

  return;
}
