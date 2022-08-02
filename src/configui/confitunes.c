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
#include "tagdef.h"
#include "ui.h"

static bool confuiSelectiTunesDir (void *udata);

void
confuiBuildUIiTunes (confuigui_t *gui)
{
  char          tmp [200];
  char          tbuff [MAXPATHLEN];
  char          *dir;
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIiTunes");
  uiCreateVertBox (&vbox);

  /* filter display */
  confuiMakeNotebookTab (&vbox, gui, ITUNES_NAME, CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);

  *tbuff = '\0';
  dir = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (dir != NULL) {
    strlcpy (tbuff, dir, sizeof (tbuff));
  }

  /* CONTEXT: configuration: itunes: the itunes media folder*/
  snprintf (tmp, sizeof (tmp), _("%s Media Folder"), ITUNES_NAME);
  confuiMakeItemEntryChooser (gui, &vbox, &sg, tmp,
      CONFUI_ENTRY_ITUNES_DIR, OPT_M_DIR_ITUNES_MEDIA,
      tbuff, confuiSelectiTunesDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_ITUNES_DIR].entry,
      uiEntryValidateDir, gui, UIENTRY_DELAYED);

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

