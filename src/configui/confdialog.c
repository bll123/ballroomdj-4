/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <stdarg.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "configui.h"
#include "log.h"
#include "quickedit.h"
#include "songfilter.h"
#include "tagdef.h"
#include "ui.h"

static int sftags [FILTER_DISP_MAX] = {
  [FILTER_DISP_GENRE] = TAG_GENRE,
  [FILTER_DISP_DANCE] = TAG_DANCE,
  [FILTER_DISP_DANCELEVEL] = TAG_DANCELEVEL,
  [FILTER_DISP_DANCERATING] = TAG_DANCERATING,
  [FILTER_DISP_STATUS] = TAG_STATUS,
  [FILTER_DISP_FAVORITE] = TAG_FAVORITE,
  /* status-playable is handled as a special case */
};

static_assert (sizeof (sftags) / sizeof (int) == FILTER_DISP_MAX,
    "missing filter-disp entry");

static int qetags [QUICKEDIT_DISP_MAX] = {
  [QUICKEDIT_DISP_SPEED] = TAG_SPEEDADJUSTMENT,
  [QUICKEDIT_DISP_VOLUME] = TAG_VOLUMEADJUSTPERC,
  [QUICKEDIT_DISP_DANCELEVEL] = TAG_DANCELEVEL,
  [QUICKEDIT_DISP_DANCERATING] = TAG_DANCERATING,
  [QUICKEDIT_DISP_FAVORITE] = TAG_FAVORITE,
};

static_assert (sizeof (qetags) / sizeof (int) == QUICKEDIT_DISP_MAX,
    "missing quickedit-disp entry");

void
confuiBuildUIDialogDisplay (confuigui_t *gui)
{
  uiwcont_t     *vbox;
  uiwcont_t     *szgrp;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  nlistidx_t    val;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* filter display */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: dialog display settings */
      _("Dialogs"), CONFUI_ID_NONE);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiwcontFree (vbox);

  /* column 1 */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  szgrp = uiCreateSizeGroupHoriz ();

  /* CONTEXT: configuration: filter display: song selection filters */
  uiwidgetp = uiCreateLabel (_("Filter Songs"));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  for (int i = 0; i < FILTER_DISP_MAX; ++i) {
    int         j;
    const char  *txt;

    j = CONFUI_WIDGET_FILTER_START + 1 + i;

    val = nlistGetNum (gui->filterDisplaySel, i);
    if (i == FILTER_DISP_STATUS_PLAYABLE) {
      /* CONTEXT: configuration: filter display: checkbox: status is playable */
      txt = _("Playable Status");
    } else {
      txt = tagdefs [sftags [i]].displayname;
    }
    confuiMakeItemCheckButton (gui, vbox, szgrp, txt, j, -1, val);
    gui->uiitem [j].outtype = CONFUI_OUT_CB;
  }

  uiwcontFree (szgrp);
  uiwcontFree (vbox);

  /* column 2 */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);
  uiwidgetp = uiCreateLabel ("   ");
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* column 3 */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  szgrp = uiCreateSizeGroupHoriz ();

  /* CONTEXT: configuration: filter display: quick edit filters */
  uiwidgetp = uiCreateLabel (_("Quick Edit"));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  for (int i = 0; i < QUICKEDIT_DISP_MAX; ++i) {
    int     j;

    j = CONFUI_WIDGET_QUICKEDIT_START + 1 + i;

    val = nlistGetNum (gui->quickeditDisplaySel, i);
    confuiMakeItemCheckButton (gui, vbox, szgrp,
        tagdefs [qetags [i]].displayname, j, -1, val);
    gui->uiitem [j].outtype = CONFUI_OUT_CB;
  }

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (hbox);

  logProcEnd ("");
}

