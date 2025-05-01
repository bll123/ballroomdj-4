/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjstring.h"
#include "callback.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "pathutil.h"
#include "playlist.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "uidd.h"
#include "uiimppl.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIIMPPL_CB_DIALOG,
  UIIMPPL_CB_PL_SEL,
  UIIMPPL_CB_TARGET,
  UIIMPPL_CB_TYPE_SEL,
  UIIMPPL_CB_DRAG_DROP,
  UIIMPPL_CB_MAX,
};

enum {
  UIIMPPL_W_STATUS_MSG,
  UIIMPPL_W_ERROR_MSG,
  UIIMPPL_W_DIALOG,
  UIIMPPL_W_URI_LABEL,
  UIIMPPL_W_URI,
  UIIMPPL_W_URI_BUTTON,
  UIIMPPL_W_NEWNAME,
  UIIMPPL_W_IMP_TYPE,
  UIIMPPL_W_PL_SEL_LABEL,
  UIIMPPL_W_MAX,
};

enum {
  UIIMPPL_ERR_NONE    = 0,
  UIIMPPL_ERR_URI     = (1 << 0),
  UIIMPPL_ERR_NEWNAME = (1 << 1),
};

typedef struct uiimppl {
  uiwcont_t         *parentwin;
  uiwcont_t         *wcont [UIIMPPL_W_MAX];
  asconf_t          *asconf;
  callback_t        *responsecb;
  uidd_t            *plselect;
  nlist_t           *options;
  nlist_t           *aslist;
  nlist_t           *askeys;
  nlist_t           *astypes;
  nlist_t           *astypelookup;
  ilist_t           *plnames;
  const char        *urilabel;
  const char        *newnamelabel;
  callback_t        *callbacks [UIIMPPL_CB_MAX];
  char              origplname [MAXPATHLEN];
  char              olduri [MAXPATHLEN];
  size_t            asmaxwidth;
  int               askey;
  int               imptype;
  int               asconfcount;
  unsigned int      haveerrors;
  bool              newnameuserchg;
  bool              isactive;
  bool              in_cb;
  bool              changed;
} uiimppl_t;

/* import playlist */
static void uiimpplCreateDialog (uiimppl_t *uiimppl);
static bool uiimpplTargetDialog (void *udata);
static void uiimpplInitDisplay (uiimppl_t *uiimppl);
static bool uiimpplResponseHandler (void *udata, int32_t responseid);
static int  uiimpplValidateURIEntry (uiwcont_t *entry, const char *label, void *udata);
static int  uiimpplValidateNNEntry (uiwcont_t *entry, const char *label, void *udata);
static int  uiimpplValidateURI (uiimppl_t *uiimppl);
static bool uiimpplSelectHandler (void *udata, int32_t idx);
static int  uiimpplValidateNewName (uiimppl_t *uiimppl);
static bool uiimpplImportTypeChg (void *udata);
static void uiimpplFreeDialog (uiimppl_t *uiimppl);
static void uiimpplProcessValidations (uiimppl_t *uiimppl, bool forceflag);
static int32_t uiimpplDragDropCallback (void *udata, const char *uri);
static int uiimpplProcessURI (uiimppl_t *uiimppl, const char *uri);
static bool uiimpplIsPlaylistFile (pathinfo_t *pi);

