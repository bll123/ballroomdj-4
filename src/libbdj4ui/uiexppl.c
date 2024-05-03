/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
#include "uiutils.h"
#include "validate.h"

enum {
  UIEXPPL_CB_DIALOG,
  UIEXPPL_CB_EXP_TYPE,
  UIEXPPL_CB_TARGET,
  UIEXPPL_CB_MAX,
};

enum {
  UIEXPPL_W_DIALOG,
  UIEXPPL_W_EXP_TYPE,
  UIEXPPL_W_TARGET,
  UIEXPPL_W_TGT_BUTTON,
  UIEXPPL_W_MAX,
};

typedef struct {
  int         type;
  const char  *display;
  const char  *ext;
} uiexppltype_t;

uiexppltype_t exptypes [EI_TYPE_MAX] = {
  [EI_TYPE_JSPF] = { EI_TYPE_JSPF,  "JSPF", ".jspf" },
  [EI_TYPE_M3U] = { EI_TYPE_M3U,   "M3U",  ".m3u" },
  [EI_TYPE_XSPF] = { EI_TYPE_XSPF,  "XSPF",  ".xspf" },
};

typedef struct uiexppl {
  uiwcont_t         *wcont [UIEXPPL_W_MAX];
  callback_t        *responsecb;
  uiwcont_t         *parentwin;
  nlist_t           *options;
  callback_t        *callbacks [UIEXPPL_CB_MAX];
  nlist_t           *typelist;
  char              *slname;
  int               currexptype;
  bool              isactive : 1;
  bool              in_validation : 1;
} uiexppl_t;

/* export playlist */
static void   uiexpplCreateDialog (uiexppl_t *uiexppl);
static bool   uiexpplTargetDialog (void *udata);
static bool   uiexpplResponseHandler (void *udata, long responseid);
static void   uiexpplFreeDialog (uiexppl_t *uiexppl);
static int    uiexpplValidateTarget (uiwcont_t *entry, void *udata);
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
  uiexppl->currexptype = EI_TYPE_M3U;
  uiexppl->isactive = false;
  uiexppl->in_validation = false;

  uiexppl->callbacks [UIEXPPL_CB_DIALOG] = callbackInitLong (
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
  int         x, y;

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiexpplDialog");
  dataFree (uiexppl->slname);
  uiexppl->slname = NULL;
  if (slname != NULL) {
    uiexppl->slname = mdstrdup (slname);
  }

  uiexpplCreateDialog (uiexppl);
  uiDialogShow (uiexppl->wcont [UIEXPPL_W_DIALOG]);
  uiexppl->isactive = true;

  x = nlistGetNum (uiexppl->options, EXP_PL_POSITION_X);
  y = nlistGetNum (uiexppl->options, EXP_PL_POSITION_Y);
  uiWindowMove (uiexppl->wcont [UIEXPPL_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uiexpplDialog", "");
  return UICB_CONT;
}

void
uiexpplDialogClear (uiexppl_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }

  uiWidgetHide (uiexppl->wcont [UIEXPPL_W_DIALOG]);
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
  const char    *odir = NULL;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uiexpplCreateDialog");

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
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (
      uiexppl->wcont [UIEXPPL_W_DIALOG], vbox);

  /* spinbox for export type */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export playlist: type of export*/
      _("Export Type"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiSpinboxTextCreate (uiexppl);
  uiSpinboxTextSet (uiwidgetp, 0, nlistGetCount (uiexppl->typelist), 5,
      uiexppl->typelist, NULL, NULL);
  uiSpinboxTextSetValue (uiwidgetp, uiexppl->currexptype);
  uiBoxPackStart (hbox, uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_EXP_TYPE] = uiwidgetp;
  uiwcontFree (hbox);

  /* target folder */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export playlist: export folder location */
      _("Export to"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_TARGET] = uiwidgetp;

  odir = nlistGetStr (uiexppl->options, MANAGE_EXP_BDJ4_DIR);
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  strlcpy (tbuff, odir, sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], tbuff);
  uiEntrySetValidate (uiwidgetp,
      uiexpplValidateTarget, uiexppl, UIENTRY_DELAYED);

  /* target folder button */
  uiwidgetp = uiCreateButton (
      uiexppl->callbacks [UIEXPPL_CB_TARGET], "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_TGT_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  uiSpinboxTextSetValueChangedCallback (uiexppl->wcont [UIEXPPL_W_EXP_TYPE],
      uiexppl->callbacks [UIEXPPL_CB_EXP_TYPE]);

  logProcEnd (LOG_PROC, "uiexpplCreateDialog", "");
}

static bool
uiexpplTargetDialog (void *udata)
{
  uiexppl_t  *uiexppl = udata;
  uiselect_t  *selectdata;
  const char  *odir = NULL;
  char        *fn = NULL;
  char        tname [200];

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  odir = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  snprintf (tname, sizeof (tname), "%s/%s%s",
      odir, uiexppl->slname, exptypes [uiexppl->currexptype].ext);
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
uiexpplResponseHandler (void *udata, long responseid)
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
        callbackHandlerStr (uiexppl->responsecb, str);
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
uiexpplValidateTarget (uiwcont_t *entry, void *udata)
{
  uiexppl_t   *uiexppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];
  char        tdir [MAXPATHLEN];
  pathinfo_t  *pi;

  if (uiexppl->in_validation) {
    return UICB_CONT;
  }

  uiexppl->in_validation = true;

  str = uiEntryGetValue (entry);
  *tbuff = '\0';
  pathNormalizePath (tbuff, sizeof (tbuff));

  pi = pathInfo (tbuff);
  snprintf (tdir, sizeof (tdir), "%.*s", (int) pi->dlen, pi->dirname);
  nlistSetStr (uiexppl->options, MANAGE_EXP_PL_DIR, tdir);
  pathInfoFree (pi);

  /* validation failures: */
  /*   target is not set (no message displayed) */
  /*   target is a directory */
  /* file may or may not exist */
  if (! *str || fileopIsDirectory (str)) {
    uiexppl->in_validation = false;
    return UIENTRY_ERROR;
  }

// ### if there is a mismatch between the filename extension and
//     the currexptype, set the currexptype and adjust the spinbox
//     but only do this if the filename extension is set to one
//     of the valid values.

  uiexppl->in_validation = false;
  return UIENTRY_OK;
}


static bool
uiexpplExportTypeCallback (void *udata)
{
  uiexppl_t   *uiexppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];
  pathinfo_t  *pi;

  if (uiexppl->in_validation) {
    return UICB_CONT;
  }

  uiexppl->in_validation = true;

  uiexppl->currexptype = uiSpinboxTextGetValue (
      uiexppl->wcont [UIEXPPL_W_EXP_TYPE]);

  str = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  strlcpy (tbuff, str, sizeof (tbuff));
  pi = pathInfo (str);
  if (pi->dlen > 0 &&
      ! pathInfoExtCheck (pi, exptypes [uiexppl->currexptype].ext)) {
fprintf (stderr, "mismatch curr:%s path:%.*s\n", exptypes [uiexppl->currexptype].ext, (int) pi->elen, pi->extension);
  }
  pathInfoFree (pi);

  uiexppl->in_validation = false;
  return UICB_CONT;
}

