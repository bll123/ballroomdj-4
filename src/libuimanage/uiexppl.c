/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjstring.h"
#include "callback.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "pathutil.h"
#include "playlist.h"
#include "sysvars.h"
#include "ui.h"
#include "uiexppl.h"
#include "validate.h"

enum {
  UIEXPPL_CB_DIALOG,
  UIEXPPL_CB_EXP_TYPE,
  UIEXPPL_CB_TARGET,
  UIEXPPL_CB_MAX,
};

enum {
  UIEXPPL_W_DIALOG,
  UIEXPPL_W_STATUS_MSG,
  UIEXPPL_W_ERROR_MSG,
  UIEXPPL_W_EXP_TYPE,
  UIEXPPL_W_TARGET,
  UIEXPPL_W_TGT_BUTTON,
  UIEXPPL_W_MAX,
};

typedef struct {
  int         type;
  const char  *display;
  const char  *ext;
  const char  *extb;
} uiexppltype_t;

static uiexppltype_t exptypes [EI_TYPE_MAX] = {
  [EI_TYPE_JSPF] = { EI_TYPE_JSPF,  "JSPF", ".jspf", NULL },
  [EI_TYPE_M3U] = { EI_TYPE_M3U,   "M3U",  ".m3u", ".m3u8" },
  [EI_TYPE_XSPF] = { EI_TYPE_XSPF,  "XSPF",  ".xspf", NULL },
};

typedef struct uiexppl {
  uiwcont_t         *wcont [UIEXPPL_W_MAX];
  callback_t        *responsecb;
  uiwcont_t         *parentwin;
  nlist_t           *options;
  callback_t        *callbacks [UIEXPPL_CB_MAX];
  nlist_t           *typelist;
  char              *slname;
  int               exptype;
  bool              isactive;
  bool              in_validate;
} uiexppl_t;

/* export playlist */
static void   uiexpplCreateDialog (uiexppl_t *uiexppl);
static bool   uiexpplTargetDialog (void *udata);
static bool   uiexpplResponseHandler (void *udata, int32_t responseid);
static void   uiexpplFreeDialog (uiexppl_t *uiexppl);
static int    uiexpplValidateTarget (uiwcont_t *entry, const char *label, void *udata);
static bool   uiexpplExportTypeCallback (void *udata);

uiexppl_t *
uiexpplInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiexppl_t   *uiexppl;
  nlist_t     *tlist;

  uiexppl = mdmalloc (sizeof (uiexppl_t));
  for (int j = 0; j < UIEXPPL_W_MAX; ++j) {
    uiexppl->wcont [j] = NULL;
  }
  uiexppl->responsecb = NULL;
  uiexppl->parentwin = windowp;
  uiexppl->options = opts;
  for (int i = 0; i < UIEXPPL_CB_MAX; ++i) {
    uiexppl->callbacks [i] = NULL;
  }
  uiexppl->slname = NULL;
  uiexppl->isactive = false;
  uiexppl->in_validate = false;

  uiexppl->exptype = nlistGetNum (uiexppl->options, MANAGE_EXP_PL_TYPE);
  if (uiexppl->exptype < 0) {
    uiexppl->exptype = EI_TYPE_M3U;
  }

  uiexppl->callbacks [UIEXPPL_CB_DIALOG] = callbackInitI (
      uiexpplResponseHandler, uiexppl);
  uiexppl->callbacks [UIEXPPL_CB_TARGET] = callbackInit (
      uiexpplTargetDialog, uiexppl, NULL);
  uiexppl->callbacks [UIEXPPL_CB_EXP_TYPE] = callbackInit (
      uiexpplExportTypeCallback, uiexppl, NULL);

  tlist = nlistAlloc ("exptype", LIST_ORDERED, NULL);
  nlistSetSize (tlist, EI_TYPE_MAX);
  for (int i = 0; i < EI_TYPE_MAX; ++i) {
    nlistSetStr (tlist, exptypes [i].type, exptypes [i].display);
  }
  uiexppl->typelist = tlist;

  return uiexppl;
}

void
uiexpplFree (uiexppl_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }

  for (int i = 0; i < UIEXPPL_CB_MAX; ++i) {
    callbackFree (uiexppl->callbacks [i]);
  }
  nlistFree (uiexppl->typelist);
  dataFree (uiexppl->slname);
  /* free-dialog frees all of the widget containers */
  uiexpplFreeDialog (uiexppl);
  mdfree (uiexppl);
}

void
uiexpplSetResponseCallback (uiexppl_t *uiexppl, callback_t *uicb)
{
  if (uiexppl == NULL) {
    return;
  }
  uiexppl->responsecb = uicb;
}

