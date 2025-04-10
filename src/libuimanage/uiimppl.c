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
#include "sysvars.h"
#include "ui.h"
#include "uidd.h"
#include "uiimppl.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIIMPPL_CB_DIALOG,
  UIIMPPL_CB_SEL,
  UIIMPPL_CB_SOURCE_TYPE,
  UIIMPPL_CB_TARGET,
  UIIMPPL_CB_TYPE_SEL,
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

typedef struct uiimppl {
  uiwcont_t         *parentwin;
  uiwcont_t         *wcont [UIIMPPL_W_MAX];
  asconf_t          *asconf;
  callback_t        *responsecb;
  uidd_t            *plselect;
  nlist_t           *options;
  nlist_t           *aslist;
  nlist_t           *askeys;
  ilist_t           *plnames;
  callback_t        *callbacks [UIIMPPL_CB_MAX];
  char              origplname [MAXPATHLEN];
  size_t            asmaxwidth;
  int               askey;
  int               imptype;
  int               asconfcount;
  bool              isactive;
  bool              in_cb;
} uiimppl_t;

/* import playlist */
static void   uiimpplCreateDialog (uiimppl_t *uiimppl);
static bool   uiimpplTargetDialog (void *udata);
static void   uiimpplInitDisplay (uiimppl_t *uiimppl);
static bool   uiimpplResponseHandler (void *udata, int32_t responseid);
static int    uiimpplValidateTarget (uiwcont_t *entry, const char *label, void *udata);
static bool   uiimpplSelectHandler (void *udata, int32_t idx);
static int    uiimpplValidateNewName (uiwcont_t *entry, const char *label, void *udata);
static bool uiimpplImportTypeCallback (void *udata);

uiimppl_t *
uiimpplInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiimppl_t   *uiimppl;
  const char  *asnm;
  size_t      len;
  ilistidx_t  asiteridx;
  ilistidx_t  key;
  int         count;
  char        tbuff [40];

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
  uiimppl->isactive = false;
  uiimppl->in_cb = false;
  uiimppl->aslist = NULL;
  uiimppl->askeys = NULL;
  uiimppl->plnames = NULL;
  uiimppl->plselect = NULL;
  uiimppl->asmaxwidth = 0;
  uiimppl->askey = -1;
  uiimppl->imptype = AUDIOSRC_TYPE_NONE;
  *uiimppl->origplname = '\0';
  uiimppl->asconf = NULL;
  uiimppl->asconfcount = 0;

  uiimppl->asconf = asconfAlloc ();
  uiimppl->asconfcount = asconfGetCount (uiimppl->asconf);

  count = 0;
  uiimppl->aslist = nlistAlloc ("aslist", LIST_UNORDERED, NULL);
  uiimppl->askeys = nlistAlloc ("aslist", LIST_UNORDERED, NULL);

  /* CONTEXT: import playlist: playlist file types (mu3/xspf/jspf) */
  snprintf (tbuff, sizeof (tbuff), _("File (%s)"), "M3U/XSPF/JSPF");
  len = strlen (tbuff);
  uiimppl->asmaxwidth = len;
  nlistSetStr (uiimppl->aslist, count, tbuff);
  nlistSetNum (uiimppl->askeys, count, uiimppl->asconfcount);
  ++count;

  asconfStartIterator (uiimppl->asconf, &asiteridx);
  while ((key = asconfIterate (uiimppl->asconf, &asiteridx)) >= 0) {
    int   type;

    type = asconfGetNum (uiimppl->asconf, key, ASCONF_TYPE);
    asnm = asconfGetStr (uiimppl->asconf, key, ASCONF_NAME);
    len = strlen (asnm);
    if (len > uiimppl->asmaxwidth) {
      uiimppl->asmaxwidth = len;
    }
    nlistSetStr (uiimppl->aslist, count, asnm);
    nlistSetNum (uiimppl->askeys, count, key);
    ++count;
  }
  nlistSort (uiimppl->aslist);
  nlistSort (uiimppl->askeys);

  uiimppl->imptype = nlistGetNum (uiimppl->options, MANAGE_IMP_PL_TYPE);
  if (uiimppl->imptype < 0) {
    uiimppl->imptype = AUDIOSRC_TYPE_FILE;
  }

  uiimppl->callbacks [UIIMPPL_CB_DIALOG] = callbackInitI (
      uiimpplResponseHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TARGET] = callbackInit (
      uiimpplTargetDialog, uiimppl, NULL);
  uiimppl->callbacks [UIIMPPL_CB_SEL] = callbackInitI (
      uiimpplSelectHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL] = callbackInit (
      uiimpplImportTypeCallback, uiimppl, NULL);

  return uiimppl;
}

