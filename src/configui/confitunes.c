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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "configui.h"
#include "datafile.h"
#include "fileop.h"
#include "inline.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathutil.h"
#include "rating.h"
#include "slist.h"
#include "tagdef.h"
#include "ui.h"
#include "uirating.h"

enum {
  CONFUI_STARS_10,
  CONFUI_STARS_20,
  CONFUI_STARS_30,
  CONFUI_STARS_40,
  CONFUI_STARS_50,
  CONFUI_STARS_60,
  CONFUI_STARS_70,
  CONFUI_STARS_80,
  CONFUI_STARS_90,
  CONFUI_STARS_100,
  CONFUI_STARS_MAX,
};

typedef struct confitunes {
  datafile_t    *starsdf;
  nlist_t       *stars;
  datafile_t    *fieldsdf;
  nlist_t       *fields;
  uirating_t    *uirating [CONFUI_STARS_MAX];
} confitunes_t;

/* must be sorted in ascii order */
static datafilekey_t starsdfkeys [CONFUI_STARS_MAX] = {
  { "10",   CONFUI_STARS_10,      VALUE_NUM,  ratingConv, -1 },
  { "100",  CONFUI_STARS_100,     VALUE_NUM,  ratingConv, -1 },
  { "20",   CONFUI_STARS_20,      VALUE_NUM,  ratingConv, -1 },
  { "30",   CONFUI_STARS_30,      VALUE_NUM,  ratingConv, -1 },
  { "40",   CONFUI_STARS_40,      VALUE_NUM,  ratingConv, -1 },
  { "50",   CONFUI_STARS_50,      VALUE_NUM,  ratingConv, -1 },
  { "60",   CONFUI_STARS_60,      VALUE_NUM,  ratingConv, -1 },
  { "70",   CONFUI_STARS_70,      VALUE_NUM,  ratingConv, -1 },
  { "80",   CONFUI_STARS_80,      VALUE_NUM,  ratingConv, -1 },
  { "90",   CONFUI_STARS_90,      VALUE_NUM,  ratingConv, -1 },
};

static bool confuiSelectiTunesDir (void *udata);
static bool confuiSelectiTunesFile (void *udata);
static int  confuiValidateMediaDir (uientry_t *entry, void *udata);

