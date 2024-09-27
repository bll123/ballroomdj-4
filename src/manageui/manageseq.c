/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "dance.h"
#include "filemanip.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "pathbld.h"
#include "playlist.h"
#include "tagdef.h"
#include "ui.h"
#include "uiduallist.h"
#include "uiselectfile.h"
#include "uiutils.h"

enum {
  MSEQ_MENU_CB_SEQ_LOAD,
  MSEQ_MENU_CB_SEQ_COPY,
  MSEQ_MENU_CB_SEQ_NEW,
  MSEQ_MENU_CB_SEQ_DELETE,
  MSEQ_CB_SEL_FILE,
  MSEQ_CB_MAX,
};

typedef struct manageseq {
  manageinfo_t    *minfo;
  uiwcont_t       *seqmenu;
  callback_t      *callbacks [MSEQ_CB_MAX];
  callback_t      *seqloadcb;
  callback_t      *seqnewcb;
  uiduallist_t    *seqduallist;
  uiwcont_t       *seqname;
  char            *seqoldname;
  const char      *newseqname;
  bool            seqbackupcreated : 1;
  bool            changed : 1;
  bool            inload : 1;
} manageseq_t;

static bool   manageSequenceLoad (void *udata);
static int32_t manageSequenceLoadCB (void *udata, const char *fn);
static bool   manageSequenceCopy (void *udata);
static bool   manageSequenceNew (void *udata);
static bool   manageSequenceDelete (void *udata);
static void   manageSetSequenceName (manageseq_t *manageseq, const char *nm);

manageseq_t *
manageSequenceAlloc (manageinfo_t *minfo)
{
  manageseq_t *manageseq;

  manageseq = mdmalloc (sizeof (manageseq_t));
  manageseq->minfo = minfo;
  manageseq->seqduallist = NULL;
  manageseq->seqoldname = NULL;
  manageseq->seqbackupcreated = false;
  manageseq->seqmenu = uiMenuAlloc ();
  manageseq->seqname = NULL;
  manageseq->changed = false;
  manageseq->inload = false;
  manageseq->seqloadcb = NULL;
  manageseq->seqnewcb = NULL;
  /* CONTEXT: sequence editor: default name for a new sequence */
  manageseq->newseqname = _("New Sequence");
  for (int i = 0; i < MSEQ_CB_MAX; ++i) {
    manageseq->callbacks [i] = NULL;
  }

  manageseq->callbacks [MSEQ_CB_SEL_FILE] =
      callbackInitS (manageSequenceLoadCB, manageseq);

  return manageseq;
}

void
manageSequenceFree (manageseq_t *manageseq)
{
  if (manageseq == NULL) {
    return;
  }

  uiwcontFree (manageseq->seqmenu);
  uiduallistFree (manageseq->seqduallist);
  dataFree (manageseq->seqoldname);
  uiwcontFree (manageseq->seqname);
  for (int i = 0; i < MSEQ_CB_MAX; ++i) {
    callbackFree (manageseq->callbacks [i]);
  }
  mdfree (manageseq);
}

void
manageSequenceSetLoadCallback (manageseq_t *manageseq, callback_t *uicb)
{
  if (manageseq == NULL) {
    return;
  }
  manageseq->seqloadcb = uicb;
}

void
manageSequenceSetNewCallback (manageseq_t *manageseq, callback_t *uicb)
{
  if (manageseq == NULL) {
    return;
  }
  manageseq->seqnewcb = uicb;
}

