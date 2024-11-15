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
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "itunes.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "configui.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathinfo.h"
#include "rating.h"
#include "slist.h"
#include "tagdef.h"
#include "ui.h"
#include "uirating.h"

enum {
  /* controls how many itunes fields are */
  /* placed in the first column in the configuration ui */
  ITUNES_FIELD_COL_MAX = 3,
  ITUNES_MAX_PER_COL = 13,
};

typedef struct confitunes {
  itunes_t      *itunes;
  uirating_t    *uirating [ITUNES_STARS_MAX];
} confitunes_t;

static bool confuiSelectiTunesDir (void *udata);
static bool confuiSelectiTunesFile (void *udata);
static int  confuiValidateMediaDir (uiwcont_t *entry, const char *label, void *udata);

void
confuiInitiTunes (confuigui_t *gui)
{
  gui->itunes = mdmalloc (sizeof (confitunes_t));

  for (int i = 0; i < ITUNES_STARS_MAX; ++i) {
    gui->itunes->uirating [i] = NULL;
  }

  gui->itunes->itunes = itunesAlloc ();
}

void
confuiCleaniTunes (confuigui_t *gui)
{
  for (int i = 0; i < ITUNES_STARS_MAX; ++i) {
    uiratingFree (gui->itunes->uirating [i]);
    gui->itunes->uirating [i] = NULL;
  }
  itunesFree (gui->itunes->itunes);
  dataFree (gui->itunes);
}


void
confuiSaveiTunes (confuigui_t *gui)
{
  bool        changed;
  int         count;
  const char  *itunesName;
  int         tagidx;

  changed = false;
  for (int i = 0; i < ITUNES_STARS_MAX; ++i) {
    int     tval, oval;

    tval = uiratingGetValue (gui->itunes->uirating [i]);
    oval = itunesGetStars (gui->itunes->itunes, ITUNES_STARS_10 + i);
    if (tval != oval) {
      changed = true;
      itunesSetStars (gui->itunes->itunes, ITUNES_STARS_10 + i, tval);
    }
  }

  if (changed) {
    itunesSaveStars (gui->itunes->itunes);
  }

  changed = false;
  count = 0;
  itunesStartIterateAvailFields (gui->itunes->itunes);
  while ((itunesName = itunesIterateAvailFields (gui->itunes->itunes, &tagidx)) != NULL) {
    bool   tval;
    bool   oval;

    if (tagidx == TAG_URI) {
      continue;
    }

    oval = false;
    if (itunesGetField (gui->itunes->itunes, tagidx) >= 0) {
      oval = true;
    }
    tval = uiToggleButtonIsActive (
        gui->uiitem [CONFUI_WIDGET_ITUNES_FIELD_1 + count].uiwidgetp);
    if (oval != tval) {
      changed = true;
      itunesSetField (gui->itunes->itunes, tagidx, tval);
    }
    ++count;
  }

  if (changed) {
    itunesSaveFields (gui->itunes->itunes);
  }
}

