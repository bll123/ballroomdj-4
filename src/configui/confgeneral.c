/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "bdjstring.h"
#include "bdjopt.h"
#include "configui.h"
#include "localeutil.h"
#include "log.h"
#include "ilist.h"
#include "instutil.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "vsencdec.h"

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
  uiwcont_t     *sw;
  uiwcont_t     *vbox;
  uiwcont_t     *szgrp;
  const char    *tmp;
  char          tbuff [MAXPATHLEN];
  char          ebuff [MAXPATHLEN];

  logProcBegin ();
  sw = uiCreateScrolledWindow (200);
  uiWidgetExpandHoriz (sw);
  uiWidgetExpandVert (sw);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (sw, vbox);
  uiWidgetExpandHoriz (vbox);
  uiWidgetAlignHorizFill (vbox);

  szgrp = uiCreateSizeGroupHoriz ();

  /* general */
  confuiMakeNotebookTab (sw, gui,
      /* CONTEXT: configuration: general options that apply to everything */
      _("General"), CONFUI_ID_NONE);

  tmp = bdjoptGetStr (OPT_M_DIR_MUSIC);
  if (tmp == NULL) {
    instutilGetMusicDir (tbuff, sizeof (tbuff));
    bdjoptSetStr (OPT_M_DIR_MUSIC, tbuff);
  }

  stpecpy (tbuff, tbuff + sizeof (tbuff), bdjoptGetStr (OPT_M_DIR_MUSIC));
  pathDisplayPath (tbuff, sizeof (tbuff));

  /* CONTEXT: configuration: the music folder where the user stores their music */
  confuiMakeItemEntryChooser (gui, vbox, szgrp, _("Music Folder"),
      CONFUI_ENTRY_CHOOSE_MUSIC_DIR, OPT_M_DIR_MUSIC,
      tbuff, confuiSelectMusicDir);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_MUSIC_DIR].uiwidgetp,
      "", uiEntryValidateDir, NULL, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the name of this profile */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, OPT_P_PROFILENAME,
      bdjoptGetStr (OPT_P_PROFILENAME), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: the profile accent color (identifies profile) */
  confuiMakeItemColorButton (gui, vbox, szgrp, _("Profile Colour"),
      CONFUI_WIDGET_UI_PROFILE_COLOR, OPT_P_UI_PROFILE_COL,
      bdjoptGetStr (OPT_P_UI_PROFILE_COL));

  /* CONTEXT: configuration: Whether to display BPM as BPM or MPM */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("BPM"),
      CONFUI_SPINBOX_BPM, OPT_G_BPM,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_BPM), NULL);

  /* bdj4 */
  /* CONTEXT: configuration: the locale to use (e.g. English or Nederlands) */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Locale"),
      CONFUI_SPINBOX_LOCALE, -1,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_LOCALE].listidx, NULL);

  /* CONTEXT: configuration: the startup script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (gui, vbox, szgrp, _("Startup Script"),
      CONFUI_ENTRY_CHOOSE_STARTUP, OPT_M_STARTUP_SCRIPT,
      bdjoptGetStr (OPT_M_STARTUP_SCRIPT), confuiSelectStartup);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_STARTUP].uiwidgetp,
      "", uiEntryValidateFile, NULL, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the shutdown script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (gui, vbox, szgrp, _("Shutdown Script"),
      CONFUI_ENTRY_CHOOSE_SHUTDOWN, OPT_M_SHUTDOWN_SCRIPT,
      bdjoptGetStr (OPT_M_SHUTDOWN_SCRIPT), confuiSelectShutdown);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_SHUTDOWN].uiwidgetp,
      "", uiEntryValidateFile, NULL, UIENTRY_DELAYED);

  /* audio tag stuff */

  /* CONTEXT: configuration: which audio tags will be written to the audio file */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Write Audio File Tags"),
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS, OPT_G_WRITETAGS,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_WRITETAGS), NULL);

  /* CONTEXT: configuration: classical music: use work/movement */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Use Work and Movement"),
      CONFUI_SWITCH_USE_WORK_MOVEMENT, OPT_G_USE_WORK_MOVEMENT,
      bdjoptGetNum (OPT_G_USE_WORK_MOVEMENT), NULL, 0);

  *ebuff = '\0';
  tmp = bdjoptGetStr (OPT_G_ACRCLOUD_API_KEY);
  if (tmp != NULL) {
    stpecpy (ebuff, ebuff + sizeof (ebuff), tmp);
  }
  if (tmp != NULL && *tmp &&
      strncmp (tmp, VSEC_E_PFX, strlen (VSEC_E_PFX)) == 0) {
    vsencdec (tmp, ebuff, sizeof (ebuff));
  }

  /* CONTEXT: configuration: the ACRCloud API Key */
  snprintf (tbuff, sizeof (tbuff), _("%s API Key"), ACRCLOUD_NAME);
  confuiMakeItemEntryEncrypt (gui, vbox, szgrp, tbuff,
      CONFUI_ENTRY_ACRCLOUD_API_KEY, OPT_G_ACRCLOUD_API_KEY,
      ebuff, CONFUI_NO_INDENT);

  *ebuff = '\0';
  tmp = bdjoptGetStr (OPT_G_ACRCLOUD_API_SECRET);
  if (tmp != NULL) {
    stpecpy (ebuff, ebuff + sizeof (ebuff), tmp);
  }
  if (tmp != NULL && *tmp &&
      strncmp (tmp, VSEC_E_PFX, strlen (VSEC_E_PFX)) == 0) {
    vsencdec (tmp, ebuff, sizeof (ebuff));
  }
  /* CONTEXT: configuration: the ACRCloud API Secret Key */
  snprintf (tbuff, sizeof (tbuff), _("%s API Secret Key"), ACRCLOUD_NAME);
  confuiMakeItemEntryEncrypt (gui, vbox, szgrp, tbuff,
      CONFUI_ENTRY_ACRCLOUD_API_SECRET, OPT_G_ACRCLOUD_API_SECRET,
      ebuff, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: the ACRCloud API Host */
  snprintf (tbuff, sizeof (tbuff), _("%s API Host"), ACRCLOUD_NAME);
  confuiMakeItemEntry (gui, vbox, szgrp, tbuff,
      CONFUI_ENTRY_ACRCLOUD_API_HOST, OPT_G_ACRCLOUD_API_HOST,
      bdjoptGetStr (OPT_G_ACRCLOUD_API_HOST), CONFUI_NO_INDENT);

  uiwcontFree (sw);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}


/* internal routines */

static bool
confuiSelectMusicDir (void *udata)
{
  uisfcb_t    *sfcb = udata;

  logProcBegin ();
  confuiSelectDirDialog (sfcb, bdjoptGetStr (OPT_M_DIR_MUSIC),
      /* CONTEXT: configuration: folder selection dialog: window title */
      _("Select Music Folder Location"));
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiSelectStartup (void *udata)
{
  uisfcb_t    *sfcb = udata;

  logProcBegin ();
  confuiSelectFileDialog (sfcb, sysvarsGetStr (SV_BDJ4_DIR_SCRIPT), NULL, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiSelectShutdown (void *udata)
{
  uisfcb_t    *sfcb = udata;

  logProcBegin ();
  confuiSelectFileDialog (sfcb, sysvarsGetStr (SV_BDJ4_DIR_SCRIPT), NULL, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static void
confuiLoadLocaleList (confuigui_t *gui)
{
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  const char    *key;
  const char    *data;
  nlist_t       *llist;
  int           count;
  bool          found;
  int           engbidx = 0;
  int           shortidx = 0;

  logProcBegin ();

  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, NULL);

  list = localeGetDisplayList ();

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
  logProcEnd ("");
}

