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

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "filemanip.h"
#include "manageui.h"
#include "pathbld.h"
#include "playlist.h"
#include "ui.h"
#include "uiduallist.h"
#include "uiselectfile.h"
#include "uiutils.h"

typedef struct manageseq {
  UIWidget        *windowp;
  nlist_t         *options;
  uimenu_t        seqmenu;
  uiduallist_t    *seqduallist;
  uientry_t       seqname;
  UIWidget        *statusMsg;
  char            *seqoldname;
  bool            seqbackupcreated;
} manageseq_t;

static void   manageSequenceLoad (GtkMenuItem *mi, gpointer udata);
static void   manageSequenceCopy (GtkMenuItem *mi, gpointer udata);
static void   manageSequenceLoadFile (void *udata, const char *fn);
static void   manageSequenceNew (GtkMenuItem *mi, gpointer udata);
static void   manageSetSequenceName (manageseq_t *manageseq, const char *nm);

manageseq_t *
manageSequenceAlloc (UIWidget *window, nlist_t *options, UIWidget *statusMsg)
{
  manageseq_t *manageseq;

  manageseq = malloc (sizeof (manageseq_t));
  manageseq->seqduallist = NULL;
  manageseq->seqoldname = NULL;
  manageseq->seqbackupcreated = false;
  uiMenuInit (&manageseq->seqmenu);
  uiEntryInit (&manageseq->seqname, 20, 50);
  manageseq->statusMsg = statusMsg;
  manageseq->windowp = window;
  manageseq->options = options;

  return manageseq;
}

void
manageSequenceFree (manageseq_t *manageseq)
{
  if (manageseq != NULL) {
    if (manageseq->seqoldname != NULL) {
      free (manageseq->seqoldname);
    }
    uiEntryFree (&manageseq->seqname);
    free (manageseq);
  }
}