uiimppl_t *
uiimpplInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiimppl_t   *uiimppl;
  const char  *asnm;
  size_t      len;
  ilistidx_t  asiteridx;
  ilistidx_t  askey;
  int         count;
  char        tbuff [40];
  slist_t     *tlist;
  slistidx_t  titeridx;

  uiimppl = mdmalloc (sizeof (uiimppl_t));
  for (int j = 0; j < UIIMPPL_W_MAX; ++j) {
    uiimppl->wcont [j] = NULL;
  }
  uiimppl->responsecb = NULL;
  uiimppl->parentwin = windowp;
  uiimppl->options = opts;
  for (int i = 0; i < UIIMPPL_CB_MAX; ++i) {
    uiimppl->callbacks [i] = NULL;
  }
  uiimppl->haveerrors = UIIMPPL_ERR_NONE;
  uiimppl->newnameuserchg = false;
  uiimppl->isactive = false;
  uiimppl->in_cb = false;
  uiimppl->changed = false;
  uiimppl->aslist = NULL;
  uiimppl->askeys = NULL;
  uiimppl->astypes = NULL;
  uiimppl->plnames = NULL;
  uiimppl->urilabel = NULL;
  uiimppl->plselect = NULL;
  uiimppl->asmaxwidth = 0;
  uiimppl->askey = -1;
  uiimppl->imptype = AUDIOSRC_TYPE_NONE;
  *uiimppl->origplname = '\0';
  uiimppl->asconf = NULL;
  uiimppl->asconfcount = 0;

  uiimppl->asconf = asconfAlloc ();
  uiimppl->asconfcount = asconfGetCount (uiimppl->asconf);

  /* CONTEXT: import playlist: playlist file types (m3u/xspf/jspf) */
  snprintf (tbuff, sizeof (tbuff), _("File (%s)"), "M3U/XSPF/JSPF");

  /* sort by audio-src name first */
  tlist = slistAlloc ("assortlist", LIST_UNORDERED, NULL);
  asconfStartIterator (uiimppl->asconf, &asiteridx);
  while ((askey = asconfIterate (uiimppl->asconf, &asiteridx)) >= 0) {
    int     mode;

    mode = asconfGetNum (uiimppl->asconf, askey, ASCONF_MODE);
    if (mode != ASCONF_MODE_CLIENT) {
      continue;
    }
    asnm = asconfGetStr (uiimppl->asconf, askey, ASCONF_NAME);
    slistSetNum (tlist, asnm, askey);
  }
  slistSetNum (tlist, tbuff, uiimppl->asconfcount);
  slistSort (tlist);

  /* create the lists for the text-spinbox */
  count = slistGetCount (tlist) + 1;
  uiimppl->aslist = nlistAlloc ("aslist", LIST_UNORDERED, NULL);
  nlistSetSize (uiimppl->aslist, count);
  uiimppl->askeys = nlistAlloc ("askeys", LIST_UNORDERED, NULL);
  nlistSetSize (uiimppl->askeys, count);
  uiimppl->astypes = nlistAlloc ("astypes", LIST_UNORDERED, NULL);
  nlistSetSize (uiimppl->astypes, count);
  uiimppl->astypelookup = nlistAlloc ("astypelookup", LIST_UNORDERED, NULL);
  nlistSetSize (uiimppl->astypelookup, count);

  count = 0;
  slistStartIterator (tlist, &titeridx);
  while ((asnm = slistIterateKey (tlist, &titeridx)) != NULL) {
    int     type;

    askey = slistGetNum (tlist, asnm);
    len = strlen (asnm);
    if (len > uiimppl->asmaxwidth) {
      uiimppl->asmaxwidth = len;
    }
    nlistSetStr (uiimppl->aslist, count, asnm);
    nlistSetNum (uiimppl->askeys, count, askey);
    type = asconfGetNum (uiimppl->asconf, askey, ASCONF_TYPE);
    if (type < 0) {
      type = AUDIOSRC_TYPE_FILE;
    }
    /* have to save the types, as audio-src-file is internal */
    nlistSetNum (uiimppl->astypes, askey, type);
    nlistSetNum (uiimppl->astypelookup, type, askey);
    ++count;
  }
  nlistSort (uiimppl->aslist);
  nlistSort (uiimppl->askeys);
  nlistSort (uiimppl->astypes);
  nlistSort (uiimppl->astypelookup);
  slistFree (tlist);

  uiimppl->askey = nlistGetNum (uiimppl->options, MANAGE_IMP_PL_ASKEY);
  if (uiimppl->askey < 0) {
    /* the asconfcount entry is set to the 'file' type */
    uiimppl->askey = uiimppl->asconfcount;
  }
  uiimppl->imptype = nlistGetNum (uiimppl->astypes, uiimppl->askey);

  uiimppl->callbacks [UIIMPPL_CB_DIALOG] = callbackInitI (
      uiimpplResponseHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TARGET] = callbackInit (
      uiimpplTargetDialog, uiimppl, NULL);
  uiimppl->callbacks [UIIMPPL_CB_PL_SEL] = callbackInitI (
      uiimpplSelectHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL] = callbackInit (
      uiimpplImportTypeChg, uiimppl, NULL);
  uiimppl->callbacks [UIIMPPL_CB_DRAG_DROP] = callbackInitS (
      uiimpplDragDropCallback, uiimppl);

  return uiimppl;
}

