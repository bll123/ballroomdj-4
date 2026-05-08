/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uinotebook.h"

uiwcont_t *
uiCreateNotebook (void)
{
  uiwcont_t   *uiwidget;
  NSTabView   *nb;

 fprintf (stderr, "c-nb\n");
  nb = [[NSTabView alloc] init];
  nb.tabPosition = NSTabPositionTop;
  nb.tabViewType = NSNoTabsLineBorder;

  uiwidget = uiwcontAlloc (WCONT_T_NOTEBOOK, WCONT_T_NOTEBOOK);
  uiwcontSetWidget (uiwidget, nb, NULL);

  uiWidgetSetIdentifier (uiwidget, "nb");

  nb.needsDisplay = true;

  return uiwidget;
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uibox,
    uiwcont_t *uilabel)
{
  NSTabView     *nb;
  NSTabViewItem *tabv;
  ILabel        *lab;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-append-page")) {
    return;
  }
  if (uibox == NULL || uibox->packwidget == NULL) {
    return;
  }

  if (uibox->wbasetype != WCONT_T_BOX &&
      uibox->wtype != WCONT_T_GRID &&
      uibox->wtype != WCONT_T_WINDOW_SCROLL) {
    fprintf (stderr, "ERR: %s incorrect type actual:%d/%s\n",
        "nb-append-page-box",
        uibox->wbasetype, uiwcontDesc (uibox->wbasetype));
    return;
  }

  nb = uinotebook->widget;
  tabv = [[NSTabViewItem alloc] init];
  if (uilabel != NULL && uilabel->widget != NULL) {
    lab = uilabel->widget;
    tabv.label = [lab stringValue];
  }
  tabv.view = uibox->packwidget;
  [nb addTabViewItem : tabv];

  uibox->packed = true;

  return;
}

void
uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum)
{
  NSTabView   *nb;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-page")) {
    return;
  }

  nb = uinotebook->widget;
  if (pagenum < nb.numberOfTabViewItems) {
    [nb selectTabViewItemAtIndex : pagenum];
  } else {
    fprintf (stderr, "ERR: nb-set-page: %d pages: %ld\n", pagenum, nb.numberOfTabViewItems);
  }
  return;
}

void
uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-cb")) {
    return;
  }
  if (uicb == NULL) {
    return;
  }

  return;
}