bool
uiexpplDialog (uiexppl_t *uiexppl, const char *slname)
{
  int           x, y;
  const char    *odir = NULL;
  char          tbuff [MAXPATHLEN];

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  logProcBegin ();
  dataFree (uiexppl->slname);
  uiexppl->slname = NULL;
  if (slname != NULL) {
    uiexppl->slname = mdstrdup (slname);
  }

  uiexpplCreateDialog (uiexppl);

  odir = nlistGetStr (uiexppl->options, MANAGE_EXP_PL_DIR);
  if (odir == NULL || ! *odir) {
    odir = sysvarsGetStr (SV_HOME);
  }
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s",
      odir, uiexppl->slname, exptypes [uiexppl->exptype].ext);
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], tbuff);

  uiDialogShow (uiexppl->wcont [UIEXPPL_W_DIALOG]);
  uiexppl->isactive = true;

  x = nlistGetNum (uiexppl->options, EXP_PL_POSITION_X);
  y = nlistGetNum (uiexppl->options, EXP_PL_POSITION_Y);
  uiWindowMove (uiexppl->wcont [UIEXPPL_W_DIALOG], x, y, -1);
  logProcEnd ("");
  return UICB_CONT;
}


/* delayed entry validation for the audio file needs to be run */
void
uiexpplProcess (uiexppl_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }
  if (! uiexppl->isactive) {
    return;
  }

  uiEntryValidate (uiexppl->wcont [UIEXPPL_W_TARGET], false);
}

/* internal routines */