void
uiimpplFree (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }

  uiimpplFreeDialog (uiimppl);
  ilistFree (uiimppl->plnames);
  uiimppl->plnames = NULL;
  nlistFree (uiimppl->aslist);
  uiimppl->aslist = NULL;
  nlistFree (uiimppl->askeys);
  uiimppl->askeys = NULL;
  nlistFree (uiimppl->astypes);
  uiimppl->astypes = NULL;
  nlistFree (uiimppl->astypelookup);
  uiimppl->astypelookup = NULL;
  asconfFree (uiimppl->asconf);
  for (int i = 0; i < UIIMPPL_CB_MAX; ++i) {
    callbackFree (uiimppl->callbacks [i]);
    uiimppl->callbacks [i] = NULL;
  }
  mdfree (uiimppl);
}

void
uiimpplSetResponseCallback (uiimppl_t *uiimppl, callback_t *uicb)
{
  if (uiimppl == NULL) {
    return;
  }
  uiimppl->responsecb = uicb;
}

bool
uiimpplDialog (uiimppl_t *uiimppl)
{
  int         x, y;

  if (uiimppl == NULL) {
    return UICB_STOP;
  }

  logProcBegin ();
  uiimppl->isactive = true;
  uiimpplCreateDialog (uiimppl);
  uiDialogShow (uiimppl->wcont [UIIMPPL_W_DIALOG]);
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");

  uiimppl->askey = nlistGetNum (uiimppl->options, MANAGE_IMP_PL_ASKEY);
  if (uiimppl->askey < 0) {
    /* the asconfcount entry is set to the 'file' type */
    uiimppl->askey = uiimppl->asconfcount;
  }
  uiimppl->imptype = nlistGetNum (uiimppl->astypes, uiimppl->askey);
  uiSpinboxTextSetValue (uiimppl->wcont [UIIMPPL_W_IMP_TYPE], uiimppl->askey);

  x = nlistGetNum (uiimppl->options, IMP_PL_POSITION_X);
  y = nlistGetNum (uiimppl->options, IMP_PL_POSITION_Y);
  uiWindowMove (uiimppl->wcont [UIIMPPL_W_DIALOG], x, y, -1);

  uiimpplInitDisplay (uiimppl);

  logProcEnd ("");
  return UICB_CONT;
}

/* delayed entry validation for the audio file needs to be run */
void
uiimpplProcess (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }
  if (! uiimppl->isactive) {
    return;
  }
  if (uiimppl->wcont [UIIMPPL_W_DIALOG] == NULL) {
    return;
  }
  if (uiimppl->in_cb) {
    return;
  }

  uiimpplProcessValidations (uiimppl, uiimppl->changed);
  uiimppl->changed = false;
}

int
uiimpplGetType (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return AUDIOSRC_TYPE_NONE;
  }
  return uiimppl->imptype;
}

int
uiimpplGetASKey (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return -1;
  }
  return uiimppl->askey;
}

const char *
uiimpplGetURI (uiimppl_t *uiimppl)
{
  const char    *uri = NULL;

  if (uiimppl == NULL) {
    return NULL;
  }

  uri = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_URI]);
  return uri;
}

const char *
uiimpplGetNewName (uiimppl_t *uiimppl)
{
  const char    *newname = NULL;

  if (uiimppl == NULL) {
    return NULL;
  }

  newname = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME]);
  return newname;
}

const char *
uiimpplGetOrigName (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return NULL;
  }

  return uiimppl->origplname;
}

/* internal routines */

