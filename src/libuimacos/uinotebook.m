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

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uinotebook.h"

uiwcont_t *
uiCreateNotebook (void)
{
  uiwcont_t   *uiwidget;
  NSTabView   *nb;

  nb = [[NSTabView alloc] init];
  [nb setTabPosition : NSTabPositionTop];
//  gtk_notebook_set_show_border (GTK_NOTEBOOK (nb), TRUE);
//  gtk_widget_set_hexpand (nb, TRUE);
//  gtk_widget_set_vexpand (nb, FALSE);
  [nb setAutoresizingMask : NSViewWidthSizable | NSViewHeightSizable];

  uiwidget = uiwcontAlloc (WCONT_T_NOTEBOOK, WCONT_T_NOTEBOOK);
  uiwcontSetWidget (uiwidget, nb, NULL);

  nb.needsDisplay = true;

  return uiwidget;
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uibox,
    const char *label, uiwcont_t *image)
{
  NSTabView       *nb;
  NSTabViewItem   *tabv;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-append-page")) {
    return;
  }
  if (! uiwcontValid (uibox, WCONT_T_BOX, "nb-append-page-box")) {
    return;
  }
  if (uibox == NULL) {
    return;
  }

  nb = uinotebook->uidata.widget;
  tabv = [[NSTabViewItem alloc] init];
// ### will need to change to draw-label so that a custom tab w/pic can
// be displayed.
// or rather, just re-implement as uivnb was done
  if (label != NULL) {
    tabv.label = [NSString stringWithUTF8String : label];
  } else {
    [nb setTabViewType : NSNoTabsNoBorder];
  }

  [tabv setView : uibox->uidata.widget];
  [nb addTabViewItem : tabv];
  uibox->packed = true;

  nb.needsDisplay = true;

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
  [nb selectTabViewItemAtIndex : pagenum];
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

void
uiNotebookHideTabs (uiwcont_t *uinotebook)
{
  NSTabView   *nb;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-tabs")) {
    return;
  }

  nb = uinotebook->uidata.widget;
  [nb setTabViewType : NSNoTabsBezelBorder];
  [nb setTabPosition : NSTabPositionNone];
}

