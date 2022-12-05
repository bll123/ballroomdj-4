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

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "datafile.h"
#include "dance.h"
#include "nlist.h"
#include "ilist.h"
#include "log.h"
#include "slist.h"
#include "ui.h"

void
confuiDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  confuigui_t   *gui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  gulong        idx = 0;
  ilistidx_t    key;
  int           widx;
  char          *sval;
  nlistidx_t    num;
  slist_t       *slist;
  datafileconv_t conv;
  dance_t       *dances;

  logProcBegin (LOG_PROC, "confuiDanceSelect");
  gui->indancechange = true;

  if (path == NULL) {
    gui->indancechange = false;
    return;
  }

  model = gtk_tree_view_get_model (tv);

  if (! gtk_tree_model_get_iter (model, &iter, path)) {
    logProcEnd (LOG_PROC, "confuiDanceSelect", "no model/iter");
    gui->indancechange = false;
    return;
  }
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, key, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiEntrySetValue (gui->uiitem [widx].entry, sval);

  slist = danceGetList (dances, key, DANCE_TAGS);
  conv.allocated = false;
  conv.list = slist;
  conv.valuetype = VALUE_LIST;
  convTextList (&conv);
  sval = conv.str;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiEntrySetValue (gui->uiitem [widx].entry, sval);
  if (conv.allocated) {
    free (conv.str);
    sval = NULL;
  }

  sval = danceGetStr (dances, key, DANCE_ANNOUNCE);
  widx = CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT;
  uiEntrySetValue (gui->uiitem [widx].entry, sval);

  num = danceGetNum (dances, key, DANCE_HIGH_BPM);
  widx = CONFUI_SPINBOX_DANCE_HIGH_BPM;
  uiSpinboxSetValue (&gui->uiitem [widx].uiwidget, num);

  num = danceGetNum (dances, key, DANCE_LOW_BPM);
  widx = CONFUI_SPINBOX_DANCE_LOW_BPM;
  uiSpinboxSetValue (&gui->uiitem [widx].uiwidget, num);

  num = danceGetNum (dances, key, DANCE_SPEED);
  widx = CONFUI_SPINBOX_DANCE_SPEED;
  uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, num);

  num = danceGetNum (dances, key, DANCE_TIMESIG);
  widx = CONFUI_SPINBOX_DANCE_TIME_SIG;
  uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, num);

  num = danceGetNum (dances, key, DANCE_TYPE);
  widx = CONFUI_SPINBOX_DANCE_TYPE;
  uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, num);

  gui->indancechange = false;
  logProcEnd (LOG_PROC, "confuiDanceSelect", "");
}