static void
uiimpplCreateDialog (uiimppl_t *uiimppl)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uiwcont_t     *szgrp;  // labels

  logProcBegin ();

  if (uiimppl == NULL) {
    return;
  }

  if (uiimppl->wcont [UIIMPPL_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  uiimppl->wcont [UIIMPPL_W_DIALOG] =
      uiCreateDialog (uiimppl->parentwin,
      uiimppl->callbacks [UIIMPPL_CB_DIALOG],
      /* CONTEXT: import playlist: title of dialog */
      _("Import Playlist"),
      /* CONTEXT: import playlist dialog: check the connection */
      _("Check Connection"),
      RESPONSE_CHECK,
      /* CONTEXT: import playlist dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: import playlist dialog: start import process button */
      _("Import"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiDialogPackInDialog (
      uiimppl->wcont [UIIMPPL_W_DIALOG], vbox);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiWidgetSetAllMargins (vbox, 4);

  uiDragDropSetDestURICallback (uiimppl->wcont [UIIMPPL_W_DIALOG],
      uiimppl->callbacks [UIIMPPL_CB_DRAG_DROP]);

  /* status/error msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiimppl->wcont [UIIMPPL_W_ERROR_MSG] = uiwidgetp;

  /* status msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiimppl->wcont [UIIMPPL_W_STATUS_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* type */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: import-from selection */
      _("Import From"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiSpinboxTextCreate (uiimppl);
  uiSpinboxTextSet (uiwidgetp, 0, nlistGetCount (uiimppl->aslist),
      uiimppl->asmaxwidth, uiimppl->aslist, uiimppl->askeys, NULL);
  uiSpinboxTextSetValue (uiwidgetp, uiimppl->askey);
  uiSpinboxTextSetValueChangedCallback (uiwidgetp,
      uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL]);
  uiimppl->wcont [UIIMPPL_W_IMP_TYPE] = uiwidgetp;
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* playlist selector */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: import playlist: select the song list */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_PL_SEL_LABEL] = uiwidgetp;

  uiimppl->plselect = uiddCreate ("imppl-plsel",
      uiimppl->wcont [UIIMPPL_W_DIALOG], hbox,
      /* CONTEXT: import playlist: select the playlist */
      DD_PACK_START, NULL, DD_LIST_TYPE_NUM, "", DD_REPLACE_TITLE,
      uiimppl->callbacks [UIIMPPL_CB_PL_SEL]);

  uiwcontFree (hbox);

  /* target folder / URI */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: import location */
      _("File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_URI_LABEL] = uiwidgetp;

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiEntrySetValue (uiwidgetp, "");
  uiimppl->wcont [UIIMPPL_W_URI] = uiwidgetp;

  uiwidgetp = uiCreateButton ("imppl-folder",
      uiimppl->callbacks [UIIMPPL_CB_TARGET],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiimppl->wcont [UIIMPPL_W_URI_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  /* new name */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: import playlist: new song list name */
  uiimppl->newnamelabel = _("New Song List Name");
  uiwidgetp = uiCreateColonLabel (uiimppl->newnamelabel);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_NEWNAME] = uiwidgetp;

  uiwcontFree (hbox);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  uiEntrySetValidate (uiimppl->wcont [UIIMPPL_W_URI], "",
      uiimpplValidateURIEntry, uiimppl, UIENTRY_DELAYED);
  uiEntrySetValidate (uiimppl->wcont [UIIMPPL_W_NEWNAME], "",
      uiimpplValidateNNEntry, uiimppl, UIENTRY_IMMEDIATE);
  uiSpinboxTextSetValueChangedCallback (uiimppl->wcont [UIIMPPL_W_IMP_TYPE],
      uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL]);

  logProcEnd ("");
}

static bool
uiimpplTargetDialog (void *udata)
{
  uiimppl_t  *uiimppl = udata;
  uiselect_t  *selectdata;
  const char  *ofn = NULL;
  char        *fn = NULL;

  if (uiimppl == NULL) {
    return UICB_STOP;
  }

  ofn = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_URI]);
  selectdata = uiSelectInit (uiimppl->parentwin,
      /* CONTEXT: import playlist: title of dialog */
      _("Import Playlist"), sysvarsGetStr (SV_BDJ4_DIR_DATATOP), NULL,
      /* CONTEXT: import playlist: name of fn import type */
      _("Playlists"), "audio/x-mpegurl|application/xspf+xml|*.jspf");

  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_URI], fn);
    logMsg (LOG_DBG, LOG_IMPORTANT, "selected file: %s", fn);
    mdfree (fn);   // allocated by gtk
  }
  uiSelectFree (selectdata);

  return UICB_CONT;
}


