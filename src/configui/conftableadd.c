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
#include "bdjvarsdf.h"
#include "configui.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"

bool
confuiTableAdd (void *udata)
{
  confuigui_t       *gui = udata;
  uitree_t          *uitree = NULL;
  int               count;
  int               flags;
  bool              found = false;

  logProcBegin (LOG_PROC, "confuiTableAdd");

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "non-table");
    return UICB_STOP;
  }

  uitree = gui->tables [gui->tablecurr].uitree;
  if (uitree == NULL) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "no-tree");
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  count = uiTreeViewSelectGetCount (uitree);
  if (count == 1) {
    found = true;
  }

  if (found) {
    int     idx;

    idx = uiTreeViewSelectGetIndex (uitree);
    if (idx == 0 &&
        (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
      if (! uiTreeViewSelectNext (uitree)) {
        found = false;
      }
    }
  }

  if (! found) {
    uiTreeViewValueAppend (uitree);
  } else {
    uiTreeViewValueInsertBefore (uitree);
  }

  switch (gui->tablecurr) {
    case CONFUI_ID_DANCE: {
      dance_t     *dances;
      ilistidx_t  dkey;

      dances = bdjvarsdfGet (BDJVDF_DANCES);
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      dkey = danceAdd (dances, _("New Dance"));
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      confuiDanceSet (uitree, _("New Dance"), dkey);
      break;
    }

    case CONFUI_ID_GENRES: {
      /* CONTEXT: configuration: genre name that is set when adding a new genre */
      confuiGenreSet (uitree, TRUE, _("New Genre"), 0);
      break;
    }

    case CONFUI_ID_RATINGS: {
      /* CONTEXT: configuration: rating name that is set when adding a new rating */
      confuiRatingSet (uitree, TRUE, _("New Rating"), 0);
      break;
    }

    case CONFUI_ID_LEVELS: {
      /* CONTEXT: configuration: level name that is set when adding a new level */
      confuiLevelSet (uitree, TRUE, _("New Level"), 0, 0);
      break;
    }

    case CONFUI_ID_STATUS: {
      /* CONTEXT: configuration: status name that is set when adding a new status */
      confuiStatusSet (uitree, TRUE, _("New Status"), 0);
      break;
    }

    default: {
      break;
    }
  }

  uiTreeViewSelectCurrent (uitree);

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    ilistidx_t    key;

    key = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);
    confuiDanceSelectLoadValues (gui, key);
  }

  gui->tables [gui->tablecurr].changed = true;
  gui->tables [gui->tablecurr].currcount += 1;
  logProcEnd (LOG_PROC, "confuiTableAdd", "");
  return UICB_CONT;
}

