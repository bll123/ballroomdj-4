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

static bool confuiPlayerQueueActiveChg (void *udata);
static bool confuiPlayerQueueChg (void *udata);
static void confuiPlayerQueueUpdateState (confuigui_t *gui, int idx);
static void confuiSetPlayerQueueList (confuigui_t *gui);
static void confuiUpdatePlayerQueueList (confuigui_t *gui);

void
confuiInitPlayerQueue (confuigui_t *gui)
{
  gui->uiitem [CONFUI_SPINBOX_PLAYER_QUEUE].displist = NULL;

  confuiSetPlayerQueueList (gui);
}

void
confuiBuildUIPlayerQueue (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      sg;
  UIWidget      sgB;

  logProcBegin (LOG_PROC, "confuiBuildUIPlayerQueue");
  gui->inbuild = true;
  uiCreateVertBox (&vbox);

  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: player queue configuration*/
      _("Player Queues"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);

  /* CONTEXT: (noun) configuration: queue: select which queue to configure */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Queue"),
      CONFUI_SPINBOX_PLAYER_QUEUE, -1, CONFUI_OUT_NUM,
      gui->uiitem [CONFUI_SPINBOX_PLAYER_QUEUE].listidx, confuiPlayerQueueChg);

  /* CONTEXT: (noun) configuration: queue: the name of the music queue */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Queue Name"),
      CONFUI_ENTRY_QUEUE_NM, OPT_Q_QUEUE_NAME,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether the queue is active */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Active"),
      CONFUI_SWITCH_Q_ACTIVE, OPT_Q_ACTIVE,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, 0),
      confuiPlayerQueueActiveChg, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to display the queue */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Display"),
      CONFUI_SWITCH_Q_DISPLAY, OPT_Q_DISPLAY,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Fade In Time"),
      CONFUI_WIDGET_Q_FADE_IN_TIME, OPT_Q_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Fade Out Time"),
      CONFUI_WIDGET_Q_FADE_OUT_TIME, OPT_Q_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (gui, &vbox, &sg, &sgB, _("Gap Between Songs"),
      CONFUI_WIDGET_Q_GAP, OPT_Q_GAP,
      0.0, 60.0, (double) bdjoptGetNumPerQueue (OPT_Q_GAP, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (gui, &vbox, &sg, &sgB, _("Maximum Play Time"),
      CONFUI_SPINBOX_Q_MAX_PLAY_TIME, OPT_Q_MAXPLAYTIME,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the time when playback will stop */
  confuiMakeItemSpinboxTime (gui, &vbox, &sg, &sgB, _("Stop At"),
      CONFUI_SPINBOX_Q_STOP_AT_TIME, OPT_Q_STOP_AT_TIME,
      bdjoptGetNum (OPT_Q_STOP_AT_TIME), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: pause each song */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Pause Each Song"),
      CONFUI_SWITCH_Q_PAUSE_EACH_SONG, OPT_Q_PAUSE_EACH_SONG,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to play announcements */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Play Announcements"),
      CONFUI_SWITCH_Q_PLAY_ANNOUNCE, OPT_Q_PLAY_ANNOUNCE,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: play when queued */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Play When Queued"),
      CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED, OPT_Q_PLAY_WHEN_QUEUED,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to show the 'queue dance' buttons */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Show Queue Dance Buttons"),
      CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE, OPT_Q_SHOW_QUEUE_DANCE,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, 0), NULL, CONFUI_INDENT);

  gui->inbuild = false;
  confuiPlayerQueueChg (gui);   // calls active-chg

  logProcEnd (LOG_PROC, "confuiBuildUIPlayerQueue", "");
}

static bool
confuiPlayerQueueActiveChg (void *udata)
{
  confuigui_t *gui = udata;
  int         tval = 0;

  if (gui->inbuild) {
    return UICB_CONT;
  }
  if (gui->inchange) {
    return UICB_CONT;
  }

  tval = uiSwitchGetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch);
  if (tval) {
    uiWidgetEnable (&gui->uiitem [CONFUI_WIDGET_Q_FADE_IN_TIME].uiwidget);
    uiWidgetEnable (&gui->uiitem [CONFUI_WIDGET_Q_FADE_OUT_TIME].uiwidget);
    uiWidgetEnable (&gui->uiitem [CONFUI_WIDGET_Q_GAP].uiwidget);
    uiSpinboxEnable (gui->uiitem [CONFUI_SPINBOX_Q_MAX_PLAY_TIME].spinbox);
    uiSpinboxEnable (gui->uiitem [CONFUI_SPINBOX_Q_STOP_AT_TIME].spinbox);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiswitch);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiswitch);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED].uiswitch);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiswitch);
  } else {
    uiWidgetDisable (&gui->uiitem [CONFUI_WIDGET_Q_FADE_IN_TIME].uiwidget);
    uiWidgetDisable (&gui->uiitem [CONFUI_WIDGET_Q_FADE_OUT_TIME].uiwidget);
    uiWidgetDisable (&gui->uiitem [CONFUI_WIDGET_Q_GAP].uiwidget);
    uiSpinboxDisable (gui->uiitem [CONFUI_SPINBOX_Q_MAX_PLAY_TIME].spinbox);
    uiSpinboxDisable (gui->uiitem [CONFUI_SPINBOX_Q_STOP_AT_TIME].spinbox);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiswitch);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiswitch);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED].uiswitch);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiswitch);
  }

  /* if called from init or from queue-chg, this is incorrect */
  confuiPlayerQueueUpdateState (gui, 1);

  return UICB_CONT;
}