static void
uiimpplInitDisplay (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }

  uiimpplImportTypeChg (uiimppl);
  uiimpplProcessValidations (uiimppl, true);
}

static bool
uiimpplResponseHandler (void *udata, int32_t responseid)
{
  uiimppl_t  *uiimppl = udata;
  int             x, y, ws;

  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  uiWindowGetPosition (
      uiimppl->wcont [UIIMPPL_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiimppl->options, IMP_PL_POSITION_X, x);
  nlistSetNum (uiimppl->options, IMP_PL_POSITION_Y, y);
  nlistSetNum (uiimppl->options, MANAGE_IMP_PL_ASKEY, uiimppl->askey);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: del window");
      uiimppl->imptype = AUDIOSRC_TYPE_NONE;
      uiimpplFreeDialog (uiimppl);
      uiimppl->isactive = false;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: close window");
      uiWidgetHide (uiimppl->wcont [UIIMPPL_W_DIALOG]);
      uiimppl->imptype = AUDIOSRC_TYPE_NONE;
      uiimppl->isactive = false;
      break;
    }
    case RESPONSE_CHECK: {
      bool    rc;

      if (uiimppl->imptype == AUDIOSRC_TYPE_NONE ||
          uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
        break;
      }

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: check connection");
      rc = audiosrcCheckConnection (uiimppl->askey,
          uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_URI]));
      if (rc == false) {
        uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
            /* CONTEXT: configuration: audio source: check connection status */
            _("Connection Failed"));
      } else {
        uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
            /* CONTEXT: configuration: audio source: check connection status */
            _("Connection OK"));
      }
      /* dialog is still active */
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: apply");
      uiLabelSetText (
          uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
          /* CONTEXT: please wait... status message */
          _("Please wait\xe2\x80\xa6"));
      /* do not close or hide the dialog; it will stay active and */
      /* the status message will be updated */
      if (uiimppl->responsecb != NULL) {
        callbackHandler (uiimppl->responsecb);
      }
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");
      uiWidgetHide (uiimppl->wcont [UIIMPPL_W_DIALOG]);
      uiimppl->imptype = AUDIOSRC_TYPE_NONE;
      uiimppl->isactive = false;
      break;
    }
  }

  return UICB_CONT;
}

static int
uiimpplValidateURIEntry (uiwcont_t *entry, const char *label, void *udata)
{
  int       rc = UICB_CONT;
  uiimppl_t *uiimppl = udata;

  rc = uiimpplValidateURI (uiimppl);
  return rc;
}

static int
uiimpplValidateNNEntry (uiwcont_t *entry, const char *label, void *udata)
{
  int       rc = UICB_CONT;
  uiimppl_t *uiimppl = udata;

  rc = uiimpplValidateNewName (uiimppl);
  return rc;
}

