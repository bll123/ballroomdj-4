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
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "dance.h"
#include "datafile.h"
#include "dnctypes.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "nlist.h"
#include "pathutil.h"
#include "slist.h"

/* dance table */
static void confuiCreateDanceTable (confuigui_t *gui);
static int  confuiDanceEntryDanceChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryTagsChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryChg (uientry_t *e, void *udata, int widx);
static bool confuiDanceSpinboxTypeChg (void *udata);
static bool confuiDanceSpinboxSpeedChg (void *udata);
static bool confuiDanceSpinboxLowBPMChg (void *udata);
static bool confuiDanceSpinboxHighBPMChg (void *udata);
static bool confuiDanceSpinboxTimeSigChg (void *udata);
static void confuiDanceSpinboxChg (void *udata, int widx);
static int  confuiDanceValidateAnnouncement (uientry_t *entry, confuigui_t *gui);
static void confuiDanceSave (confuigui_t *gui);
static bool confuiSelectAnnouncement (void *udata);
static void confuiLoadDanceTypeList (confuigui_t *gui);

void
confuiInitEditDances (confuigui_t *gui)
{
  confuiLoadDanceTypeList (gui);

  confuiSpinboxTextInitDataNum (gui, "cu-dance-speed",
      CONFUI_SPINBOX_DANCE_SPEED,
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_SLOW, _("slow"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_NORMAL, _("normal"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_FAST, _("fast"),
      -1);

  confuiSpinboxTextInitDataNum (gui, "cu-dance-time-sig",
      CONFUI_SPINBOX_DANCE_TIME_SIG,
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_24, _("2/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_34, _("3/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_44, _("4/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_48, _("4/8"),
      -1);
}


void
confuiBuildUIEditDances (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      sg;
  UIWidget      sgB;
  UIWidget      sgC;
  const char    *bpmstr;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiBuildUIEditDances");
  gui->indancechange = true;
  uiCreateVertBox (&vbox);

  /* edit dances */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit the dance table */
      _("Edit Dances"), CONFUI_ID_DANCE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);
  uiCreateSizeGroupHoriz (&sgC);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_DANCE, CONFUI_TABLE_NO_UP_DOWN);
  gui->tables [CONFUI_ID_DANCE].savefunc = confuiDanceSave;
  confuiCreateDanceTable (gui);
  g_signal_connect (gui->tables [CONFUI_ID_DANCE].tree, "row-activated",
      G_CALLBACK (confuiDanceSelect), gui);

  uiCreateVertBox (&dvbox);
  uiWidgetSetMarginStart (&dvbox, 8);
  uiBoxPackStart (&hbox, &dvbox);

  confuiMakeItemEntry (gui, &dvbox, &sg, tagdefs [TAG_DANCE].displayname,
      CONFUI_ENTRY_DANCE_DANCE, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].entry,
      confuiDanceEntryDanceChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].danceidx = DANCE_DANCE;

  /* CONTEXT: configuration: dances: the type of the dance (club/latin/standard) */
  confuiMakeItemSpinboxText (gui, &dvbox, &sg, &sgB, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTypeChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].danceidx = DANCE_TYPE;

  /* CONTEXT: configuration: dances: the speed of the dance (fast/normal/slow) */
  confuiMakeItemSpinboxText (gui, &dvbox, &sg, &sgB, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxSpeedChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_SPEED].danceidx = DANCE_SPEED;

  /* CONTEXT: configuration: dances: tags associated with the dance */
  confuiMakeItemEntry (gui, &dvbox, &sg, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].entry,
      confuiDanceEntryTagsChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].danceidx = DANCE_TAGS;

  /* CONTEXT: configuration: dances: play the selected announcement before the dance is played */
  confuiMakeItemEntryChooser (gui, &dvbox, &sg, _("Announcement"),
      CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT, -1, "",
      confuiSelectAnnouncement);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].entry,
      confuiDanceEntryAnnouncementChg, gui, UIENTRY_DELAYED);
  gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].danceidx = DANCE_ANNOUNCE;

  bpmstr = tagdefs [TAG_BPM].displayname;
  /* CONTEXT: configuration: dances: low BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, &dvbox, &sg, &sgC, tbuff,
      CONFUI_SPINBOX_DANCE_LOW_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxLowBPMChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_LOW_BPM].danceidx = DANCE_LOW_BPM;

  /* CONTEXT: configuration: dances: high BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, &dvbox, &sg, &sgC, tbuff,
      CONFUI_SPINBOX_DANCE_HIGH_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxHighBPMChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_HIGH_BPM].danceidx = DANCE_HIGH_BPM;

  /* CONTEXT: configuration: dances: time signature for the dance */
  confuiMakeItemSpinboxText (gui, &dvbox, &sg, &sgC, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTimeSigChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].danceidx = DANCE_TIMESIG;

  gui->indancechange = false;
  logProcEnd (LOG_PROC, "confuiBuildUIEditDances", "");
}