void
uiimpplFree (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }

  ilistFree (uiimppl->plnames);
  uiimppl->plnames = NULL;
  nlistFree (uiimppl->aslist);
  uiimppl->aslist = NULL;
  nlistFree (uiimppl->askeys);
  uiimppl->askeys = NULL;
  uiddFree (uiimppl->plselect);
  uiimppl->plselect = NULL;
  for (int j = 0; j < UIIMPPL_W_MAX; ++j) {
    uiwcontFree (uiimppl->wcont [j]);
    uiimppl->wcont [j] = NULL;
  }
  for (int i = 0; i < UIIMPPL_CB_MAX; ++i) {
    callbackFree (uiimppl->callbacks [i]);
    uiimppl->callbacks [i] = NULL;
  }
  asconfFree (uiimppl->asconf);
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
  uiimpplCreateDialog (uiimppl);
  uiDialogShow (uiimppl->wcont [UIIMPPL_W_DIALOG]);
  uiimpplInitDisplay (uiimppl);
  uiimppl->isactive = true;
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  x = nlistGetNum (uiimppl->options, IMP_PL_POSITION_X);
  y = nlistGetNum (uiimppl->options, IMP_PL_POSITION_Y);
  uiWindowMove (uiimppl->wcont [UIIMPPL_W_DIALOG], x, y, -1);
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

  uiEntryValidate (uiimppl->wcont [UIIMPPL_W_URI], false);
  uiEntryValidate (uiimppl->wcont [UIIMPPL_W_NEWNAME], false);
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
  const char    *uri;

  if (uiimppl == NULL) {
    return NULL;
  }

  uri = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_URI]);
  return uri;
}

const char *
uiimpplGetNewName (uiimppl_t *uiimppl)
{
  const char    *newname;

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
  const char    *odir = NULL;
  char          tbuff [MAXPATHLEN];

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
      RESPONSE_A,
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

  /* status msg */
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
  uiSpinboxTextSetValue (uiwidgetp, uiimppl->imptype);
  uiSpinboxTextSetValueChangedCallback (uiwidgetp,
      uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL]);
  uiimppl->wcont [UIIMPPL_W_IMP_TYPE] = uiwidgetp;
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* playlist selector */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_PL_SEL_LABEL] = uiwidgetp;

  uiimppl->plselect = uiddCreate ("imppl-plsel",
      uiimppl->wcont [UIIMPPL_W_DIALOG], hbox,
      /* CONTEXT: import playlist: select the playlist */
      DD_PACK_START, NULL, DD_LIST_TYPE_NUM, "", DD_REPLACE_TITLE,
      uiimppl->callbacks [UIIMPPL_CB_SEL]);

  uiwcontFree (hbox);

  /* target folder */
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
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_URI] = uiwidgetp;

  odir = nlistGetStr (uiimppl->options, MANAGE_IMP_PL_DIR);
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  stpecpy (tbuff, tbuff + sizeof (tbuff), odir);
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_URI], tbuff);
  uiEntrySetValidate (uiwidgetp, "",
      uiimpplValidateTarget, uiimppl, UIENTRY_DELAYED);

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

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: new song list name */
      _("New Song List Name"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_NEWNAME] = uiwidgetp;

  /* CONTEXT: import playlist: select the song list */
  uiEntrySetValidate (uiwidgetp, _("Playlist"),
      uiimpplValidateNewName, uiimppl, UIENTRY_IMMEDIATE);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

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

  uiimpplImportTypeCallback (uiimppl);
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

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: del window");
      uiimppl->imptype = AUDIOSRC_TYPE_NONE;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: close window");
      uiWidgetHide (uiimppl->wcont [UIIMPPL_W_DIALOG]);
      uiimppl->imptype = AUDIOSRC_TYPE_NONE;
      break;
    }
    case RESPONSE_A: {
      bool    rc;

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: import playlist: check connection");

      rc = audiosrcCheckConnection (uiimppl->askey);
      if (rc == false) {
        uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
            /* CONTEXT: configuration: audio source: check connection status */
            _("Connection Failed"));
      } else {
        uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
            /* CONTEXT: configuration: audio source: check connection status */
            _("Connection OK"));
      }
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
      break;
    }
  }

  uiimppl->isactive = false;
  return UICB_CONT;
}