void
confuiInitiTunes (confuigui_t *gui)
{
  char        tbuff [MAXPATHLEN];
  slist_t     *tlist;
  slistidx_t  iteridx;
  char        *key;

  gui->itunes = malloc (sizeof (confitunes_t));

  for (int i = 0; i < CONFUI_STARS_MAX; ++i) {
    gui->itunes->uirating [i] = NULL;
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_STARS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  gui->itunes->starsdf = datafileAllocParse ("itunes-stars",
      DFTYPE_KEY_VAL, tbuff, starsdfkeys, CONFUI_STARS_MAX);
  gui->itunes->stars = datafileGetList (gui->itunes->starsdf);

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  gui->itunes->fieldsdf = datafileAllocParse ("itunes-fields",
      DFTYPE_LIST, tbuff, NULL, 0);
  tlist = datafileGetList (gui->itunes->fieldsdf);

  gui->itunes->fields = nlistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  nlistSetSize (gui->itunes->fields, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((key = slistIterateKey (tlist, &iteridx)) != NULL) {
    int     val;

    val = tagdefLookup (key);
    nlistSetNum (gui->itunes->fields, val, 1);
  }
}

void
confuiCleaniTunes (confuigui_t *gui)
{
  for (int i = 0; i < CONFUI_STARS_MAX; ++i) {
    uiratingFree (gui->itunes->uirating [i]);
    gui->itunes->uirating [i] = NULL;
  }
  datafileFree (gui->itunes->starsdf);
  datafileFree (gui->itunes->fieldsdf);
  nlistFree (gui->itunes->fields);
  dataFree (gui->itunes);
}


void
confuiSaveiTunes (confuigui_t *gui)
{
  bool    changed;
  int     count;
  char    tbuff [MAXPATHLEN];

  changed = false;
  for (int i = 0; i < CONFUI_STARS_MAX; ++i) {
    int     tval, oval;

    tval = uiratingGetValue (gui->itunes->uirating [i]);
    oval = nlistGetNum (gui->itunes->stars, CONFUI_STARS_10 + i);
    if (tval != oval) {
      changed = true;
      nlistSetNum (gui->itunes->stars, CONFUI_STARS_10 + i, tval);
    }
  }

  if (changed) {
    pathbldMakePath (tbuff, sizeof (tbuff),
        ITUNES_STARS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
    datafileSaveKeyVal ("itunes-stars", tbuff, starsdfkeys,
        CONFUI_STARS_MAX, gui->itunes->stars);
  }

  changed = false;
  count = 0;
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    bool   tval;
    bool   oval;

    if (tagdefs [i].itunesName == NULL) {
      continue;
    }
    if (i == TAG_FILE || i == TAG_DURATION) {
      continue;
    }

    oval = false;
    if (nlistGetNum (gui->itunes->fields, i) >= 0) {
      oval = true;
    }
    tval = uiToggleButtonIsActive (
        &gui->uiitem [CONFUI_WIDGET_ITUNES_FIELD_1 + count].uiwidget);
    if (oval != tval) {
      changed = true;
      nlistSetNum (gui->itunes->fields, i, tval);
    }
    ++count;
  }

  if (changed) {
    int         key;
    int         tval;
    nlistidx_t  iteridx;
    slist_t     *nlist;

    nlist = slistAlloc ("itunes-fields", LIST_ORDERED, NULL);
    nlistStartIterator (gui->itunes->fields, &iteridx);
    while ((key = nlistIterateKey (gui->itunes->fields, &iteridx)) >= 0) {
      tval = nlistGetNum (gui->itunes->fields, key);
      if (tval > 0) {
        slistSetNum (nlist, tagdefs [key].tag, 0);
      }
    }

    pathbldMakePath (tbuff, sizeof (tbuff),
        ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
    datafileSaveList ("itunes-fields", tbuff, nlist);
  }
}

void
confuiBuildUIiTunes (confuigui_t *gui)
{
  char          tmp [200];
  char          tbuff [MAXPATHLEN];
  char          *tdata;
  UIWidget      mvbox;
  UIWidget      vbox;
  UIWidget      vboxb;
  UIWidget      mhbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *vboxp;
  UIWidget      sg;
  UIWidget      sgr;
  int           count;

  logProcBegin (LOG_PROC, "confuiBuildUIiTunes");
  uiCreateVertBox (&mvbox);

  /* filter display */
  confuiMakeNotebookTab (&mvbox, gui, ITUNES_NAME, CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgr);

  *tbuff = '\0';
  tdata = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (tdata != NULL) {
    strlcpy (tbuff, tdata, sizeof (tbuff));
  }

  /* CONTEXT: configuration: itunes: the itunes media folder */
  snprintf (tmp, sizeof (tmp), _("%s Media Folder"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, &mvbox, &sg, tmp,
      CONFUI_ENTRY_ITUNES_DIR, OPT_M_DIR_ITUNES_MEDIA,
      tbuff, confuiSelectiTunesDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_ITUNES_DIR].entry,
      confuiValidateMediaDir, gui, UIENTRY_DELAYED);

  *tbuff = '\0';
  tdata = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  if (tdata != NULL) {
    strlcpy (tbuff, tdata, sizeof (tbuff));
  }

  /* CONTEXT: configuration: itunes: the itunes xml file */
  snprintf (tmp, sizeof (tmp), _("%s XML File"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, &mvbox, &sg, tmp,
      CONFUI_ENTRY_ITUNES_XML, OPT_M_ITUNES_XML_FILE,
      tbuff, confuiSelectiTunesFile);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_ITUNES_XML].entry,
      uiEntryValidateFile, gui, UIENTRY_DELAYED);

  uiCreateHorizBox (&mhbox);
  uiBoxPackStart (&mvbox, &mhbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackStart (&mhbox, &vbox);

  /* CONTEXT: configuration: itunes: label for itunes rating conversion */
  uiCreateLabel (&uiwidget, _("Rating"));
  uiBoxPackStart (&vbox, &uiwidget);

  /* itunes uses 10..100 mapping to 0.5,1,1.5,...,4.5,5 stars */
  for (int i = 0; i < CONFUI_STARS_MAX; ++i) {
    double  starval;

    starval = (double) i / 2.0 + 0.5;
    if (i % 2 == 1) {
      snprintf (tmp, sizeof (tmp), "%0.0f", starval);
    } else {
      snprintf (tmp, sizeof (tmp), "%0.1f", starval);
    }
    /* CONTEXT: configuration: itunes rating in stars (0.5, 1, 1.5, etc.) */
    snprintf (tbuff, sizeof (tbuff), _("%s Stars"), tmp);

    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    uiCreateLabel (&uiwidget, tbuff);
    uiSizeGroupAdd (&sgr, &uiwidget);
    uiBoxPackStart (&hbox, &uiwidget);

    gui->itunes->uirating [i] = uiratingSpinboxCreate (&hbox, false);

    uiratingSetValue (gui->itunes->uirating [i],
        nlistGetNum (gui->itunes->stars, CONFUI_STARS_10 + i));
  }

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackStart (&mhbox, &vbox);

  /* CONTEXT: configuration: itunes: which fields should be imported from itunes */
  uiCreateLabel (&uiwidget, _("Fields to Import"));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateVertBox (&vboxb);
  uiWidgetSetAllMargins (&vboxb, uiBaseMarginSz * 2);
  uiBoxPackStart (&hbox, &vboxb);

  count = 0;
  vboxp = &vbox;
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    bool   tval;

    if (tagdefs [i].itunesName == NULL) {
      continue;
    }
    if (i == TAG_FILE || i == TAG_DURATION) {
      continue;
    }

    tval = false;
    if (nlistGetNum (gui->itunes->fields, i) >= 0) {
      tval = true;
    }
    confuiMakeItemCheckButton (gui, vboxp, &sg, tagdefs [i].displayname,
        CONFUI_WIDGET_ITUNES_FIELD_1 + count, -1, tval);
    ++count;
    if (count > TAG_ITUNES_MAX / 2) {
      vboxp = &vboxb;
    }
    if (count >= TAG_ITUNES_MAX) {
      break;
    }
  }

  logProcEnd (LOG_PROC, "confuiBuildUIiTunes", "");
}

/* internal routines */

static bool
confuiSelectiTunesDir (void *udata)
{
  confuigui_t *gui = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiSelectiTunesDir");
  /* CONTEXT: configuration: itunes media folder selection dialog: window title */
  snprintf (tbuff, sizeof (tbuff), _("Select %s Media Location"), ITUNES_NAME);
  selectdata = uiDialogCreateSelect (&gui->window, tbuff,
      bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_ITUNES_DIR].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  free (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectiTunesDir", "");
  return UICB_CONT;
}

static bool
confuiSelectiTunesFile (void *udata)
{
  confuigui_t *gui = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiSelectiTunesFile");
  /* CONTEXT: configuration: itunes xml file selection dialog: window title */
  snprintf (tbuff, sizeof (tbuff), _("Select %s XML File"), ITUNES_NAME);
  selectdata = uiDialogCreateSelect (&gui->window, tbuff,
      /* CONTEXT: configuration: dialog: XML file types */
      bdjoptGetStr (OPT_M_ITUNES_XML_FILE), NULL, _("XML Files"), "application/xml");
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_ITUNES_XML].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  free (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectiTunesFile", "");
  return UICB_CONT;
}

static int
confuiValidateMediaDir (uientry_t *entry, void *udata)
{
  confuigui_t *gui = udata;
  const char  *sval;
  char        tbuff [MAXPATHLEN];
  pathinfo_t  *pi;

  logProcBegin (LOG_PROC, "confuiValidateMediaDir");
  sval = uiEntryGetValue (entry);
  if (! fileopIsDirectory (sval)) {
    return UIENTRY_ERROR;
  }

  pi = pathInfo (sval);
  snprintf (tbuff, sizeof (tbuff), "%.*s/%s",
      (int) pi->dlen, pi->dirname, ITUNES_XML_NAME);
  if (fileopFileExists (tbuff)) {
    sval = uiEntryGetValue (gui->uiitem [CONFUI_ENTRY_ITUNES_XML].entry);
    if (sval == NULL || ! *sval) {
      uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_ITUNES_XML].entry, tbuff);
    }
  }
  pathInfoFree (pi);
  logProcEnd (LOG_PROC, "confuiValidateMediaDir", "");
  return UIENTRY_OK;
}