/* internal routines */

static void
confuiCreateDanceTable (confuigui_t *gui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  slistidx_t        iteridx;
  ilistidx_t        key;
  dance_t           *dances;
  GtkWidget         *tree;
  slist_t           *dancelist;


  logProcBegin (LOG_PROC, "confuiCreateDanceTable");

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  store = gtk_list_store_new (CONFUI_DANCE_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
  assert (store != NULL);

  dancelist = danceGetDanceList (dances);
  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    gtk_list_store_append (store, &iter);
    confuiDanceSet (store, &iter, dancedisp, key);
    gui->tables [CONFUI_ID_DANCE].currcount += 1;
  }

  tree = gui->tables [CONFUI_ID_DANCE].tree;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_DANCE_COL_DANCE));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_DANCE_COL_DANCE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_DANCE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateDanceTable", "");
}

static int
confuiDanceEntryDanceChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_DANCE);
}

static int
confuiDanceEntryTagsChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_TAGS);
}

static int
confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT);
}

static int
confuiDanceEntryChg (uientry_t *entry, void *udata, int widx)
{
  confuigui_t     *gui = udata;
  const char      *str;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  glong           key;
  dance_t         *dances;
  int             didx;
  datafileconv_t  conv;
  int             entryrc = UIENTRY_ERROR;

  logProcBegin (LOG_PROC, "confuiDanceEntryChg");
  if (gui->indancechange) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "in-dance-select");
    return UIENTRY_OK;
  }

  str = uiEntryGetValue (entry);
  if (str == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "null-string");
    return UIENTRY_OK;
  }

  didx = gui->uiitem [widx].danceidx;

  tree = gui->tables [CONFUI_ID_DANCE].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "no-selection");
    return UIENTRY_OK;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &key, -1);

  if (widx == CONFUI_ENTRY_DANCE_DANCE) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        CONFUI_DANCE_COL_DANCE, str,
        -1);
    danceSetStr (dances, key, didx, str);
    entryrc = UIENTRY_OK;
  }
  if (widx == CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT) {
    entryrc = confuiDanceValidateAnnouncement (entry, gui);
    if (entryrc == UIENTRY_OK) {
      danceSetStr (dances, key, didx, str);
    }
  }
  if (widx == CONFUI_ENTRY_DANCE_TAGS) {
    slist_t *slist;

    conv.allocated = true;
    conv.str = strdup (str);
    conv.valuetype = VALUE_STR;
    convTextList (&conv);
    slist = conv.list;
    danceSetList (dances, key, didx, slist);
    entryrc = UIENTRY_OK;
  }
  if (entryrc == UIENTRY_OK) {
    gui->tables [gui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceEntryChg", "");
  return entryrc;
}

static bool
confuiDanceSpinboxTypeChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TYPE);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxSpeedChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_SPEED);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxLowBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_LOW_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxHighBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_HIGH_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxTimeSigChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TIME_SIG);
  return UICB_CONT;
}