static int
uiimpplValidateURI (uiimppl_t *uiimppl)
{
  uiwcont_t   *entry;
  const char  *str;
  char        tbuff [MAXPATHLEN];
  pathinfo_t  *pi;
  bool        haderrors = false;

  if (uiimppl == NULL) {
    return UIENTRY_OK;
  }
  if (uiimppl->isactive == false) {
    return UIENTRY_OK;
  }
  uiimppl->in_cb = true;

  entry = uiimppl->wcont [UIIMPPL_W_URI];

//  /* any change clears the status message */
//  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  if (uiimppl->haveerrors != UIIMPPL_ERR_NONE) {
    haderrors = true;
  }

  uiimppl->haveerrors &= ~ UIIMPPL_ERR_URI;
  str = uiEntryGetValue (entry);

  *tbuff = '\0';
  stpecpy (tbuff, tbuff + sizeof (tbuff), str);

  if (strcmp (uiimppl->olduri, str) != 0) {
    /* re-sets the type to match the URI, sets the URI */
    if (uiimpplProcessURI (uiimppl, tbuff) == UICB_STOP) {
      uiimppl->haveerrors |= UIIMPPL_ERR_URI;
      uiimppl->in_cb = false;
      return UIENTRY_ERROR;
    }
  }

  if (uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
    pathNormalizePath (tbuff, sizeof (tbuff));
  }

  pi = pathInfo (tbuff);

  if (uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
    bool    extok = false;

    if (uiimpplIsPlaylistFile (pi)) {
      extok = true;
    }

    if (*tbuff && (! extok || ! fileopFileExists (tbuff))) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: import playlist: invalid target file */
          _("Invalid File"));
      pathInfoFree (pi);
      uiimppl->haveerrors |= UIIMPPL_ERR_URI;
      uiimppl->in_cb = false;
      return UIENTRY_ERROR;
    }
  }

  /* both bdj4:// and podcasts will have a full-url */
  if (uiimppl->imptype != AUDIOSRC_TYPE_FILE) {
    int     vrc;
    char    tmsg [300];

    vrc = validate (tmsg, sizeof (tmsg), uiimppl->urilabel, str,
        VAL_NOT_EMPTY | VAL_FULL_URI);
    if (vrc == false) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], tmsg);
      uiimppl->haveerrors |= UIIMPPL_ERR_URI;
      uiimppl->in_cb = false;
      return UIENTRY_ERROR;
    }
  }

  if (uiimppl->imptype == AUDIOSRC_TYPE_BDJ4) {
    if (strncmp (str, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) != 0) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: import playlist: invalid URL */
          _("Invalid URL"));
      uiimppl->haveerrors |= UIIMPPL_ERR_URI;
      uiimppl->in_cb = false;
      return UIENTRY_ERROR;
    }
  }

  /* podcast rss feeds will have a .xml extension */
  if (uiimppl->imptype == AUDIOSRC_TYPE_PODCAST) {
    if (strncmp (str, AS_HTTPS_PFX, AS_HTTPS_PFX_LEN) != 0 ||
        strncmp (str + strlen (str) - AS_XML_SFX_LEN,
            AS_XML_SFX, AS_XML_SFX_LEN) != 0) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: import playlist: invalid URL */
          _("Invalid URL"));
      uiimppl->haveerrors |= UIIMPPL_ERR_URI;
      uiimppl->in_cb = false;
      return UIENTRY_ERROR;
    }
  }

  /* do not update the new-name if the user has modified it */
  if (uiimppl->newnameuserchg == false) {
    snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
    uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME], tbuff);
  }
  pathInfoFree (pi);

  stpecpy (uiimppl->olduri, uiimppl->olduri + sizeof (uiimppl->olduri), str);

  if (haderrors && uiimppl->haveerrors == UIIMPPL_ERR_NONE) {
    uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");
  }

  uiimppl->in_cb = false;
  return UIENTRY_OK;
}

static bool
uiimpplSelectHandler (void *udata, int idx)
{
  uiimppl_t   *uiimppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];

  if (uiimppl == NULL) {
    return UICB_STOP;
  }
  if (uiimppl->isactive == false) {
    return UICB_STOP;
  }
  uiimppl->in_cb = true;

  /* any change to the selected playlist clears the status/error message */
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");

  uiimppl->haveerrors = UIIMPPL_ERR_NONE;
  uiimppl->newnameuserchg = false;

  str = ilistGetStr (uiimppl->plnames, idx, DD_LIST_DISP);
  if (uiimppl->imptype == AUDIOSRC_TYPE_BDJ4) {
    snprintf (tbuff, sizeof (tbuff), "%s%s:%" PRIu16 "/%s",
        AS_BDJ4_PFX,
        asconfGetStr (uiimppl->asconf, uiimppl->askey, ASCONF_URI),
        (uint16_t) asconfGetNum (uiimppl->asconf, uiimppl->askey, ASCONF_PORT),
        str);
    uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_URI], tbuff);
  }
  if (uiimppl->imptype == AUDIOSRC_TYPE_PODCAST) {
    ;
  }
  stpecpy (uiimppl->origplname,
      uiimppl->origplname + sizeof (uiimppl->origplname), str);

  /* allow entry validators to run */
  uiimppl->in_cb = false;

  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME], str);
  return UICB_CONT;
}