static void
uiexpplCreateDialog (uiexppl_t *uiexppl)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uiwcont_t     *szgrp;  // labels

  logProcBegin ();

  if (uiexppl == NULL) {
    return;
  }

  if (uiexppl->wcont [UIEXPPL_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  uiexppl->wcont [UIEXPPL_W_DIALOG] =
      uiCreateDialog (uiexppl->parentwin,
      uiexppl->callbacks [UIEXPPL_CB_DIALOG],
      /* CONTEXT: export playlist: title of dialog */
      _("Export Playlist"),
      /* CONTEXT: export playlist: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: export playlist dialog: export button */
      _("Export"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiDialogPackInDialog (
      uiexppl->wcont [UIEXPPL_W_DIALOG], vbox);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiWidgetSetAllMargins (vbox, 4);

  /* status msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiexppl->wcont [UIEXPPL_W_STATUS_MSG] = uiwidgetp;

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiexppl->wcont [UIEXPPL_W_ERROR_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* spinbox for export type */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export playlist: type of export*/
      _("Export Type"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiSpinboxTextCreate (uiexppl);
  uiSpinboxTextSet (uiwidgetp, 0, nlistGetCount (uiexppl->typelist), 5,
      uiexppl->typelist, NULL, NULL);
  uiSpinboxTextSetValue (uiwidgetp, uiexppl->exptype);
  uiBoxPackStart (hbox, uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_EXP_TYPE] = uiwidgetp;
  uiwcontFree (hbox);

  /* target folder */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export playlist: export folder location */
      _("Export to"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_TARGET] = uiwidgetp;

  uiEntrySetValidate (uiwidgetp, "",
      uiexpplValidateTarget, uiexppl, UIENTRY_DELAYED);

  /* target folder button */
  uiwidgetp = uiCreateButton ("exppl-folder",
      uiexppl->callbacks [UIEXPPL_CB_TARGET], "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiexppl->wcont [UIEXPPL_W_TGT_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  uiSpinboxTextSetValueChangedCallback (uiexppl->wcont [UIEXPPL_W_EXP_TYPE],
      uiexppl->callbacks [UIEXPPL_CB_EXP_TYPE]);

  logProcEnd ("");
}

static bool
uiexpplTargetDialog (void *udata)
{
  uiexppl_t  *uiexppl = udata;
  uiselect_t  *selectdata;
  const char  *str;
  char        odir [MAXPATHLEN];
  char        *fn = NULL;
  char        tname [200];
  pathinfo_t  *pi;

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  str = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  pi = pathInfo (str);
  snprintf (odir, sizeof (odir), "%.*s", (int) pi->dlen, pi->dirname);
  pathInfoFree (pi);
  snprintf (tname, sizeof (tname), "%s%s",
      uiexppl->slname, exptypes [uiexppl->exptype].ext);
  selectdata = uiSelectInit (uiexppl->parentwin,
      /* CONTEXT: managementui: export playlist: title of save dialog */
      _("Export Playlist"),
      odir, tname,
      /* CONTEXT: managementui: export playlist: name of file export type */
      _("Playlists"), "audio/x-mpegurl|application/xspf+xml|*.jspf");

  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], fn);
    logMsg (LOG_DBG, LOG_IMPORTANT, "selected file: %s", fn);
    mdfree (fn);   // allocated by gtk
  }
  uiSelectFree (selectdata);

  return UICB_CONT;
}


static bool
uiexpplResponseHandler (void *udata, int32_t responseid)
{
  uiexppl_t  *uiexppl = udata;
  int             x, y, ws;

  uiWindowGetPosition (
      uiexppl->wcont [UIEXPPL_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiexppl->options, EXP_PL_POSITION_X, x);
  nlistSetNum (uiexppl->options, EXP_PL_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: exppl: del window");
      uiexpplFreeDialog (uiexppl);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: exppl: close window");
      uiWidgetHide (uiexppl->wcont [UIEXPPL_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      const char  *str;

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: exppl: apply");
      str = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
      if (uiexppl->responsecb != NULL) {
        callbackHandlerSI (uiexppl->responsecb, str, uiexppl->exptype);
      }
      uiWidgetHide (uiexppl->wcont [UIEXPPL_W_DIALOG]);
      break;
    }
  }

  uiexppl->isactive = false;
  return UICB_CONT;
}

static void
uiexpplFreeDialog (uiexppl_t *uiexppl)
{
  for (int j = 0; j < UIEXPPL_W_MAX; ++j) {
    uiwcontFree (uiexppl->wcont [j]);
    uiexppl->wcont [j] = NULL;
  }
}

static int
uiexpplValidateTarget (uiwcont_t *entry, const char *label, void *udata)
{
  uiexppl_t   *uiexppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];
  char        tdir [MAXPATHLEN];
  pathinfo_t  *pi;
  bool        found = false;

  if (uiexppl->in_validate) {
    return UIENTRY_OK;
  }

  uiLabelSetText (uiexppl->wcont [UIEXPPL_W_ERROR_MSG], "");

  uiexppl->in_validate = true;

  str = uiEntryGetValue (entry);

  stpecpy (tbuff, tbuff + sizeof (tbuff), str);
  pathNormalizePath (tbuff, sizeof (tbuff));

  if (! *tbuff || fileopIsDirectory (tbuff)) {
    if (*tbuff) {
      uiLabelSetText (uiexppl->wcont [UIEXPPL_W_ERROR_MSG],
          /* CONTEXT: export playlist: invalid target file */
          _("Invalid File"));
    }
    uiexppl->in_validate = false;
    return UIENTRY_ERROR;
  }

  pi = pathInfo (tbuff);
  snprintf (tdir, sizeof (tdir), "%.*s", (int) pi->dlen, pi->dirname);
  if (! fileopIsDirectory (tdir)) {
    uiLabelSetText (uiexppl->wcont [UIEXPPL_W_ERROR_MSG],
        /* CONTEXT: export playlist: invalid target file */
        _("Invalid File"));
    uiexppl->in_validate = false;
    return UIENTRY_ERROR;
  }

  for (int i = 0; i < EI_TYPE_MAX; ++i) {
    if (pathInfoExtCheck (pi, exptypes [i].ext) ||
        (exptypes [i].extb != NULL &&
        pathInfoExtCheck (pi, exptypes [i].extb))) {
      uiSpinboxTextSetValue (uiexppl->wcont [UIEXPPL_W_EXP_TYPE], i);
      uiexppl->exptype = i;
      found = true;
      break;
    }
  }
  pathInfoFree (pi);

  if (! found) {
    uiLabelSetText (uiexppl->wcont [UIEXPPL_W_ERROR_MSG],
        /* CONTEXT: export playlist: invalid target file extension */
        _("Invalid Extension"));
    uiexppl->in_validate = false;
    return UIENTRY_ERROR;
  }

  nlistSetStr (uiexppl->options, MANAGE_EXP_PL_DIR, tdir);
  nlistSetNum (uiexppl->options, MANAGE_EXP_PL_TYPE, uiexppl->exptype);

  uiexppl->in_validate = false;
  return UIENTRY_OK;
}


static bool
uiexpplExportTypeCallback (void *udata)
{
  uiexppl_t   *uiexppl = udata;
  const char  *str;
  pathinfo_t  *pi;

  if (uiexppl->in_validate) {
    return UICB_CONT;
  }

  uiexppl->in_validate = true;

  uiexppl->exptype = uiSpinboxTextGetValue (
      uiexppl->wcont [UIEXPPL_W_EXP_TYPE]);

  str = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  pi = pathInfo (str);
  if (pi->dlen > 0 &&
      ! pathInfoExtCheck (pi, exptypes [uiexppl->exptype].ext)) {
    char        tbuff [MAXPATHLEN];

    snprintf (tbuff, sizeof (tbuff), "%.*s/%.*s%s",
        (int) pi->dlen, pi->dirname,
        (int) pi->blen, pi->basename,
        exptypes [uiexppl->exptype].ext);
    uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], tbuff);
  }
  pathInfoFree (pi);

  uiexppl->in_validate = false;
  return UICB_CONT;
}