void
manageBuildUISequence (manageseq_t *manageseq, uiwcont_t *vboxp)
{
  uiwcont_t           *hbox;
  uiwcont_t           *uiwidgetp;
  dance_t             *dances;
  slist_t             *dancelist;

  logProcBegin ();

  /* edit sequences */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  /* CONTEXT: sequence editor: label for sequence name */
  uiwidgetp = uiCreateColonLabel (_("Sequence"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, 100);
  manageseq->seqname = uiwidgetp;
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  manageSetSequenceName (manageseq, manageseq->newseqname);
  uiBoxPackStart (hbox, uiwidgetp);
  /* CONTEXT: sequence editor: sequence name */
  uiEntrySetValidate (manageseq->seqname, _("Sequence"),
      uiutilsValidatePlaylistName,
      manageseq->minfo->errorMsg, UIENTRY_IMMEDIATE);

  manageseq->seqduallist = uiCreateDualList (vboxp,
      DL_FLAGS_MULTIPLE | DL_FLAGS_PERSISTENT,
      tagdefs [TAG_DANCE].displayname,
      /* CONTEXT: sequence editor: title for the sequence list  */
      _("Sequence"));

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  /* as the source list is persistent, it is ok to set it before */
  /* setting the duallist target */
  uiduallistSet (manageseq->seqduallist, dancelist, DL_LIST_SOURCE);

  uiwcontFree (hbox);

  logProcEnd ("");
}

uiwcont_t *
manageSequenceMenu (manageseq_t *manageseq, uiwcont_t *uimenubar)
{
  uiwcont_t   *menu = NULL;
  uiwcont_t   *menuitem = NULL;

  logProcBegin ();
  if (uiMenuIsInitialized (manageseq->seqmenu)) {
    uiMenuDisplay (manageseq->seqmenu);
    logProcEnd ("is-init");
    return manageseq->seqmenu;
  }

  menuitem = uiMenuAddMainItem (uimenubar,
      /* CONTEXT: sequence editor: menu selection: sequence: edit menu */
      manageseq->seqmenu, _("Edit"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageseq->callbacks [MSEQ_MENU_CB_SEQ_LOAD] = callbackInit (
      manageSequenceLoad, manageseq, NULL);
  /* CONTEXT: sequence editor: menu selection: sequence: edit menu: load */
  menuitem = uiMenuCreateItem (menu, _("Load"),
      manageseq->callbacks [MSEQ_MENU_CB_SEQ_LOAD]);
  uiwcontFree (menuitem);

  manageseq->callbacks [MSEQ_MENU_CB_SEQ_NEW] = callbackInit (
      manageSequenceNew, manageseq, NULL);
  /* CONTEXT: sequence editor: menu selection: sequence: edit menu: start new sequence */
  menuitem = uiMenuCreateItem (menu, _("Start New Sequence"),
      manageseq->callbacks [MSEQ_MENU_CB_SEQ_NEW]);
  uiwcontFree (menuitem);

  manageseq->callbacks [MSEQ_MENU_CB_SEQ_COPY] = callbackInit (
      manageSequenceCopy, manageseq, NULL);
  /* CONTEXT: sequence editor: menu selection: sequence: edit menu: create copy */
  menuitem = uiMenuCreateItem (menu, _("Create Copy"),
      manageseq->callbacks [MSEQ_MENU_CB_SEQ_COPY]);
  uiwcontFree (menuitem);

  manageseq->callbacks [MSEQ_MENU_CB_SEQ_DELETE] = callbackInit (
      manageSequenceDelete, manageseq, NULL);
  /* CONTEXT: sequence editor: menu selection: sequence: edit menu: delete sequence */
  menuitem = uiMenuCreateItem (menu, _("Delete"),
      manageseq->callbacks [MSEQ_MENU_CB_SEQ_DELETE]);
  uiwcontFree (menuitem);

  uiMenuSetInitialized (manageseq->seqmenu);
  uiwcontFree (menu);

  uiMenuDisplay (manageseq->seqmenu);

  logProcEnd ("");
  return manageseq->seqmenu;
}

void
manageSequenceSave (manageseq_t *manageseq)
{
  sequence_t  *seq = NULL;
  slist_t     *slist;
  char        nnm [MAXPATHLEN];
  char        *name;
  bool        changed = false;
  bool        notvalid = false;

  logProcBegin ();
  if (manageseq->seqoldname == NULL) {
    logProcEnd ("no-old-name");
    return;
  }

  slist = uiduallistGetList (manageseq->seqduallist);
  if (slistGetCount (slist) <= 0) {
    slistFree (slist);
    logProcEnd ("no-seq-data");
    return;
  }

  if (uiduallistIsChanged (manageseq->seqduallist)) {
    changed = true;
  }

  name = manageGetEntryValue (manageseq->seqname);
  notvalid = false;
  if (uiEntryIsNotValid (manageseq->seqname)) {
    mdfree (name);
    name = mdstrdup (manageseq->seqoldname);
    uiEntrySetValue (manageseq->seqname, manageseq->seqoldname);
    notvalid = true;
  }

  /* the sequence has been renamed */
  if (strcmp (manageseq->seqoldname, name) != 0) {
    playlistRename (manageseq->seqoldname, name);
    changed = true;
  }

  if (! changed) {
    mdfree (name);
    slistFree (slist);
    logProcEnd ("not-changed");
    return;
  }

  /* need the full name for backups */
  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  if (! manageseq->seqbackupcreated) {
    filemanipBackup (nnm, 1);
    manageseq->seqbackupcreated = true;
  }

  manageSetSequenceName (manageseq, name);
  seq = sequenceCreate (name);
  sequenceSave (seq, slist);
  sequenceFree (seq);

  playlistCheckAndCreate (name, PLTYPE_SEQUENCE);
  slistFree (slist);
  if (manageseq->seqloadcb != NULL) {
    callbackHandlerS (manageseq->seqloadcb, name);
  }
  mdfree (name);

  if (notvalid) {
    /* set the message after the entry field has been reset */
    /* CONTEXT: Saving Sequence: Error message for invalid sequence name. */
    uiLabelSetText (manageseq->minfo->errorMsg, _("Invalid name. Using old name."));
  }

  logProcEnd ("");
}

/* the current sequence may be renamed or deleted. */
/* check for this and if the sequence has */
/* disappeared, reset */
void
manageSequenceLoadCheck (manageseq_t *manageseq)
{
  char    *name;

  logProcBegin ();
  if (manageseq->seqoldname == NULL) {
    logProcEnd ("no-old-name");
    return;
  }

  name = manageGetEntryValue (manageseq->seqname);

  if (! sequenceExists (name)) {
    /* make sure no save happens */
    dataFree (manageseq->seqoldname);
    manageseq->seqoldname = NULL;
    manageSequenceNew (manageseq);
  } else {
    if (manageseq->seqloadcb != NULL) {
      callbackHandlerS (manageseq->seqloadcb, name);
    }
  }
  mdfree (name);
  logProcEnd ("");
}

void
manageSequenceLoadFile (manageseq_t *manageseq, const char *fn, int preloadflag)
{
  sequence_t  *seq = NULL;
  const char  *dstr = NULL;
  nlist_t     *dancelist = NULL;
  slist_t     *tlist = NULL;
  nlistidx_t  iteridx;
  nlistidx_t  didx;

  if (manageseq->inload) {
    return;
  }
  logProcBegin ();

  logMsg (LOG_DBG, LOG_ACTIONS, "load sequence file");
  if (! sequenceExists (fn)) {
    logProcEnd ("no-seq-file");
    return;
  }

  manageseq->inload = true;

  if (preloadflag == MANAGE_STD) {
    manageSequenceSave (manageseq);
  }

  seq = sequenceLoad (fn);
  if (seq == NULL) {
    logProcEnd ("null");
    manageseq->inload = false;
    return;
  }

  dancelist = sequenceGetDanceList (seq);
  tlist = slistAlloc ("temp-seq", LIST_UNORDERED, NULL);
  nlistStartIterator (dancelist, &iteridx);
  while ((didx = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    dstr = nlistGetStr (dancelist, didx);
    slistSetNum (tlist, dstr, didx);
  }
  uiduallistSet (manageseq->seqduallist, tlist, DL_LIST_TARGET);
  uiduallistClearChanged (manageseq->seqduallist);
  slistFree (tlist);

  manageSetSequenceName (manageseq, fn);
  if (manageseq->seqloadcb != NULL && preloadflag == MANAGE_STD) {
    callbackHandlerS (manageseq->seqloadcb, fn);
  }

  sequenceFree (seq);
  manageseq->seqbackupcreated = false;
  manageseq->inload = false;
  logProcEnd ("");
}

/* internal routines */

static bool
manageSequenceLoad (void *udata)
{
  manageseq_t  *manageseq = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load sequence");
  uiLabelSetText (manageseq->minfo->statusMsg, "");
  manageSequenceSave (manageseq);
  selectFileDialog (SELFILE_SEQUENCE, manageseq->minfo->window,
      manageseq->minfo->options,
      manageseq->callbacks [MSEQ_CB_SEL_FILE]);
  logProcEnd ("");
  return UICB_CONT;
}

static int32_t
manageSequenceLoadCB (void *udata, const char *fn)
{
  manageseq_t *manageseq = udata;

  logProcBegin ();
  manageSequenceLoadFile (manageseq, fn, MANAGE_STD);
  logProcEnd ("");
  return 0;
}

static bool
manageSequenceCopy (void *udata)
{
  manageseq_t *manageseq = udata;
  char        *oname;
  char        newname [200];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy sequence");
  manageSequenceSave (manageseq);

  oname = manageGetEntryValue (manageseq->seqname);

  /* CONTEXT: sequence editor: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manageseq->minfo->errorMsg, oname, newname)) {
    manageSetSequenceName (manageseq, newname);
    manageseq->seqbackupcreated = false;
    uiduallistClearChanged (manageseq->seqduallist);
    if (manageseq->seqloadcb != NULL) {
      callbackHandlerS (manageseq->seqloadcb, newname);
    }
  }
  mdfree (oname);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSequenceNew (void *udata)
{
  manageseq_t *manageseq = udata;
  char        tbuff [200];
  slist_t     *tlist;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new sequence");
  uiLabelSetText (manageseq->minfo->statusMsg, "");
  manageSequenceSave (manageseq);

  stpecpy (tbuff, tbuff + sizeof (tbuff), manageseq->newseqname);
  manageSetSequenceName (manageseq, tbuff);
  manageseq->seqbackupcreated = false;
  tlist = slistAlloc ("tmp-sequence", LIST_UNORDERED, NULL);
  uiduallistSet (manageseq->seqduallist, tlist, DL_LIST_TARGET);
  uiduallistClearChanged (manageseq->seqduallist);
  slistFree (tlist);
  if (manageseq->seqnewcb != NULL) {
    callbackHandler (manageseq->seqnewcb);
  }
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSequenceDelete (void *udata)
{
  manageseq_t *manageseq = udata;
  char        *oname;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: delete sequence");
  oname = manageGetEntryValue (manageseq->seqname);
  manageDeletePlaylist (oname);
  uiduallistClearChanged (manageseq->seqduallist);
  manageSequenceNew (manageseq);
  manageDeleteStatus (manageseq->minfo->statusMsg, oname);
  mdfree (oname);
  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSetSequenceName (manageseq_t *manageseq, const char *name)
{

  logProcBegin ();
  dataFree (manageseq->seqoldname);
  manageseq->seqoldname = mdstrdup (name);
  uiEntrySetValue (manageseq->seqname, name);
  logProcEnd ("");
}