void
confuiBuildUIiTunes (confuigui_t *gui)
{
  char          tmp [200];
  char          tbuff [MAXPATHLEN];
  const char    *tdata;
  uiwcont_t     *mvbox = NULL;
  uiwcont_t     *vbox [3] = { NULL, NULL, NULL };
  uiwcont_t     *mhbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *vboxp = NULL;
  uiwcont_t     *szgrp;
  uiwcont_t     *szgrpr;
  int           count;
  const char    *itunesName;
  int           tagidx;
  int           cidx;

  logProcBegin ();
  mvbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpr = uiCreateSizeGroupHoriz ();

  /* filter display */
  confuiMakeNotebookTab (mvbox, gui, ITUNES_NAME, CONFUI_ID_NONE);

  *tbuff = '\0';
  tdata = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (tdata != NULL) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), tdata);
  }

  /* CONTEXT: configuration: itunes: the itunes media folder */
  snprintf (tmp, sizeof (tmp), _("%s Media Folder"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, mvbox, szgrp, tmp,
      CONFUI_ENTRY_CHOOSE_ITUNES_DIR, OPT_M_DIR_ITUNES_MEDIA,
      tbuff, confuiSelectiTunesDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_DIR].uiwidgetp,
      "", confuiValidateMediaDir, gui, UIENTRY_DELAYED);

  *tbuff = '\0';
  tdata = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  if (tdata != NULL) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), tdata);
  }

  /* CONTEXT: configuration: itunes: the itunes xml file */
  snprintf (tmp, sizeof (tmp), _("%s XML File"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, mvbox, szgrp, tmp,
      CONFUI_ENTRY_CHOOSE_ITUNES_XML, OPT_M_ITUNES_XML_FILE,
      tbuff, confuiSelectiTunesFile);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_XML].uiwidgetp,
      "", uiEntryValidateFile, NULL, UIENTRY_DELAYED);

  mhbox = uiCreateHorizBox ();
  uiBoxPackStart (mvbox, mhbox);

  vboxp = uiCreateVertBox ();
  uiBoxPackStart (mhbox, vboxp);
  uiWidgetSetAllMargins (vboxp, 2);

  /* CONTEXT: configuration: itunes: label for itunes rating conversion */
  uiwidgetp = uiCreateLabel (_("Rating"));
  uiBoxPackStart (vboxp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* itunes uses 10..100 mapping to 0.5,1,1.5,...,4.5,5 stars */
  for (int i = 0; i < ITUNES_STARS_MAX; ++i) {
    char    *p;
    char    *end = tbuff + sizeof (tbuff);

    *tbuff = '\0';
    p = tbuff;
    for (int j = 0; j < i; j += 2) {
      // black star
      p = stpecpy (p, end, "\xe2\x98\x85");
    }
    if (i % 2 == 0) {
      // star with left half black does not work on windows. "\xe2\xaf\xaa"
      // left half black star does not work on windows "\xe2\xaf\xa8"
      p = stpecpy (p, end, "\xc2\xbd");
    }

    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vboxp, hbox);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrpr, uiwidgetp);
    uiwcontFree (uiwidgetp);

    gui->itunes->uirating [i] = uiratingSpinboxCreate (hbox, UIRATING_NORM);

    uiratingSetValue (gui->itunes->uirating [i],
        itunesGetStars (gui->itunes->itunes, ITUNES_STARS_10 + i));
    uiwcontFree (hbox);
  }

  uiwcontFree (vboxp);

  vboxp = uiCreateVertBox ();
  uiBoxPackStart (mhbox, vboxp);
  uiWidgetSetAllMargins (vboxp, 2);

  /* CONTEXT: configuration: itunes: which fields should be imported from itunes */
  uiwidgetp = uiCreateLabel (_("Fields to Import"));
  uiBoxPackStart (vboxp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  uiwcontFree (vboxp);

  for (int i = 0; i < ITUNES_FIELD_COL_MAX; ++i) {
    vbox [i] = uiCreateVertBox ();
    uiBoxPackStart (hbox, vbox [i]);
    uiWidgetSetAllMargins (vbox [i], 2);
  }

  count = 0;
  cidx = 0;
  vboxp = vbox [cidx];
  itunesStartIterateAvailFields (gui->itunes->itunes);
  while ((itunesName = itunesIterateAvailFields (gui->itunes->itunes, &tagidx)) != NULL) {
    bool   tval;

    if (tagidx == TAG_URI) {
      continue;
    }

    tval = false;
    if (itunesGetField (gui->itunes->itunes, tagidx) >= 0) {
      tval = true;
    }
    confuiMakeItemCheckButton (gui, vboxp, szgrp, tagdefs [tagidx].displayname,
        CONFUI_WIDGET_ITUNES_FIELD_1 + count, -1, tval);
    ++count;
    if (count >= ITUNES_MAX_PER_COL) {
      ++cidx;
      vboxp = vbox [cidx];
      count = 0;
    }
  }

  uiwcontFree (hbox);
  for (int i = 0; i < ITUNES_FIELD_COL_MAX; ++i) {
    uiwcontFree (vbox [i]);
  }
  uiwcontFree (mvbox);
  uiwcontFree (mhbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpr);

  logProcEnd ("");
}

/* internal routines */

static bool
confuiSelectiTunesDir (void *udata)
{
  uisfcb_t    *sfcb = udata;
  char        tbuff [MAXPATHLEN];

  logProcBegin ();
  /* CONTEXT: configuration: itunes media folder selection dialog: window title */
  snprintf (tbuff, sizeof (tbuff), _("Select %s Media Location"), ITUNES_NAME);
  confuiSelectDirDialog (sfcb, bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA), tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiSelectiTunesFile (void *udata)
{
  uisfcb_t    *sfcb = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [MAXPATHLEN];
  char        dirbuff [MAXPATHLEN];
  pathinfo_t  *pi;

  logProcBegin ();
  /* CONTEXT: configuration: itunes xml file selection dialog: window title */
  snprintf (tbuff, sizeof (tbuff), _("Select %s XML File"), ITUNES_NAME);
  stpecpy (dirbuff, dirbuff + sizeof (dirbuff), bdjoptGetStr (OPT_M_ITUNES_XML_FILE));
  pi = pathInfo (dirbuff);
  dirbuff [pi->dlen] = '\0';
  pathInfoFree (pi);

  selectdata = uiSelectInit (sfcb->window, tbuff,
      dirbuff, NULL,
      /* CONTEXT: configuration: dialog: XML file types */
      _("XML Files"), "application/xml");
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (sfcb->entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  logProcEnd ("");
  return UICB_CONT;
}

static int
confuiValidateMediaDir (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t *gui = udata;
  const char  *sval;
  char        tbuff [MAXPATHLEN];
  pathinfo_t  *pi;

  logProcBegin ();
  sval = uiEntryGetValue (entry);
  if (! fileopIsDirectory (sval)) {
    return UIENTRY_ERROR;
  }

  pi = pathInfo (sval);
  snprintf (tbuff, sizeof (tbuff), "%.*s/%s",
      (int) pi->dlen, pi->dirname, ITUNES_XML_NAME);
  if (fileopFileExists (tbuff)) {
    sval = uiEntryGetValue (gui->uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_XML].uiwidgetp);
    if (sval == NULL || ! *sval) {
      uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_XML].uiwidgetp, tbuff);
    }
  }
  pathInfoFree (pi);
  logProcEnd ("");
  return UIENTRY_OK;
}

