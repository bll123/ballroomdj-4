/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
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
  uiwcont_t        *parentwinp;
  uiwcont_t         *selfileDialog;
  uitree_t          *selfiletree;
  callback_t        *rowactivecb;
  callback_t        *respcb;
  callback_t        *selfilecb;
  nlist_t           *options;
} uiselectfile_t;

static void selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb);
static bool selectFileSelect (void *udata, long col);
static bool selectFileResponseHandler (void *udata, long responseid);
static bool selectFileCallback (uisfcb_t *uisfcb, const char *disp, const char *mimetype);

void
selectFileDialog (int type, uiwcont_t *window, nlist_t *options,
    callback_t *cb)
{
  uiselectfile_t *selectfile;
  slist_t     *filelist;
  int         x, y;
  int         playlistSel;
  const char  *title;

  selectfile = mdmalloc (sizeof (uiselectfile_t));
  selectfile->parentwinp = window;
  selectfile->selfileDialog = NULL;
  selectfile->selfiletree = NULL;
  selectfile->rowactivecb = NULL;
  selectfile->respcb = NULL;
  selectfile->selfilecb = NULL;
  selectfile->options = options;
  selectfile->respcb = callbackInitLong (
      selectFileResponseHandler, selectfile);

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
    filelist = playlistGetPlaylistList (playlistSel, NULL);

    selectFileCreateDialog (selectfile, filelist, title, cb);
    uiDialogShow (selectfile->selfileDialog);

    x = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_X);
    y = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y);
    uiWindowMove (selectfile->selfileDialog, x, y, -1);

    slistFree (filelist);
  }
}

void
selectFileFree (uiselectfile_t *selectfile)
{
  if (selectfile != NULL) {
    uiwcontFree (selectfile->selfileDialog);
    callbackFree (selectfile->respcb);
    callbackFree (selectfile->rowactivecb);
    uiTreeViewFree (selectfile->selfiletree);
    mdfree (selectfile);
  }
}

bool
selectAudioFileCallback (void *udata)
{
  uisfcb_t    *uisfcb = udata;
  int         rc;

  rc = selectFileCallback (uisfcb,
      /* CONTEXT: select audio file: file selection dialog: audio file filter */
      _("Audio Files"), "audio/*");
  return rc;
}

bool
selectAllFileCallback (void *udata)
{
  uisfcb_t    *uisfcb = udata;
  int         rc;

  rc = selectFileCallback (uisfcb, NULL, NULL);
  return rc;
}

/* internal routines */

static void
selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *uitreewidgetp;
  uiwcont_t     *scwindow;
  char          tbuff [200];
  slistidx_t    fliteridx;
  char          *disp;


  selectfile->selfilecb = cb;

  /* CONTEXT: select file: title of window: select <file-type> */
  snprintf (tbuff, sizeof (tbuff), _("Select %s"), filetype);
  selectfile->selfileDialog = uiCreateDialog (
      selectfile->parentwinp, selectfile->respcb, tbuff,
      /* CONTEXT: select file: closes the dialog */
      _("Close"), RESPONSE_CLOSE,
      /* CONTEXT: select file: selects the file */
      _("Select"), RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (selectfile->selfileDialog, vbox);

  scwindow = uiCreateScrolledWindow (200);
  uiWidgetExpandHoriz (scwindow);
  uiWidgetExpandVert (scwindow);
  uiBoxPackStartExpand (vbox, scwindow);

  selectfile->selfiletree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetWidgetContainer (selectfile->selfiletree);
  uiTreeViewDisableSingleClick (selectfile->selfiletree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetAlignVertFill (uitreewidgetp);
  uiBoxPackInWindow (scwindow, uitreewidgetp);

  uiwcontFree (scwindow);

  uiTreeViewCreateValueStore (selectfile->selfiletree, SELFILE_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_END);

  slistStartIterator (filelist, &fliteridx);
  while ((disp = slistIterateKey (filelist, &fliteridx)) != NULL) {
    uiTreeViewValueAppend (selectfile->selfiletree);
    uiTreeViewSetValues (selectfile->selfiletree,
        SELFILE_COL_DISP, disp,
        SELFILE_COL_SB_PAD, "  ",
        TREE_VALUE_END);
  }

  uiTreeViewAppendColumn (selectfile->selfiletree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_TEXT, SELFILE_COL_DISP, TREE_COL_TYPE_END);
  uiTreeViewAppendColumn (selectfile->selfiletree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_TEXT, SELFILE_COL_SB_PAD, TREE_COL_TYPE_END);

  selectfile->rowactivecb = callbackInitLong (selectFileSelect, selectfile);
  uiTreeViewSetRowActivatedCallback (selectfile->selfiletree, selectfile->rowactivecb);

  /* the dialog doesn't have any space above the buttons */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
}

static bool
selectFileSelect (void *udata, long col)
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
  char          *str;
  int           count;

  uiWindowGetPosition (selectfile->selfileDialog, &x, &y, &ws);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_X, x);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      selectfile->selfilecb = NULL;
      uiwcontFree (selectfile->selfileDialog);
      selectfile->selfileDialog = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      uiDialogDestroy (selectfile->selfileDialog);
      uiwcontFree (selectfile->selfileDialog);
      selectfile->selfileDialog = NULL;
      selectfile->selfilecb = NULL;
      break;
    }
    case RESPONSE_APPLY: {
      count = uiTreeViewSelectGetCount (selectfile->selfiletree);
      if (count != 1) {
        break;
      }

      str = uiTreeViewGetValueStr (selectfile->selfiletree, SELFILE_COL_DISP);
      uiDialogDestroy (selectfile->selfileDialog);
      uiwcontFree (selectfile->selfileDialog);
      selectfile->selfileDialog = NULL;
      if (selectfile->selfilecb != NULL) {
        callbackHandlerStr (selectfile->selfilecb, str);
      }
      dataFree (str);
      selectfile->selfilecb = NULL;
      break;
    }
  }

  selectFileFree (selectfile);
  return UICB_CONT;
}



static bool
selectFileCallback (uisfcb_t *uisfcb, const char *disp, const char *mimetype)
{
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [100];

  if (uisfcb->title == NULL) {
    /* CONTEXT: select audio file: dialog title for selecting audio files */
    snprintf (tbuff, sizeof (tbuff), _("Select Audio File"));
  }
  selectdata = uiDialogCreateSelect (uisfcb->window,
      tbuff,
      bdjoptGetStr (OPT_M_DIR_MUSIC),
      uiEntryGetValue (uisfcb->entry),
      disp, mimetype);
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (uisfcb->entry, fn);
    mdfree (fn);
  }
  mdfree (selectdata);
  return UICB_CONT;
}