static int
uiimpplValidateTarget (uiwcont_t *entry, const char *label, void *udata)
{
  uiimppl_t  *uiimppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];
  pathinfo_t  *pi;

  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");

  str = uiEntryGetValue (entry);

  if (uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
    bool    extok = false;
    *tbuff = '\0';
    stpecpy (tbuff, tbuff + sizeof (tbuff), str);
    pathNormalizePath (tbuff, sizeof (tbuff));
    pi = pathInfo (tbuff);
    if (pathInfoExtCheck (pi, ".m3u") ||
        pathInfoExtCheck (pi, ".m3u8") ||
        pathInfoExtCheck (pi, ".xspf") ||
        pathInfoExtCheck (pi, ".jspf")) {
      extok = true;
    }

    if (*tbuff && (! extok || ! fileopFileExists (tbuff))) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: import playlist: invalid target file */
          _("Invalid File"));
      pathInfoFree (pi);
      return UIENTRY_ERROR;
    }

    snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
    uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME], tbuff);
    pathInfoFree (pi);
  }

  if (uiimppl->imptype == AUDIOSRC_TYPE_BDJ4) {
    if (strncmp (str, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) != 0) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: import playlist: invalid URI */
          _("Invalid URI"));
      return UIENTRY_ERROR;
    }
// ### need to get origplname from target
  }

  return UIENTRY_OK;
}

static bool
uiimpplSelectHandler (void *udata, int idx)
{
  uiimppl_t   *uiimppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];

  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  str = ilistGetStr (uiimppl->plnames, idx, DD_LIST_DISP);
  snprintf (tbuff, sizeof (tbuff), "%s%s:%" PRIu16 "/%s",
      AS_BDJ4_PFX,
      asconfGetStr (uiimppl->asconf, uiimppl->askey, ASCONF_URI),
      (uint16_t) asconfGetNum (uiimppl->asconf, uiimppl->askey, ASCONF_PORT),
      str);
  stpecpy (uiimppl->origplname,
      uiimppl->origplname + sizeof (uiimppl->origplname), str);
  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_URI], tbuff);
  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME], str);
  return UICB_CONT;
}

static int
uiimpplValidateNewName (uiwcont_t *entry, const char *label, void *udata)
{
  uiimppl_t  *uiimppl = udata;
  uiwcont_t   *statusMsg = NULL;
  uiwcont_t   *errorMsg = NULL;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];

  statusMsg = uiimppl->wcont [UIIMPPL_W_STATUS_MSG];
  errorMsg = uiimppl->wcont [UIIMPPL_W_ERROR_MSG];
  uiLabelSetText (statusMsg, "");
  uiLabelSetText (errorMsg, "");
  rc = uiutilsValidatePlaylistName (entry, label, errorMsg);

  if (rc == UIENTRY_OK) {
    str = uiEntryGetValue (entry);
    if (*str) {
      pathbldMakePath (fn, sizeof (fn),
          str, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
      if (fileopFileExists (fn)) {
        /* CONTEXT: import playlist: message if the target song list already exists */
        snprintf (tbuff, sizeof (tbuff), "%s: %s", str, _("Already Exists"));
        uiLabelSetText (statusMsg, tbuff);
      }
    }
  }

  return rc;
}

static bool
uiimpplImportTypeCallback (void *udata)
{
  uiimppl_t   *uiimppl = udata;
  char        tbuff [40];
  int         askey;

  if (uiimppl->in_cb) {
    return UICB_CONT;
  }
  uiimppl->in_cb = true;

  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_STATUS_MSG], "");

  askey = uiSpinboxTextGetValue (uiimppl->wcont [UIIMPPL_W_IMP_TYPE]);
  uiimppl->askey = askey;
  if (askey == uiimppl->asconfcount) {
    uiimppl->imptype = AUDIOSRC_TYPE_FILE;
  } else {
    uiimppl->imptype = asconfGetNum (uiimppl->asconf, askey, ASCONF_TYPE);
  }

  if (uiimppl->imptype != AUDIOSRC_TYPE_FILE) {
    asiter_t    *asiter;
    const char  *plnm;
    int         idx;

    idx = 0;
    ilistFree (uiimppl->plnames);
    asiter = audiosrcStartIterator (uiimppl->imptype, AS_ITER_PL_NAMES, NULL, askey);
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
    snprintf (tbuff, sizeof (tbuff), "%s:", _("File"));
  } else {
    uiWidgetSetState (uiimppl->wcont [UIIMPPL_W_PL_SEL_LABEL], UIWIDGET_ENABLE);
    uiddSetState (uiimppl->plselect, UIWIDGET_ENABLE);
    uiWidgetHide (uiimppl->wcont [UIIMPPL_W_URI_BUTTON]);
    /* CONTEXT: import playlist: import location */
    snprintf (tbuff, sizeof (tbuff), "%s:", _("URI"));
  }
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_URI_LABEL], tbuff);

  uiimppl->in_cb = false;
  return UICB_CONT;
}