static int
uiimpplValidateNewName (uiimppl_t *uiimppl)
{
  uiwcont_t   *entry;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  bool        haderrors = false;

  if (uiimppl == NULL) {
    return UIENTRY_OK;
  }
  if (uiimppl->isactive == false) {
    return UIENTRY_OK;
  }
  uiimppl->in_cb = true;

  entry = uiimppl->wcont [UIIMPPL_W_NEWNAME];

  /* any change clears the status message */
//fprintf (stderr, "val-nn: clr\n");
//  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  if (uiimppl->haveerrors != UIIMPPL_ERR_NONE) {
    haderrors = true;
  }
  uiimppl->haveerrors &= ~ UIIMPPL_ERR_NEWNAME;

  rc = uiutilsValidatePlaylistName (entry, uiimppl->newnamelabel,
      uiimppl->wcont [UIIMPPL_W_ERROR_MSG]);

  if (rc == UIENTRY_ERROR) {
    uiimppl->haveerrors |= UIIMPPL_ERR_NEWNAME;
  }

  if (rc == UIENTRY_OK) {
    uiimppl->newnameuserchg = true;
    str = uiEntryGetValue (entry);
    if (*str) {
      pathbldMakePath (fn, sizeof (fn),
          str, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
      if (fileopFileExists (fn)) {
        /* CONTEXT: import playlist: message if the target song list already exists */
        snprintf (tbuff, sizeof (tbuff), "%s: %s", str, _("Already Exists"));
        uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], tbuff);
      }
    }
  }

  if (haderrors && uiimppl->haveerrors == UIIMPPL_ERR_NONE) {
    uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");
  }

  uiimppl->in_cb = false;
  return rc;
}

static bool
uiimpplImportTypeChg (void *udata)
{
  uiimppl_t   *uiimppl = udata;
  char        tbuff [40];
  int         askey;

  if (uiimppl == NULL) {
    return UICB_STOP;
  }
  if (uiimppl->isactive == false) {
    return UICB_STOP;
  }
  if (uiimppl->in_cb) {
    return UICB_CONT;
  }
  uiimppl->in_cb = true;

  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");
  uiimppl->haveerrors = UIIMPPL_ERR_NONE;

  askey = uiSpinboxTextGetValue (uiimppl->wcont [UIIMPPL_W_IMP_TYPE]);
  uiimppl->askey = askey;
  uiimppl->imptype = nlistGetNum (uiimppl->astypes, uiimppl->askey);

  if (uiimppl->imptype != AUDIOSRC_TYPE_FILE) {
    asiter_t    *asiter;
    const char  *plnm;
    int         idx;

    idx = 0;
    ilistFree (uiimppl->plnames);
    asiter = audiosrcStartIterator (uiimppl->imptype, AS_ITER_PL_NAMES,
        uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_URI]), askey);
    uiimppl->plnames = ilistAlloc ("plnames", LIST_ORDERED);
    ilistSetSize (uiimppl->plnames, audiosrcIterCount (asiter));
    while ((plnm = audiosrcIterate (asiter)) != NULL) {
      ilistSetNum (uiimppl->plnames, idx, DD_LIST_KEY_NUM, idx);
      ilistSetStr (uiimppl->plnames, idx, DD_LIST_DISP, plnm);
      ++idx;
    }
    audiosrcCleanIterator (asiter);
    uiddSetList (uiimppl->plselect, uiimppl->plnames);
    uiddSetSelection (uiimppl->plselect, 0);
  }

  if (uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
    uiWidgetSetState (uiimppl->wcont [UIIMPPL_W_PL_SEL_LABEL], UIWIDGET_DISABLE);
    uiddSetState (uiimppl->plselect, UIWIDGET_DISABLE);
    uiWidgetShow (uiimppl->wcont [UIIMPPL_W_URI_BUTTON]);
    /* CONTEXT: import playlist: import location */
    uiimppl->urilabel = _("File");
  } else {
    uiWidgetSetState (uiimppl->wcont [UIIMPPL_W_PL_SEL_LABEL], UIWIDGET_ENABLE);
    uiddSetState (uiimppl->plselect, UIWIDGET_ENABLE);
    uiWidgetHide (uiimppl->wcont [UIIMPPL_W_URI_BUTTON]);
    /* CONTEXT: import playlist: import location */
    uiimppl->urilabel = _("URL");
  }
  snprintf (tbuff, sizeof (tbuff), "%s:", uiimppl->urilabel);
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_URI_LABEL], tbuff);

  uiimppl->in_cb = false;
  /* process all validations again */
  uiimppl->changed = true;

  return UICB_CONT;
}

