/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  char        *p = NULL;
  int         count;

  gui->org = mdmalloc (sizeof (conforg_t));
  gui->org->orgopt = orgoptAlloc ();
  tlist = orgoptGetList (gui->org->orgopt);

  gui->uiitem [CONFUI_COMBOBOX_ORGPATH].displist = tlist;
  slistStartIterator (tlist, &iteridx);
  gui->uiitem [CONFUI_COMBOBOX_ORGPATH].listidx = 0;
  count = 0;
  while ((p = slistIterateValueData (tlist, &iteridx)) != NULL) {
    if (p != NULL && strcmp (p, bdjoptGetStr (OPT_G_ORGPATH)) == 0) {
      gui->uiitem [CONFUI_COMBOBOX_ORGPATH].listidx = count;
      break;
    }
    ++count;
  }
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
  uiwidget_t    vbox;
  uiwidget_t    sg;

  logProcBegin (LOG_PROC, "confuiBuildUIOrganization");
  uiCreateVertBox (&vbox);

  /* organization */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the audio file organization path */
  confuiMakeItemCombobox (gui, &vbox, &sg, _("Organisation Path"),
      CONFUI_COMBOBOX_ORGPATH, OPT_G_ORGPATH,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_ORGPATH));
  /* CONTEXT: configuration: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (gui, &vbox, &sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  confuiMakeItemSwitch (gui, &vbox, &sg,
      /* CONTEXT: configuration: checkbox: the database will load the dance from the audio file genre tag */
      _("Database Loads Dance From Genre"),
      CONFUI_SWITCH_DB_LOAD_FROM_GENRE, OPT_G_LOADDANCEFROMGENRE,
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE), NULL, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: checkbox: is automatic organization enabled */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Auto Organise"),
      CONFUI_SWITCH_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE), NULL, CONFUI_NO_INDENT);
  logProcEnd (LOG_PROC, "confuiBuildUIOrganization", "");
}

