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
#include "bdjopt.h"
#include "configui.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "pathbld.h"
#include "pli.h"
#include "sysvars.h"
#include "ui.h"
#include "volsink.h"
#include "volume.h"

static void confuiLoadVolIntfcList (confuigui_t *gui);
static void confuiLoadPlayerIntfcList (confuigui_t *gui);
static bool confuiPlayerQueueChg (void *udata);
static void confuiSetPlayerQueueList (confuigui_t *gui);
static void confuiUpdatePlayerQueueList (confuigui_t *gui);

void
confuiInitPlayer (confuigui_t *gui)
{
  volsinklist_t   sinklist;
  volume_t        *volume = NULL;
  nlist_t         *tlist = NULL;
  nlist_t         *llist = NULL;

  confuiLoadVolIntfcList (gui);
  confuiLoadPlayerIntfcList (gui);

  volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  assert (volume != NULL);
  volumeSinklistInit (&sinklist);
  volumeGetSinkList (volume, "", &sinklist);
  if (! volumeHaveSinkList (volume)) {
    pli_t     *pli;

    pli = pliInit (bdjoptGetStr (OPT_M_VOLUME_INTFC), "default");
    pliAudioDeviceList (pli, &sinklist);
  }

  gui->uiitem [CONFUI_SPINBOX_PLAYER_QUEUE].displist = NULL;

  tlist = nlistAlloc ("cu-audio-out", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, free);
  /* CONTEXT: configuration: audio: The default audio sink (audio output) */
  nlistSetStr (tlist, 0, _("Default"));
  nlistSetStr (llist, 0, "default");
  gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = 0;
  for (size_t i = 0; i < sinklist.count; ++i) {
    if (strcmp (sinklist.sinklist [i].name, bdjoptGetStr (OPT_MP_AUDIOSINK)) == 0) {
      gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = i + 1;
    }
    nlistSetStr (tlist, i + 1, sinklist.sinklist [i].description);
    nlistSetStr (llist, i + 1, sinklist.sinklist [i].name);
  }
  gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].sbkeylist = llist;

  volumeFreeSinkList (&sinklist);
  volumeFree (volume);

  confuiSetPlayerQueueList (gui);
}

void
confuiBuildUIPlayer (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      sg;
  UIWidget      sgB;

  logProcBegin (LOG_PROC, "confuiBuildUIPlayer");
  uiCreateVertBox (&vbox);

  /* player options */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: options associated with the player */
      _("Player Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);

  /* CONTEXT: configuration: which player interface to use */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Player"),
      CONFUI_SPINBOX_PLAYER, OPT_M_PLAYER_INTFC,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_PLAYER].listidx, NULL);

  /* CONTEXT: configuration: which audio interface to use */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Audio"),
      CONFUI_SPINBOX_VOL_INTFC, OPT_M_VOLUME_INTFC,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx, NULL);

  /* CONTEXT: configuration: which audio sink (output) to use */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_MP_AUDIOSINK,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx, NULL);

  /* CONTEXT: configuration: the volume used when starting the player */
  confuiMakeItemSpinboxNum (gui, &vbox, &sg, &sgB, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME), NULL);

  /* CONTEXT: configuration: the time when playback will stop */
  confuiMakeItemSpinboxTime (gui, &vbox, &sg, &sgB, _("Stop At"),
      CONFUI_SPINBOX_STOP_AT_TIME, OPT_P_STOPATTIME,
      bdjoptGetNum (OPT_P_STOPATTIME), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: the number of items loaded into the music queue */
  confuiMakeItemSpinboxNum (gui, &vbox, &sg, &sgB, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN), NULL);

  /* CONTEXT: configuration: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: which queue to configure */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Queue"),
      CONFUI_SPINBOX_PLAYER_QUEUE, -1, CONFUI_OUT_NUM,
      gui->uiitem [CONFUI_SPINBOX_PLAYER_QUEUE].listidx, confuiPlayerQueueChg);

  /* CONTEXT: configuration: The name of the music queue */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Queue Name"),
      CONFUI_ENTRY_QUEUE_NM, OPT_Q_QUEUE_NAME,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: whether the queue is active */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Active"),
      CONFUI_SWITCH_Q_ACTIVE, OPT_Q_ACTIVE,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: whether to display the queue */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Display"),
      CONFUI_SWITCH_Q_DISPLAY, OPT_Q_DISPLAY,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Fade In Time"),
      CONFUI_WIDGET_FADE_IN_TIME, OPT_Q_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Fade Out Time"),
      CONFUI_WIDGET_FADE_OUT_TIME, OPT_Q_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Gap Between Songs"),
      CONFUI_WIDGET_GAP, OPT_Q_GAP,
      0.0, 60.0, (double) bdjoptGetNumPerQueue (OPT_Q_GAP, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (gui, &vbox, &sg, &sgB, _("Maximum Play Time"),
      CONFUI_SPINBOX_MAX_PLAY_TIME, OPT_Q_MAXPLAYTIME,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: pause each song */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Pause Each Song"),
      CONFUI_SWITCH_Q_PAUSE_EACH_SONG, OPT_Q_PAUSE_EACH_SONG,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: whether to play announcements */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Play Announcements"),
      CONFUI_SWITCH_Q_PLAY_ANNOUNCE, OPT_Q_PLAY_ANNOUNCE,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: whether to show the 'queue dance' buttons */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Show Queue Dance Buttons"),
      CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE, OPT_Q_SHOW_QUEUE_DANCE,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, 0), NULL, CONFUI_INDENT);

  confuiPlayerQueueChg (gui);

  logProcEnd (LOG_PROC, "confuiBuildUIPlayer", "");
}