static void
uiimpplFreeDialog (uiimppl_t *uiimppl)
{
  uiddFree (uiimppl->plselect);
  uiimppl->plselect = NULL;
  for (int j = 0; j < UIIMPPL_W_MAX; ++j) {
    uiwcontFree (uiimppl->wcont [j]);
    uiimppl->wcont [j] = NULL;
  }
}

static void
uiimpplProcessValidations (uiimppl_t *uiimppl, bool forceflag)
{
  uiEntryValidate (uiimppl->wcont [UIIMPPL_W_URI], forceflag);
  uiEntryValidate (uiimppl->wcont [UIIMPPL_W_NEWNAME], forceflag);

//  if (uiimppl->haveerrors == UIIMPPL_ERR_NONE) {
//fprintf (stderr, "proc-val: no-errors\n");
//    uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");
//  }
}

static int32_t
uiimpplDragDropCallback (void *udata, const char *uri)
{
  uiimppl_t   *uiimppl = udata;

  if (uiimppl == NULL || uri == NULL || ! *uri) {
    return UICB_STOP;
  }

  return uiimpplProcessURI (uiimppl, uri);
}

static int
uiimpplProcessURI (uiimppl_t *uiimppl, const char *uri)
{
  int         type;
  size_t      urilen;
  pathinfo_t  *pi;

  urilen = strlen (uri);
  pi = pathInfo (uri);

  type = AUDIOSRC_TYPE_NONE;
  if (strncmp (uri, AS_FILE_PFX, AS_FILE_PFX_LEN) == 0 &&
      uiimpplIsPlaylistFile (pi)) {
    type = AUDIOSRC_TYPE_FILE;
    uri += AS_FILE_PFX_LEN;
  }
  if (strncmp (uri, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) == 0) {
    type = AUDIOSRC_TYPE_BDJ4;
  }
  if (strncmp (uri, AS_HTTPS_PFX, AS_HTTPS_PFX_LEN) == 0 &&
      pathInfoExtCheck (pi, AS_XML_SFX)) {
    type = AUDIOSRC_TYPE_PODCAST;
  }

  if (type == AUDIOSRC_TYPE_NONE) {
    pathInfoFree (pi);
    return UICB_STOP;
  }

  uiimppl->askey = nlistGetNum (uiimppl->astypelookup, type);
  uiSpinboxTextSetValue (uiimppl->wcont [UIIMPPL_W_IMP_TYPE], uiimppl->askey);
  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_URI], uri);
  uiimppl->changed = true;

  pathInfoFree (pi);
  return UICB_CONT;
}

static bool
uiimpplIsPlaylistFile (pathinfo_t *pi)
{
  bool    rc = false;

  if (pathInfoExtCheck (pi, ".m3u") ||
      pathInfoExtCheck (pi, ".m3u8") ||
      pathInfoExtCheck (pi, ".xspf") ||
      pathInfoExtCheck (pi, ".jspf")) {
    rc = true;
  }

  return rc;
}