static bool
confuiPlayerQueueChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;
  int         widx;

  logProcBegin (LOG_PROC, "confuiPlayerQueueChg");

  if (gui->inbuild) {
    return UICB_CONT;
  }
  if (gui->inchange) {
    return UICB_CONT;
  }

  /* must prevent the active-chg handler from executing, as the */
  /* toggle switches do not display correctly otherwise */
  gui->inchange = true;

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

  /* set all of the display values for the queue specific items */
  uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_QUEUE_NM].entry,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, nselidx));
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_Q_FADE_IN_TIME].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_Q_FADE_OUT_TIME].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (&gui->uiitem [CONFUI_WIDGET_Q_GAP].uiwidget,
      (double) bdjoptGetNumPerQueue (OPT_Q_GAP, nselidx) / 1000.0);
  uiSpinboxTimeSetValue (gui->uiitem [CONFUI_SPINBOX_Q_MAX_PLAY_TIME].spinbox,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, nselidx));
  /* divide by to convert from hh:mm to mm:ss for the spinbox */
  uiSpinboxTimeSetValue (gui->uiitem [CONFUI_SPINBOX_Q_STOP_AT_TIME].spinbox,
      bdjoptGetNumPerQueue (OPT_Q_STOP_AT_TIME, nselidx) / 60);
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiswitch,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, nselidx));

  gui->inchange = false;

  confuiPlayerQueueActiveChg (gui);

  /* do this after the values have changed, otherwise the switches */
  /* may not display the insensitive/sensitive state correctly */
  confuiPlayerQueueUpdateState (gui, nselidx);

  logProcEnd (LOG_PROC, "confuiPlayerQueueChg", "");
  return UICB_CONT;
}

static void
confuiPlayerQueueUpdateState (confuigui_t *gui, int idx)
{
  if (idx == 0) {
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch);
    uiSwitchDisable (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch);
  } else {
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiswitch);
    uiSwitchEnable (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiswitch);
  }
}

static void
confuiSetPlayerQueueList (confuigui_t *gui)
{
  nlist_t     *tlist;
  int         widx;

  widx = CONFUI_SPINBOX_PLAYER_QUEUE;

  tlist = nlistAlloc ("queue-name", LIST_ORDERED, NULL);
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

