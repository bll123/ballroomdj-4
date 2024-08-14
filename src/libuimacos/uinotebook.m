/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uinotebook.h"

uiwcont_t *
uiCreateNotebook (void)
{
  uiwcont_t   *uiwidget;
  NSTabView   *nb;

  nb = [[NSTabView alloc] init];
  [nb setTabPosition: NSTabPositionTop];
//  gtk_notebook_set_show_border (GTK_NOTEBOOK (nb), TRUE);
//  gtk_nb_set_margin_top (nb, uiBaseMarginSz * 2);
//  gtk_widget_set_hexpand (nb, TRUE);
//  gtk_widget_set_vexpand (nb, FALSE);
//  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (nb), GTK_POS_TOP);

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_NOTEBOOK;
  uiwidget->wtype = WCONT_T_NOTEBOOK;
  uiwidget->uidata.widget = nb;
  uiwidget->uidata.packwidget = nb;
  return uiwidget;
}

void
uiNotebookTabPositionLeft (uiwcont_t *uinotebook)
{
  NSTabView     *nb;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-tab-left")) {
    return;
  }

  nb = uinotebook->uidata.widget;
  [nb setTabPosition: NSTabPositionLeft];
  return;
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uibox,
    uiwcont_t *uilabel)
{
  NSTabView       *nb;
  NSTabViewItem         *tabv;
  NSTextField     *lab;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-append-page")) {
    return;
  }
  if (! uiwcontValid (uibox, WCONT_T_BOX, "nb-append-page-box")) {
    return;
  }
  if (uibox == NULL) {
    return;
  }

  tabv = [[NSTabViewItem alloc] init];
// ### will need to change to draw-label so that a custom tab w/pic can
// be displayed.
// macos position-left also needs to have horizontal tabs.
  lab = uilabel->uidata.widget;
  tabv.label = [lab stringValue];
  [tabv setView: uibox->uidata.widget];
  nb = uinotebook->uidata.widget;
  [nb addTabViewItem: tabv];

  return;
}

void
uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-action-widget")) {
    return;
  }
  if (uiwidget == NULL) {
    return;
  }

// ### I don't know if this is possible on macos

  return;
}

void
uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum)
{
  NSTabView   *nb;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-page")) {
    return;
  }

  nb = uinotebook->uidata.widget;
  [nb selectTabViewItemAtIndex: pagenum];
  return;
}

void
uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-hide-show")) {
    return;
  }

// ### is this possible on macos?
// it's an nsview?, it should be possible.
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