static void
confuiLoadVolIntfcList (confuigui_t *gui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  ilist_t       *list = NULL;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  char          *os;
  char          *intfc;
  char          *desc;
  nlist_t       *llist;
  int           count;

  static datafilekey_t dfkeys [CONFUI_VOL_MAX] = {
    { "DESC",   CONFUI_VOL_DESC,  VALUE_STR, NULL, -1 },
    { "INTFC",  CONFUI_VOL_INTFC, VALUE_STR, NULL, -1 },
    { "OS",     CONFUI_VOL_OS,    VALUE_STR, NULL, -1 },
  };

  logProcBegin (LOG_PROC, "confuiLoadVolIntfcList");

  tlist = nlistAlloc ("cu-volintfc-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-volintfc-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "volintfc", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  df = datafileAllocParse ("conf-volintfc-list", DFTYPE_INDIRECT, tbuff,
      dfkeys, CONFUI_VOL_MAX);
  list = datafileGetList (df);

  ilistStartIterator (list, &iteridx);
  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    intfc = ilistGetStr (list, key, CONFUI_VOL_INTFC);
    desc = ilistGetStr (list, key, CONFUI_VOL_DESC);
    os = ilistGetStr (list, key, CONFUI_VOL_OS);
    if (strcmp (os, sysvarsGetStr (SV_OSNAME)) == 0 ||
        strcmp (os, "all") == 0) {
      if (strcmp (intfc, bdjoptGetStr (OPT_M_VOLUME_INTFC)) == 0) {
        gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx = count;
      }
      nlistSetStr (tlist, count, desc);
      nlistSetStr (llist, count, intfc);
      ++count;
    }
  }
  datafileFree (df);

  gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadVolIntfcList", "");
}

