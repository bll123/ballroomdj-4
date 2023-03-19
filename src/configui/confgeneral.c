/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
#include "bdjstring.h"
#include "bdjopt.h"
#include "configui.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"
#include "ui.h"

static bool confuiSelectMusicDir (void *udata);
static bool confuiSelectStartup (void *udata);
static bool confuiSelectShutdown (void *udata);
static void confuiLoadLocaleList (confuigui_t *gui);

void
confuiInitGeneral (confuigui_t *gui)
{
  confuiLoadLocaleList (gui);

  confuiSpinboxTextInitDataNum (gui, "cu-audio-file-tags",
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
      /* CONTEXT: configuration: write audio file tags: do not write any tags to the audio file */
      WRITE_TAGS_NONE, _("Don't Write"),
      /* CONTEXT: configuration: write audio file tags: only write BDJ tags to the audio file */
      WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"),
      /* CONTEXT: configuration: write audio file tags: write all tags (BDJ and standard) to the audio file */
      WRITE_TAGS_ALL, _("All Tags"),
      -1);

  confuiSpinboxTextInitDataNum (gui, "cu-bpm",
      CONFUI_SPINBOX_BPM,
      /* CONTEXT: configuration: BPM: beats per minute (not bars per minute) */
      BPM_BPM, "BPM",
      /* CONTEXT: configuration: MPM: measures per minute (aka bars per minute) */
      BPM_MPM, "MPM",
      -1);
}

void
confuiBuildUIGeneral (confuigui_t *gui)
{
  uiwcont_t    vbox;
  uiwcont_t    sg;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiBuildUIGeneral");
  uiCreateVertBox (&vbox);

  /* general */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: general options that apply to everything */
      _("General"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  strlcpy (tbuff, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (tbuff));
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }

  /* CONTEXT: configuration: the music folder where the user store their music */
  confuiMakeItemEntryChooser (gui, &vbox, &sg, _("Music Folder"),
      CONFUI_ENTRY_CHOOSE_MUSIC_DIR, OPT_M_DIR_MUSIC,
      tbuff, confuiSelectMusicDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_MUSIC_DIR].entry,
      uiEntryValidateDir, gui, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the name of this profile */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, OPT_P_PROFILENAME,
      bdjoptGetStr (OPT_P_PROFILENAME), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: the profile accent color (identifies profile) */
  confuiMakeItemColorButton (gui, &vbox, &sg, _("Profile Colour"),
      CONFUI_WIDGET_UI_PROFILE_COLOR, OPT_P_UI_PROFILE_COL,
      bdjoptGetStr (OPT_P_UI_PROFILE_COL));

  /* CONTEXT: configuration: Whether to display BPM as BPM or MPM */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("BPM"),
      CONFUI_SPINBOX_BPM, OPT_G_BPM,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_BPM), NULL);

  /* database */

  /* CONTEXT: configuration: which audio tags will be written to the audio file */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Write Audio File Tags"),
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS, OPT_G_WRITETAGS,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_WRITETAGS), NULL);

  /* CONTEXT: configuration: write audio file tags in ballroomdj 3 compatibility mode */
  snprintf (tbuff, sizeof (tbuff), _("%s Compatible Audio File Tags"), BDJ3_NAME);
  confuiMakeItemSwitch (gui, &vbox, &sg, tbuff,
      CONFUI_SWITCH_BDJ3_COMPAT_TAGS, OPT_G_BDJ3_COMPAT_TAGS,
      bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS), NULL, CONFUI_NO_INDENT);

  /* bdj4 */
  /* CONTEXT: configuration: the locale to use (e.g. English or Nederlands) */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Locale"),
      CONFUI_SPINBOX_LOCALE, -1,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_LOCALE].listidx, NULL);

  /* CONTEXT: configuration: the startup script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (gui, &vbox, &sg, _("Startup Script"),
      CONFUI_ENTRY_CHOOSE_STARTUP, OPT_M_STARTUPSCRIPT,
      bdjoptGetStr (OPT_M_STARTUPSCRIPT), confuiSelectStartup);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_STARTUP].entry,
      uiEntryValidateFile, gui, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the shutdown script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (gui, &vbox, &sg, _("Shutdown Script"),
      CONFUI_ENTRY_CHOOSE_SHUTDOWN, OPT_M_SHUTDOWNSCRIPT,
      bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT), confuiSelectShutdown);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_SHUTDOWN].entry,
      uiEntryValidateFile, gui, UIENTRY_DELAYED);
  logProcEnd (LOG_PROC, "confuiBuildUIGeneral", "");
}


/* internal routines */

static bool
confuiSelectMusicDir (void *udata)
{
  confuigui_t *gui = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin (LOG_PROC, "confuiSelectMusicDir");
  selectdata = uiDialogCreateSelect (&gui->window,
      /* CONTEXT: configuration: folder selection dialog: window title */
      _("Select Music Folder Location"),
      bdjoptGetStr (OPT_M_DIR_MUSIC), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_CHOOSE_MUSIC_DIR].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    mdfree (fn);
  }
  mdfree (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectMusicDir", "");
  return UICB_CONT;
}

static bool
confuiSelectStartup (void *udata)
{
  confuigui_t *gui = udata;

  logProcBegin (LOG_PROC, "confuiSelectStartup");
  confuiSelectFileDialog (gui, CONFUI_ENTRY_CHOOSE_STARTUP,
      sysvarsGetStr (SV_BDJ4_DIR_SCRIPT), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectStartup", "");
  return UICB_CONT;
}

static bool
confuiSelectShutdown (void *udata)
{
  confuigui_t *gui = udata;

  logProcBegin (LOG_PROC, "confuiSelectShutdown");
  confuiSelectFileDialog (gui, CONFUI_ENTRY_CHOOSE_SHUTDOWN,
      sysvarsGetStr (SV_BDJ4_DIR_SCRIPT), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectShutdown", "");
  return UICB_CONT;
}

static void
confuiLoadLocaleList (confuigui_t *gui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  char          *key;
  char          *data;
  nlist_t       *llist;
  int           count;
  bool          found;
  int           engbidx = 0;
  int           shortidx = 0;

  logProcBegin (LOG_PROC, "confuiLoadLocaleList");

  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, NULL);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "locales", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_LOCALE);
  df = datafileAllocParse ("conf-locale-list", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  list = datafileGetList (df);

  slistStartIterator (list, &iteridx);
  count = 0;
  found = false;
  shortidx = -1;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (strcmp (data, "en_GB") == 0) {
      engbidx = count;
    }
    if (strcmp (data, sysvarsGetStr (SV_LOCALE)) == 0) {
      gui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = count;
      found = true;
    }
    if (strncmp (data, sysvarsGetStr (SV_LOCALE_SHORT), 2) == 0) {
      shortidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  if (! found && shortidx >= 0) {
    gui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = shortidx;
  } else if (! found) {
    gui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = engbidx;
  }
  datafileFree (df);

  gui->uiitem [CONFUI_SPINBOX_LOCALE].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_LOCALE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadLocaleList", "");
}