void
manageBuildUISequence (manageseq_t *manageseq, UIWidget *vboxp)
{
  GtkWidget           *widget;
  UIWidget            hbox;
  UIWidget            uiwidget;
  dance_t             *dances;
  slist_t             *dancelist;

  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  /* edit sequences */
  uiWidgetSetAllMargins (vboxp, uiBaseMarginSz * 2);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (vboxp, &hbox);

  /* CONTEXT: sequence editor: label for sequence name */
  uiCreateColonLabel (&uiwidget, _("Sequence"));
  uiBoxPackStart (&hbox, &uiwidget);

  widget = uiEntryCreate (&manageseq->seqname);
  uiEntrySetColor (&manageseq->seqname, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  /* CONTEXT: sequence: default name for a new sequence */
  manageSetSequenceName (manageseq, _("New Sequence"));
  uiBoxPackStartUW (&hbox, widget);

  manageseq->seqduallist = uiCreateDualList (vboxp,
      DUALLIST_FLAGS_MULTIPLE | DUALLIST_FLAGS_PERSISTENT,
      /* CONTEXT: sequence editor: titles for the selection list and the sequence list  */
      _("Dance"), _("Sequence"));

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  uiduallistSet (manageseq->seqduallist, dancelist, DUALLIST_TREE_SOURCE);
}

uimenu_t *
manageSequenceMenu (manageseq_t *manageseq, GtkWidget *menubar)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manageseq->seqmenu.initialized) {
    menuitem = uiMenuAddMainItem (menubar,
        /* CONTEXT: menu selection: sequence: edit menu */
        &manageseq->seqmenu, _("Edit"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: sequence: edit menu: load */
    menuitem = uiMenuCreateItem (menu, _("Load"),
        manageSequenceLoad, manageseq);

    /* CONTEXT: menu selection: sequence: edit menu: create copy */
    menuitem = uiMenuCreateItem (menu, _("Create Copy"),
        manageSequenceCopy, manageseq);

    /* CONTEXT: menu selection: sequence: edit menu: start new sequence */
    menuitem = uiMenuCreateItem (menu, _("Start New Sequence"),
        manageSequenceNew, manageseq);

    manageseq->seqmenu.initialized = true;
  }

  uiMenuDisplay (&manageseq->seqmenu);
  return &manageseq->seqmenu;
}

void
manageSequenceSave (manageseq_t *manageseq)
{
  sequence_t  *seq = NULL;
  slist_t     *slist;
  char        onm [MAXPATHLEN];
  char        nnm [MAXPATHLEN];
  const char  *name;

  if (manageseq->seqoldname == NULL) {
    return;
  }

  slist = uiduallistGetList (manageseq->seqduallist);
  if (slistGetCount (slist) <= 0) {
    slistFree (slist);
    return;
  }

  name = uiEntryGetValue (&manageseq->seqname);

  /* the song list has been renamed */
  if (strcmp (manageseq->seqoldname, name) != 0) {
    manageRenamePlaylistFiles (manageseq->seqoldname, name);
  }

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  strlcat (onm, ".n", sizeof (onm));

  seq = sequenceCreate (name);
  sequenceSave (seq, slist);
  sequenceFree (seq);

  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  if (! manageseq->seqbackupcreated) {
    filemanipBackup (nnm, 1);
    manageseq->seqbackupcreated = true;
  }
  filemanipMove (onm, nnm);

  manageCheckAndCreatePlaylist (name, nnm, PLTYPE_SEQ);
  slistFree (slist);
}

/* internal routines */

static void
manageSequenceLoad (GtkMenuItem *mi, gpointer udata)
{
  manageseq_t  *manageseq = udata;

  selectFileDialog (SELFILE_SEQUENCE, manageseq->windowp, manageseq->options,
      manageseq, manageSequenceLoadFile);
}

static void
manageSequenceLoadFile (void *udata, const char *fn)
{
  manageseq_t *manageseq = udata;
  sequence_t  *seq = NULL;
  char        *dstr = NULL;
  nlist_t     *dancelist = NULL;
  slist_t     *tlist = NULL;
  nlistidx_t  iteridx;
  nlistidx_t  didx;

  manageSequenceSave (manageseq);

  seq = sequenceAlloc (fn);
  if (seq == NULL) {
    return;
  }

  dancelist = sequenceGetDanceList (seq);
  tlist = slistAlloc ("temp-seq", LIST_UNORDERED, NULL);
  nlistStartIterator (dancelist, &iteridx);
  while ((didx = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    dstr = nlistGetStr (dancelist, didx);
    slistSetNum (tlist, dstr, didx);
  }
  uiduallistSet (manageseq->seqduallist, tlist, DUALLIST_TREE_TARGET);
  slistFree (tlist);

  manageSetSequenceName (manageseq, fn);
  manageseq->seqbackupcreated = false;
}

static void
manageSequenceCopy (GtkMenuItem *mi, gpointer udata)
{
  manageseq_t *manageseq = udata;
  const char  *oname;
  char        newname [200];

  manageSequenceSave (manageseq);

  oname = uiEntryGetValue (&manageseq->seqname);
  /* CONTEXT: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manageseq->statusMsg, oname, newname)) {
    manageSetSequenceName (manageseq, newname);
    manageseq->seqbackupcreated = false;
  }
}

static void
manageSequenceNew (GtkMenuItem *mi, gpointer udata)
{
  manageseq_t *manageseq = udata;
  char        tbuff [200];
  slist_t     *tlist;

  manageSequenceSave (manageseq);

  /* CONTEXT: sequence: default name for a new sequence */
  snprintf (tbuff, sizeof (tbuff), _("New Sequence"));
  manageSetSequenceName (manageseq, tbuff);
  manageseq->seqbackupcreated = false;
  tlist = slistAlloc ("tmp-sequence", LIST_UNORDERED, NULL);
  uiduallistSet (manageseq->seqduallist, tlist, DUALLIST_TREE_TARGET);
  slistFree (tlist);
}

static void
manageSetSequenceName (manageseq_t *manageseq, const char *name)
{
  uiEntrySetValue (&manageseq->seqname, name);
  if (manageseq->seqoldname != NULL) {
    free (manageseq->seqoldname);
  }
  manageseq->seqoldname = strdup (name);
}