static void
confuiLoadPlayerIntfcList (confuigui_t *gui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  ilist_t       *list = NULL;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  char          *os;
  char          *intfc;
  char          *desc;
  nlist_t       *llist;
  int           count;

  static datafilekey_t dfkeys [CONFUI_PLAYER_MAX] = {
    { "DESC",   CONFUI_PLAYER_DESC,  VALUE_STR, NULL, -1 },
    { "INTFC",  CONFUI_PLAYER_INTFC, VALUE_STR, NULL, -1 },
    { "OS",     CONFUI_PLAYER_OS,    VALUE_STR, NULL, -1 },
  };

  logProcBegin (LOG_PROC, "confuiLoadPlayerIntfcList");

  tlist = nlistAlloc ("cu-playerintfc-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-playerintfc-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "playerintfc", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  df = datafileAllocParse ("conf-playerintfc-list", DFTYPE_INDIRECT, tbuff,
      dfkeys, CONFUI_PLAYER_MAX);
  list = datafileGetList (df);

  ilistStartIterator (list, &iteridx);
  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    intfc = ilistGetStr (list, key, CONFUI_PLAYER_INTFC);
    desc = ilistGetStr (list, key, CONFUI_PLAYER_DESC);
    os = ilistGetStr (list, key, CONFUI_PLAYER_OS);
    if (strcmp (os, sysvarsGetStr (SV_OSNAME)) == 0 ||
        strcmp (os, "all") == 0) {
      if (strcmp (intfc, bdjoptGetStr (OPT_M_PLAYER_INTFC)) == 0) {
        gui->uiitem [CONFUI_SPINBOX_PLAYER].listidx = count;
      }
      nlistSetStr (tlist, count, desc);
      nlistSetStr (llist, count, intfc);
      ++count;
    }
  }
  datafileFree (df);

  gui->uiitem [CONFUI_SPINBOX_PLAYER].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_PLAYER].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadPlayerIntfcList", "");
}

static bool
confuiPlayerQueueChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;
  int         widx;

  logProcBegin (LOG_PROC, "confuiPlayerQueueChg");

  widx = CONFUI_SPINBOX_PLAYER_QUEUE;
  oselidx = gui->uiitem [widx].listidx;
  nselidx = uiSpinboxTextGetValue (gui->uiitem [widx].spinbox);
  if (oselidx != nselidx) {
    /* make sure the current selection gets saved to the options data */
    confuiPopulateOptions (gui);
    confuiSetPlayerQueueList (gui);
    confuiUpdatePlayerQueueList (gui);
    uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, nselidx);
  }
  gui->uiitem [widx].listidx = nselidx;

  if (nselidx == 0) {
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch);
  } else {
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch);
  }

  /* set all of the display values for the queue specific items */
  uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_QUEUE_NM].entry,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, nselidx));
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_FADE_IN_TIME].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_FADE_OUT_TIME].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_GAP].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_GAP, nselidx) / 1000.0);
  uiSpinboxTimeSetValue (gui->uiitem [CONFUI_SPINBOX_MAX_PLAY_TIME].spinbox,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, nselidx));

  logProcEnd (LOG_PROC, "confuiPlayerQueueChg", "");
  return UICB_CONT;
}

static void
confuiSetPlayerQueueList (confuigui_t *gui)
{
  nlist_t     *tlist;
  int         widx;

  widx = CONFUI_SPINBOX_PLAYER_QUEUE;

  tlist = nlistAlloc ("queue-name", LIST_ORDERED, free);
  gui->uiitem [widx].listidx = 0;
  for (size_t i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    nlistSetStr (tlist, i, bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, i));
  }
  nlistFree (gui->uiitem [widx].displist);
  gui->uiitem [widx].displist = tlist;
}

static void
confuiUpdatePlayerQueueList (confuigui_t *gui)
{
  nlist_t     *tlist;
  int         widx;
  nlistidx_t  iteridx;
  nlistidx_t  key;
  char        *val;
  size_t      maxWidth = 10;

  widx = CONFUI_SPINBOX_PLAYER_QUEUE;
  if (gui->uiitem [widx].spinbox == NULL) {
    return;
  }

  tlist = gui->uiitem [widx].displist;

  nlistStartIterator (tlist, &iteridx);
  while ((key = nlistIterateKey (tlist, &iteridx)) >= 0) {
    size_t      len;

    val = nlistGetStr (tlist, key);
    len = strlen (val);
    maxWidth = len > maxWidth ? len : maxWidth;
  }

  uiSpinboxTextSet (gui->uiitem [widx].spinbox, 0,
      nlistGetCount (tlist), maxWidth, tlist, NULL, NULL);
}
