/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"      // needed for snprintf macro
#include "bdjvarsdf.h"
#include "oslocale.h"
#include "songfav.h"
#include "sysvars.h"
#include "ui.h"
#include "uiutils.h"
#include "validate.h"

enum {
  PROFILE_BOX_SZ = 26,
};

static bool favclassinit = false;

/* = as a side effect, hbox is set, and */
/* uiwidget is set to the profile color box (needed by bdj4starterui) */
void
uiutilsAddProfileColorDisplay (uiwcont_t *vboxp, uiutilsaccent_t *accent)
{
  uiwcont_t       *hbox;
  uiwcont_t       *cbox;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  cbox = uiCreateHorizBox ();
  uiWidgetSetSizeRequest (cbox, PROFILE_BOX_SZ, PROFILE_BOX_SZ);
  uiutilsSetProfileColor (cbox, NULL);
  uiBoxPackEnd (hbox, cbox);
  uiWidgetAlignHorizCenter (cbox);
  uiWidgetAlignVertCenter (cbox);
  uiWidgetSetMarginStart (cbox, 4);

  uiWidgetShowAll (hbox);

  accent->cbox = cbox;
  accent->hbox = hbox;
}

void
uiutilsSetProfileColor (uiwcont_t *uiwidgetp, const char *oldcolor)
{
  char        classnm [100];
  char        bclassnm [100];
  const char  *tcolor = NULL;

  tcolor = bdjoptGetStr (OPT_P_UI_PROFILE_COL);
  if (tcolor == NULL || ! *tcolor) {
    tcolor = "#ffa600";
  }

  if (oldcolor != NULL) {
    snprintf (classnm, sizeof (classnm), "profcol%s", oldcolor + 1);
    uiWidgetRemoveClass (uiwidgetp, classnm);
  }

  snprintf (classnm, sizeof (classnm), "profcol%s", tcolor + 1);
  /* macos will crash if just box.profcol%s is used! */
  snprintf (bclassnm, sizeof (bclassnm), "box.horizontal.profcol%s", tcolor + 1);
  /* the ui library has code to prevent duplicates */
  uiAddBGColorClass (bclassnm, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiWidgetAddClass (uiwidgetp, classnm);
}

const char *
uiutilsGetCurrentFont (void)
{
  const char  *tstr;

  tstr = bdjoptGetStr (OPT_M_UI_FONT);
  if (tstr == NULL || ! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  return tstr;
}

const char *
uiutilsGetListingFont (void)
{
  const char  *tstr;

  tstr = bdjoptGetStr (OPT_M_LISTING_FONT);
  if (tstr == NULL || ! *tstr) {
    tstr = bdjoptGetStr (OPT_M_UI_FONT);
    if (tstr == NULL || ! *tstr) {
      tstr = sysvarsGetStr (SV_FONT_DEFAULT);
    }
  }
  return tstr;
}

int
uiutilsValidatePlaylistName (uiwcont_t *entry, const char *label, void *udata)
{
  uiwcont_t   *msgwidget = udata;
  int         rc;
  const char  *str;
  char        tbuff [200];
  int         flags = VAL_NOT_EMPTY | VAL_NO_SLASHES;
  bool        val;

  uiLabelSetText (msgwidget, "");
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
uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot)
{
  char        tbuff [200];

  if (statusMsg == NULL) {
    return;
  }

  if (count < 0 && tot < 0) {
    uiLabelSetText (statusMsg, "");
  } else {
    /* CONTEXT: please wait... (count/total) status message */
    snprintf (tbuff, sizeof (tbuff), _("Please wait\xe2\x80\xa6 (%1$d/%2$d)"), count, tot);
    uiLabelSetText (statusMsg, tbuff);
  }

  return;
}

void
uiutilsNewFontSize (char *buff, size_t sz, const char *font, const char *style, int newsz)
{
  char        fontname [200];
  size_t      i;

  stpecpy (fontname, fontname + sizeof (fontname), font);

  i = strlen (fontname) - 1;
  while (i != 0 && (isdigit (fontname [i]) || isspace (fontname [i]))) {
    --i;
  }
  ++i;
  fontname [i] = '\0';

  if (style == NULL) {
    style = "";
  }
  snprintf (buff, sz, "%s %s %d", fontname, style, newsz);
}

void
uiutilsAddFavoriteClasses (void)
{
  int         count;
  const char  *name;
  const char  *color;
  songfav_t   *songfav;

  if (favclassinit) {
    return;
  }

  songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  count = songFavoriteGetCount (songfav);
  for (int idx = 0; idx < count; ++idx) {
    name = songFavoriteGetStr (songfav, idx, SONGFAV_NAME);
    color = songFavoriteGetStr (songfav, idx, SONGFAV_COLOR);
    if (name == NULL || ! *name) {
      continue;
    }
    uiSpinboxAddClass (name, color);
    uiLabelAddClass (name, color);
  }

  favclassinit = true;
}

