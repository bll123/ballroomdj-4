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
#include <stdarg.h>

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
#include "mdebug.h"
#include "slist.h"
#include "ui.h"

bool
confuiDanceSelect (void *udata, long col)
{
  confuigui_t   *gui = udata;
  uiwcont_t     *uitree = NULL;
  ilistidx_t    key;

  logProcBegin (LOG_PROC, "confuiDanceSelect");
  gui->inchange = true;

  uitree = gui->tables [CONFUI_ID_DANCE].uitree;
  if (uitree == NULL) {
    return UICB_CONT;
  }

  uiTreeViewSelectCurrent (uitree);

  key = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);
  confuiDanceSelectLoadValues (gui, key);
  gui->inchange = false;
  logProcEnd (LOG_PROC, "confuiDanceSelect", "");
  return UICB_CONT;
}

void
confuiDanceSelectLoadValues (confuigui_t *gui, ilistidx_t danceIdx)
{
  dance_t         *dances;
  const char      *sval;
  slist_t         *slist;
  datafileconv_t  conv;
  int             widx;
  nlistidx_t      num;
  int             timesig;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, danceIdx, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  /* because the same entry field is used when switching dances, */
  /* and there is a validation timer running, */
  /* the validation timer must be cleared */
  /* the entry field does not need to be validated when being loaded */
  /* this applies to the dance, tags and announcement */
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  slist = danceGetList (dances, danceIdx, DANCE_TAGS);
  conv.list = slist;
  conv.invt = VALUE_LIST;
  convTextList (&conv);
  sval = conv.strval;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  dataFree (conv.strval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  timesig = danceGetTimeSignature (danceIdx);

  sval = danceGetStr (dances, danceIdx, DANCE_ANNOUNCE);
  widx = CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  num = danceGetNum (dances, danceIdx, DANCE_MPM_HIGH);
  widx = CONFUI_WIDGET_DANCE_MPM_HIGH;
  num = danceConvertMPMtoBPM (danceIdx, num);
  uiSpinboxSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = danceGetNum (dances, danceIdx, DANCE_MPM_LOW);
  widx = CONFUI_WIDGET_DANCE_MPM_LOW;
  num = danceConvertMPMtoBPM (danceIdx, num);
  uiSpinboxSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = danceGetNum (dances, danceIdx, DANCE_SPEED);
  widx = CONFUI_SPINBOX_DANCE_SPEED;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);

  widx = CONFUI_SPINBOX_DANCE_TIME_SIG;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, timesig);

  num = danceGetNum (dances, danceIdx, DANCE_TYPE);
  widx = CONFUI_SPINBOX_DANCE_TYPE;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);
}
