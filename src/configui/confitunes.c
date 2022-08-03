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
#include "log.h"
#include "rating.h"
#include "tagdef.h"
#include "ui.h"

static bool confuiSelectiTunesDir (void *udata);
static bool confuiSelectiTunesFile (void *udata);

void
confuiInitiTunes (confuigui_t *gui)
{
  rating_t    *rating;


  for (int i = 0; i < CONFUI_ITUNES_STARS_MAX; ++i) {
    confuiSpinboxTextInitDataNum (gui, "itunes-stars",
        CONFUI_SPINBOX_ITUNES_STARS_0 + i,
// ### Fix: use actual ratings
        0, _("Poor"),
        1, _("Good"),
        2, _("Great"),
        -1);
  }
}

void
confuiBuildUIiTunes (confuigui_t *gui)
{
  char          tmp [200];
  char          tbuff [MAXPATHLEN];
  char          *dir;
  UIWidget      mvbox;
  UIWidget      vbox;
  UIWidget      vboxb;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *vboxp;
  UIWidget      sg;
  int           count;

  logProcBegin (LOG_PROC, "confuiBuildUIiTunes");
  uiCreateVertBox (&mvbox);

  /* filter display */
  confuiMakeNotebookTab (&mvbox, gui, ITUNES_NAME, CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);

  *tbuff = '\0';
  dir = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (dir != NULL) {
    strlcpy (tbuff, dir, sizeof (tbuff));
  }

  /* CONTEXT: configuration: itunes: the itunes media folder */
  snprintf (tmp, sizeof (tmp), _("%s Media Folder"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, &mvbox, &sg, tmp,
      CONFUI_ENTRY_ITUNES_DIR, OPT_M_DIR_ITUNES_MEDIA,
      tbuff, confuiSelectiTunesDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_ITUNES_DIR].entry,
      uiEntryValidateDir, gui, UIENTRY_DELAYED);

  /* CONTEXT: configuration: itunes: the itunes xml file */
  snprintf (tmp, sizeof (tmp), _("%s XML File"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, &mvbox, &sg, tmp,
      CONFUI_ENTRY_ITUNES_XML, OPT_M_ITUNES_XML_FILE,
      tbuff, confuiSelectiTunesFile);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_ITUNES_XML].entry,
      uiEntryValidateFile, gui, UIENTRY_DELAYED);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&mvbox, &hbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateLabel (&uiwidget, _("Rating"));
  uiBoxPackStart (&vbox, &uiwidget);

  /* the loop traverses from [0..10] */
  /* itunes uses 0..100 mapping to 0,0.5,1,1.5,...,4.5,5 stars */
  for (int i = 0; i < CONFUI_ITUNES_STARS_MAX; ++i) {
    double  starval;

    starval = (double) i / 2.0;
    if (i % 2 == 0) {
      snprintf (tmp, sizeof (tmp), "%0.0f", starval);
    } else {
      snprintf (tmp, sizeof (tmp), "%0.1f", starval);
    }
    /* CONTEXT: configuration: itunes rating in stars (0, 0.5, 1, 1.5, etc.) */
    snprintf (tbuff, sizeof (tbuff), _("%s Stars"), tmp);

    /* CONTEXT: configuration: itunes rating */
    confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, tbuff,
        CONFUI_SPINBOX_ITUNES_STARS_0 + i, -1,
// ### fix, need ratings list */
// ### fix, need ratings value from itunes-stars file */
        CONFUI_OUT_NUM, 1, NULL);
  }

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateVertBox (&vboxb);
  uiWidgetSetAllMargins (&vboxb, uiBaseMarginSz * 2);
  uiBoxPackStart (&hbox, &vboxb);

  count = 0;
  vboxp = &vbox;
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].itunesName == NULL) {
      continue;
    }
    if (i == TAG_FILE || i == TAG_DURATION) {
      continue;
    }

// ### fix, set the value appropriately.
    confuiMakeItemCheckButton (gui, vboxp, &sg, tagdefs [i].displayname,
        CONFUI_WIDGET_ITUNES_FIELD_1 + count, -1, 0);
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
      bdjoptGetStr (OPT_M_ITUNES_XML_FILE), NULL, NULL, NULL);
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

