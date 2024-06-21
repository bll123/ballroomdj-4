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
  uiwcont_t         *uitree = NULL;
  uivirtlist_t      *uivl = NULL;
  int               count = 0;
  int               flags;
  bool              found = false;

  logProcBegin ();

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd ("non-table");
    return UICB_STOP;
  }

  uitree = gui->tables [gui->tablecurr].uitree;
  uivl = gui->tables [gui->tablecurr].uivl;
  if (uitree == NULL && uivl == NULL) {
    logProcEnd ("no-tree");
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  if (uitree != NULL) {
    count = uiTreeViewSelectGetCount (uitree);
  }
  if (uivl != NULL) {
    count = uivlSelectionCount (uivl);
  }
  if (count == 1) {
    found = true;
  }

  if (found) {
    int     idx;

    if (uitree != NULL) {
      idx = uiTreeViewSelectGetIndex (uitree);
    }
    if (uivl != NULL) {
      idx = uivlGetCurrSelection (uivl);
    }
    if (idx == 0 &&
        (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
      if (uitree != NULL) {
        if (! uiTreeViewSelectNext (uitree)) {
          found = false;
        }
      }
      if (uivl != NULL) {
        int   nidx;

        nidx = uivlMoveSelection (uivl, VL_DIR_DOWN);
        if (idx == nidx) {
          found = false;
        }
      }
    }
  }

  if (uitree != NULL) {
    if (! found) {
      uiTreeViewValueAppend (uitree);
    } else {
      uiTreeViewValueInsertBefore (uitree);
    }
  }

  switch (gui->tablecurr) {
    case CONFUI_ID_DANCE: {
      dance_t     *dances;
      ilistidx_t  dkey;

      dances = bdjvarsdfGet (BDJVDF_DANCES);
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      dkey = danceAdd (dances, _("New Dance"));
      danceSave (dances, NULL, -1);
      uivlSetNumRows (uivl, danceGetCount (dances));
      uivlPopulate (uivl);
      confuiDanceSearchSelect (gui, dkey);
      break;
    }

    case CONFUI_ID_GENRES: {
      /* CONTEXT: configuration: genre name that is set when adding a new genre */
      confuiGenreSet (uitree, true, _("New Genre"), 0);
      break;
    }

    case CONFUI_ID_RATINGS: {
      addfunc_t   addfunc;

      addfunc = gui->tables [gui->tablecurr].addfunc;
      if (addfunc != NULL) {
        addfunc (gui);
      }
      break;
    }

    case CONFUI_ID_LEVELS: {
      /* CONTEXT: configuration: level name that is set when adding a new level */
      confuiLevelSet (uitree, true, _("New Level"), 0, 0);
      break;
    }

    case CONFUI_ID_STATUS: {
      /* CONTEXT: configuration: status name that is set when adding a new status */
      confuiStatusSet (uitree, true, _("New Status"), 0);
      break;
    }

    default: {
      break;
    }
  }

  if (uitree != NULL) {
    uiTreeViewSelectCurrent (uitree);
  }

  gui->tables [gui->tablecurr].currcount += 1;
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd ("");
  return UICB_CONT;
}