static void
confuiDanceSpinboxChg (void *udata, int widx)
{
  confuigui_t     *gui = udata;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  glong           idx;
  double          value;
  long            nval = 0;
  ilistidx_t      key;
  dance_t         *dances;
  int             didx;

  logProcBegin (LOG_PROC, "confuiDanceSpinboxChg");
  if (gui->indancechange) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "in-dance-select");
    return;
  }

  didx = gui->uiitem [widx].danceidx;

  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_TEXT) {
    /* text spinbox */
    nval = uiSpinboxTextGetValue (gui->uiitem [widx].spinbox);
  }
  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_NUM) {
    value = uiSpinboxGetValue (&gui->uiitem [widx].uiwidget);
    nval = (long) value;
  }

  tree = gui->tables [CONFUI_ID_DANCE].tree;
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "no-selection");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;
  danceSetNum (dances, key, didx, nval);
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "");
}

static int
confuiDanceValidateAnnouncement (uientry_t *entry, confuigui_t *gui)
{
  int               rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];
  char              nfn [MAXPATHLEN];
  char              *musicdir;
  size_t            mlen;

  logProcBegin (LOG_PROC, "confuiDanceValidateAnnouncement");

  musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  mlen = strlen (musicdir);

  rc = UIENTRY_ERROR;
  fn = uiEntryGetValue (entry);
  if (fn == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "bad-fn");
    return UIENTRY_ERROR;
  }

  strlcpy (nfn, fn, sizeof (nfn));
  pathNormPath (nfn, sizeof (nfn));
  if (strncmp (musicdir, fn, mlen) == 0) {
    strlcpy (nfn, fn + mlen + 1, sizeof (nfn));
    uiEntrySetValue (entry, nfn);
  }

  if (*nfn == '\0') {
    rc = UIENTRY_OK;
  } else {
    *tbuff = '\0';
    if (*nfn != '/' && *(nfn + 1) != ':') {
      strlcpy (tbuff, musicdir, sizeof (tbuff));
      strlcat (tbuff, "/", sizeof (tbuff));
    }
    strlcat (tbuff, nfn, sizeof (tbuff));
    if (fileopFileExists (tbuff)) {
      rc = UIENTRY_OK;
    }
  }

  /* sanitizeaddress creates a buffer underflow error */
  /* if tablecurr is set to CONFUI_ID_NONE */
  /* also this validation routine gets called at most any time, but */
  /* the changed flag should only be set for the edit dance tab */
  if (gui->tablecurr == CONFUI_ID_DANCE) {
    gui->tables [gui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "");
  return rc;
}

static void
confuiDanceSave (confuigui_t *gui)
{
  dance_t   *dances;

  logProcBegin (LOG_PROC, "confuiDanceSave");

  if (gui->tables [CONFUI_ID_DANCE].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* the data is already saved in the dance list; just re-use it */
  danceSave (dances, NULL);
  logProcEnd (LOG_PROC, "confuiDanceSave", "");
}

static bool
confuiSelectAnnouncement (void *udata)
{
  confuigui_t *gui = udata;

  logProcBegin (LOG_PROC, "confuiSelectAnnouncement");
  confuiSelectFileDialog (gui, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT,
      /* CONTEXT: configuration: announcement selection dialog: audio file filter */
      bdjoptGetStr (OPT_M_DIR_MUSIC), _("Audio Files"), "audio/*");
  logProcEnd (LOG_PROC, "confuiSelectAnnouncement", "");
  return UICB_CONT;
}

static void
confuiLoadDanceTypeList (confuigui_t *gui)
{
  nlist_t       *tlist = NULL;
  nlist_t       *llist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  char          *key;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadDanceTypeList");

  tlist = nlistAlloc ("cu-dance-type", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-dance-type-l", LIST_ORDERED, free);

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  dnctypesStartIterator (dnctypes, &iteridx);
  count = 0;
  while ((key = dnctypesIterate (dnctypes, &iteridx)) != NULL) {
    nlistSetStr (tlist, count, key);
    nlistSetNum (llist, count, count);
    ++count;
  }

  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadDanceTypeList", "");
}

