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
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "configui.h"
#include "log.h"
#include "mdebug.h"
#include "orgopt.h"
#include "slist.h"
#include "ui.h"

typedef struct conforg {
  orgopt_t    *orgopt;
} conforg_t;

void
confuiInitOrganization (confuigui_t *gui)
{
  slist_t     *tlist;
  slistidx_t  iteridx;
  const char  *disp = NULL;
  const char  *origpath;
  ilist_t     *ddlist;
  int         count;

  gui->org = mdmalloc (sizeof (conforg_t));
  gui->org->orgopt = orgoptAlloc ();
  tlist = orgoptGetList (gui->org->orgopt);

  /* save the old organization path */
  origpath = bdjoptGetStr (OPT_G_ORGPATH);
  bdjoptSetStr (OPT_G_OLDORGPATH, origpath);

  gui->uiitem [CONFUI_DD_ORGPATH].displist = tlist;
  gui->uiitem [CONFUI_DD_ORGPATH].listidx = 0;
  ddlist = ilistAlloc ("conforg", LIST_ORDERED);

  slistStartIterator (tlist, &iteridx);
  count = 0;
  while ((disp = slistIterateKey (tlist, &iteridx)) != NULL) {
    const char  *path;

    path = slistGetStr (tlist, disp);
    ilistSetStr (ddlist, count, DD_LIST_DISP, disp);
    ilistSetStr (ddlist, count, DD_LIST_KEY_STR, path);
    ilistSetNum (ddlist, count, DD_LIST_KEY_NUM, count);
    if (path != NULL && strcmp (path, origpath) == 0) {
      /* need this for the initial setup */
      gui->uiitem [CONFUI_DD_ORGPATH].listidx = count;
    }
    ++count;
  }

  gui->uiitem [CONFUI_DD_ORGPATH].ddlist = ddlist;
}

void
confuiCleanOrganization (confuigui_t *gui)
{
  if (gui->org->orgopt != NULL) {
    orgoptFree (gui->org->orgopt);
  }
  dataFree (gui->org);
}

void
confuiBuildUIOrganization (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* organization */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);

  /* CONTEXT: configuration: the audio file organization path */
  confuiMakeItemDropdown (gui, vbox, szgrp, _("Organisation Path"),
      CONFUI_DD_ORGPATH, OPT_G_ORGPATH,
      confuiOrgPathSelect);

  /* CONTEXT: configuration: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (gui, vbox, szgrp, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, CONFUI_INDENT_FIELD);
  for (int i = CONFUI_WIDGET_AO_EXAMPLE_2; i < CONFUI_WIDGET_AO_EXAMPLE_MAX; ++i) {
    confuiMakeItemLabelDisp (gui, vbox, szgrp, "", i, CONFUI_INDENT_FIELD);
  }

  confuiMakeItemSwitch (gui, vbox, szgrp,
      /* CONTEXT: configuration: checkbox: the database will load the dance from the audio file genre tag */
      _("Database Loads Dance From Genre"),
      CONFUI_SWITCH_DB_LOAD_FROM_GENRE, OPT_G_LOADDANCEFROMGENRE,
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE), NULL, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: checkbox: is automatic organization enabled */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Auto Organise"),
      CONFUI_SWITCH_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE), NULL, CONFUI_NO_INDENT);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}

