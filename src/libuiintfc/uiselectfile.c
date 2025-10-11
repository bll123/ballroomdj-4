/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "uivirtlist.h"

enum {
  SELFILE_COL_DISP,
  SELFILE_COL_MAX,
};

enum {
  SELFILE_MAX_DISP = 10,
};

typedef struct uiselectfile {
  uiwcont_t         *parentwinp;
  uiwcont_t         *selfileDialog;
  uivirtlist_t      *uivl;
  callback_t        *respcb;
  callback_t        *selfilecb;
  nlist_t           *options;
  slist_t           *filelist;
  bool              inchange;
} uiselectfile_t;

static void selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb);
static bool selectFileResponseHandler (void *udata, int32_t responseid);
static bool selectFileCallback (uisfcb_t *uisfcb, const char *disp, const char *mimetype);
static void selectfileFillRow (void *udata, uivirtlist_t *uivl, int32_t rownum);
static void selectfileSelected (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

void
selectInitCallback (uisfcb_t *uisfcb)
{
  uisfcb->title = NULL;
  uisfcb->defdir = NULL;
  uisfcb->entry = NULL;
  uisfcb->window = NULL;
}

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
  selectfile->uivl = NULL;
  selectfile->respcb = NULL;
  selectfile->selfilecb = NULL;
  selectfile->options = options;
  selectfile->filelist = NULL;
  selectfile->inchange = false;

  selectfile->respcb = callbackInitI (
      selectFileResponseHandler, selectfile);

  /* CONTEXT: select file: file type for the file selection dialog (song list) */
  title = _("Song List");

  playlistSel = PL_LIST_NORMAL; /* excludes queuedance, history */
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

  if (cb == NULL) {
    return;
  }

  filelist = playlistGetPlaylistNames (playlistSel, NULL);
  slistFree (selectfile->filelist);
  selectfile->filelist = filelist;

  selectFileCreateDialog (selectfile, filelist, title, cb);
  uiDialogShow (selectfile->selfileDialog);

  x = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_X);
  y = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y);
  uiWindowMove (selectfile->selfileDialog, x, y, -1);
  uivlUpdateDisplay (selectfile->uivl);
}

void
selectFileFree (uiselectfile_t *selectfile)
{
  if (selectfile == NULL) {
    return;
  }

  uiwcontFree (selectfile->selfileDialog);
  callbackFree (selectfile->respcb);
  uivlFree (selectfile->uivl);
  slistFree (selectfile->filelist);
  selectfile->filelist = NULL;
  mdfree (selectfile);
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

/* internal routines */

static void
selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, callback_t *cb)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  char          tbuff [200];
  uivirtlist_t  *uivl;
  int           dispsize;
  slistidx_t    count;


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
  uiWindowSetNoMaximize (selectfile->selfileDialog);

  vbox = uiCreateVertBox ();
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (selectfile->selfileDialog, vbox);

  count = slistGetCount (filelist);
  dispsize = count;
  if (dispsize > SELFILE_MAX_DISP) {
    dispsize = SELFILE_MAX_DISP;
  }

  uivl = uivlCreate ("selectfile", NULL, vbox, dispsize, VL_NO_WIDTH,
      VL_NO_HEADING | VL_ENABLE_KEYS);
  selectfile->uivl = uivl;

  uivlSetNumColumns (uivl, SELFILE_COL_MAX);
  uivlMakeColumn (uivl, "disp", SELFILE_COL_DISP, VL_TYPE_LABEL);
  uivlSetColumnMinWidth (uivl, SELFILE_COL_DISP, 10);
  uivlSetNumRows (uivl, count);
  uivlSetRowFillCallback (uivl, selectfileFillRow, selectfile);
  uivlSetAllowDoubleClick (uivl);
  uivlSetDoubleClickCallback (uivl, selectfileSelected, selectfile);

  uivlDisplay (uivl);

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
selectFileResponseHandler (void *udata, int32_t responseid)
{
  uiselectfile_t  *selectfile = udata;
  int             x, y, ws;
  char            *str = NULL;
  int             count;

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
      int32_t     sel;

      count = uivlSelectionCount (selectfile->uivl);
      if (count != 1) {
        break;
      }

      sel = uivlGetCurrSelection (selectfile->uivl);
      str = slistGetDataByIdx (selectfile->filelist, sel);
      uiDialogDestroy (selectfile->selfileDialog);
      uiwcontFree (selectfile->selfileDialog);
      selectfile->selfileDialog = NULL;
      if (selectfile->selfilecb != NULL) {
        callbackHandlerS (selectfile->selfilecb, str);
      }
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
  const char  *defdir = NULL;

  if (uisfcb->title == NULL) {
    /* CONTEXT: select audio file: dialog title for selecting audio files */
    snprintf (tbuff, sizeof (tbuff), _("Select Audio File"));
    uisfcb->title = tbuff;
  }

  defdir = uisfcb->defdir;
  if (defdir == NULL || ! *defdir) {
    defdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  }
  selectdata = uiSelectInit (uisfcb->window,
      uisfcb->title,
      defdir,
      uiEntryGetValue (uisfcb->entry),
      disp, mimetype);
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (uisfcb->entry, fn);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  return UICB_CONT;
}

static void
selectfileFillRow (void *udata, uivirtlist_t *uivl, int32_t rownum)
{
  uiselectfile_t  *selectfile = udata;
  const char      *str;

  selectfile->inchange = true;
  str = slistGetDataByIdx (selectfile->filelist, rownum);
  uivlSetRowColumnStr (selectfile->uivl, rownum, SELFILE_COL_DISP, str);
  selectfile->inchange = false;
}

static void
selectfileSelected (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uiselectfile_t  *selectfile = udata;

  if (selectfile->inchange) {
    return;
  }

  selectFileResponseHandler (selectfile, RESPONSE_APPLY);
}
