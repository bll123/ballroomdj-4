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

#include "bdj4intl.h"
#include "bdj4ui.h"
#include "mdebug.h"
#include "nlist.h"
#include "playlist.h"
#include "ui.h"
#include "callback.h"
#include "uiselectfile.h"

enum {
  SELFILE_COL_DISP,
  SELFILE_COL_SB_PAD,
  SELFILE_COL_MAX,
};

typedef struct uiselectfile {
  UIWidget          *parentwinp;
  UIWidget          uidialog;
  uitree_t          *selfiletree;
  callback_t        *rowactivecb;
  callback_t        *respcb;
  callback_t        *selfilecb;
  nlist_t           *options;
} uiselectfile_t;

static void selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb);
static bool selectFileSelect (void *udata);
static bool selectFileResponseHandler (void *udata, long responseid);

void
selectFileDialog (int type, UIWidget *window, nlist_t *options,
    callback_t *cb)
{
  uiselectfile_t *selectfile;
  slist_t     *filelist;
  int         x, y;
  int         playlistSel;
  const char  *title;

  selectfile = mdmalloc (sizeof (uiselectfile_t));
  selectfile->parentwinp = window;
  uiutilsUIWidgetInit (&selectfile->uidialog);
  selectfile->selfiletree = NULL;
  selectfile->rowactivecb = NULL;
  selectfile->respcb = NULL;
  selectfile->selfilecb = NULL;
  selectfile->options = options;

  /* CONTEXT: select file: file type for the file selection dialog (song list) */
  title = _("Song List");

  playlistSel = PL_LIST_NORMAL; /* excludes queuedance */
  switch (type) {
    case SELFILE_SONGLIST: {
      playlistSel = PL_LIST_SONGLIST;
      break;
    }
    case SELFILE_SEQUENCE: {
      playlistSel = PL_LIST_SEQUENCE;
      /* CONTEXT: select file: file type for the file selection dialog (sequence) */
      title = _("Sequence");
      break;
    }
    case SELFILE_PLAYLIST: {
      playlistSel = PL_LIST_ALL;
      /* CONTEXT: select file: file type for the file selection dialog (playlist) */
      title = _("Playlist");
      break;
    }
  }

  if (cb != NULL) {
    filelist = playlistGetPlaylistList (playlistSel);

    selectFileCreateDialog (selectfile, filelist, title, cb);
    uiDialogShow (&selectfile->uidialog);

    x = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_X);
    y = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y);
    uiWindowMove (&selectfile->uidialog, x, y, -1);

    slistFree (filelist);
  }
}

void
selectFileFree (uiselectfile_t *selectfile)
{
  if (selectfile != NULL) {
    callbackFree (selectfile->respcb);
    callbackFree (selectfile->rowactivecb);
    uiTreeViewFree (selectfile->selfiletree);
    mdfree (selectfile);
  }
}

static void
selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uitreewidgetp;
  UIWidget      scwindow;
  char          tbuff [200];
  slistidx_t    fliteridx;
  char          *disp;


  selectfile->selfilecb = cb;

  selectfile->respcb = callbackInitLong (
      selectFileResponseHandler, selectfile);

  /* CONTEXT: select file: title of window: select <file-type> */
  snprintf (tbuff, sizeof (tbuff), _("Select %s"), filetype);
  uiCreateDialog (&selectfile->uidialog,
      selectfile->parentwinp, selectfile->respcb, tbuff,
      /* CONTEXT: select file: closes the dialog */
      _("Close"), RESPONSE_CLOSE,
      /* CONTEXT: select file: selects the file */
      _("Select"), RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiWidgetExpandVert (&vbox);
  uiDialogPackInDialog (&selectfile->uidialog, &vbox);

  uiCreateScrolledWindow (&scwindow, 200);
  uiWidgetExpandHoriz (&scwindow);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  selectfile->selfiletree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetUIWidget (selectfile->selfiletree);
  uiTreeViewDisableSingleClick (selectfile->selfiletree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetAlignVertFill (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);

  uiTreeViewCreateValueStore (selectfile->selfiletree, SELFILE_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_END);

  slistStartIterator (filelist, &fliteridx);
  while ((disp = slistIterateKey (filelist, &fliteridx)) != NULL) {
    uiTreeViewAppendValueStore (selectfile->selfiletree);
    uiTreeViewSetValues (selectfile->selfiletree,
        SELFILE_COL_DISP, disp,
        SELFILE_COL_SB_PAD, "  ",
        TREE_VALUE_END);
  }

  uiTreeViewAppendColumn (selectfile->selfiletree, TREE_COL_DISP_GROW, "",
      TREE_COL_MODE_TEXT, SELFILE_COL_DISP, TREE_COL_MODE_END);
  uiTreeViewAppendColumn (selectfile->selfiletree, TREE_COL_DISP_NORM, "",
      TREE_COL_MODE_TEXT, SELFILE_COL_SB_PAD, TREE_COL_MODE_END);

  selectfile->rowactivecb = callbackInit (selectFileSelect, selectfile, NULL);
  uiTreeViewSetRowActivatedCallback (selectfile->selfiletree, selectfile->rowactivecb);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);
}

static bool
selectFileSelect (void *udata)
{
  uiselectfile_t  *selectfile = udata;

  selectFileResponseHandler (selectfile, RESPONSE_APPLY);
  return UICB_CONT;
}

static bool
selectFileResponseHandler (void *udata, long responseid)
{
  uiselectfile_t  *selectfile = udata;
  int           x, y, ws;
  const char    *str;
  int           count;

  uiWindowGetPosition (&selectfile->uidialog, &x, &y, &ws);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_X, x);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      selectfile->selfilecb = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      uiCloseWindow (&selectfile->uidialog);
      selectfile->selfilecb = NULL;
      break;
    }
    case RESPONSE_APPLY: {
      count = uiTreeViewGetSelectCount (selectfile->selfiletree);
      if (count != 1) {
        break;
      }

      str = uiTreeViewGetValueStr (selectfile->selfiletree, SELFILE_COL_DISP);
      uiCloseWindow (&selectfile->uidialog);
      if (selectfile->selfilecb != NULL) {
        callbackHandlerStr (selectfile->selfilecb, str);
      }
      selectfile->selfilecb = NULL;
      break;
    }
  }

  selectFileFree (selectfile);
  return UICB_CONT;
}

