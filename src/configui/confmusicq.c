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
#include "bdjopt.h"
#include "configui.h"
#include "istring.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "pathbld.h"
#include "sysvars.h"
#include "ui.h"

static bool confuiMusicQActiveChg (void *udata);
static bool confuiMusicQDisplayChg (void *udata);
static bool confuiMusicQChg (void *udata);
static void confuiMusicQUpdateState (confuigui_t *gui, int idx);
static void confuiSetMusicQList (confuigui_t *gui);
static void confuiUpdateMusicQList (confuigui_t *gui);

void
confuiInitMusicQs (confuigui_t *gui)
{
  gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].displist = NULL;

  confuiSetMusicQList (gui);
}

void
confuiBuildUIMusicQs (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;
  uiwcont_t    *szgrpB;

  logProcBegin (LOG_PROC, "confuiBuildUIMusicQs");

  gui->inbuild = true;

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();

  vbox = uiCreateVertBox ();

  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: music queue configuration*/
      _("Music Queues"), CONFUI_ID_NONE);

  /* CONTEXT: (noun) configuration: queue: select which queue to configure */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Queue_noun"),
      CONFUI_SPINBOX_MUSIC_QUEUE, -1, CONFUI_OUT_NUM,
      gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].listidx, confuiMusicQChg);

  /* CONTEXT: (noun) configuration: queue: the name of the music queue */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Queue Name"),
      CONFUI_ENTRY_QUEUE_NM, OPT_Q_QUEUE_NAME,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether the queue is active */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Active"),
      CONFUI_SWITCH_Q_ACTIVE, OPT_Q_ACTIVE,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, 0),
      confuiMusicQActiveChg, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to display the queue */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Display"),
      CONFUI_SWITCH_Q_DISPLAY, OPT_Q_DISPLAY,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, 0),
      confuiMusicQDisplayChg, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (gui, vbox, szgrp, szgrpB, _("Fade In Time"),
      CONFUI_WIDGET_Q_FADE_IN_TIME, OPT_Q_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (gui, vbox, szgrp, szgrpB, _("Fade Out Time"),
      CONFUI_WIDGET_Q_FADE_OUT_TIME, OPT_Q_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (gui, vbox, szgrp, szgrpB, _("Gap Between Songs"),
      CONFUI_WIDGET_Q_GAP, OPT_Q_GAP,
      0.0, 60.0, (double) bdjoptGetNumPerQueue (OPT_Q_GAP, 0) / 1000.0,
      CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (gui, vbox, szgrp, szgrpB, _("Maximum Play Time"),
      CONFUI_SPINBOX_Q_MAX_PLAY_TIME, OPT_Q_MAXPLAYTIME,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, 0), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: the time when playback will stop */
  confuiMakeItemSpinboxTime (gui, vbox, szgrp, szgrpB, _("Stop At"),
      CONFUI_SPINBOX_Q_STOP_AT_TIME, OPT_Q_STOP_AT_TIME,
      bdjoptGetNum (OPT_Q_STOP_AT_TIME), CONFUI_INDENT);

  /* CONTEXT: configuration: queue: pause each song */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Pause Each Song"),
      CONFUI_SWITCH_Q_PAUSE_EACH_SONG, OPT_Q_PAUSE_EACH_SONG,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to play announcements */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Play Announcements"),
      CONFUI_SWITCH_Q_PLAY_ANNOUNCE, OPT_Q_PLAY_ANNOUNCE,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: play when queued (verb) */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Play When Queued"),
      CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED, OPT_Q_PLAY_WHEN_QUEUED,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, 0), NULL, CONFUI_INDENT);

  /* CONTEXT: configuration: queue: whether to show the extra queue buttons (queue dance, queue 5) */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Show Extra Queue Buttons"),
      CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE, OPT_Q_SHOW_QUEUE_DANCE,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, 0), NULL, CONFUI_INDENT);

  gui->inbuild = false;
  confuiMusicQChg (gui);   // calls active-chg

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);

  logProcEnd (LOG_PROC, "confuiBuildUIMusicQs", "");
}

static bool
confuiMusicQActiveChg (void *udata)
{
  confuigui_t *gui = udata;
  int         tval = 0;
  int         state;
  int         selidx;

  if (gui->inbuild) {
    return UICB_CONT;
  }
  if (gui->inchange) {
    return UICB_CONT;
  }

  tval = uiSwitchGetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiwidgetp);

  /* force a no-change for ui backends where enable/disable are buggy */
  selidx = uiSpinboxTextGetValue (gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].spinbox);
  if (selidx == 0 && tval == 0) {
    uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiwidgetp, 1);
    return UICB_CONT;
  }

  state = UIWIDGET_DISABLE;
  if (tval) {
    state = UIWIDGET_ENABLE;
  }

  uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_Q_FADE_IN_TIME].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_Q_FADE_OUT_TIME].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_Q_GAP].uiwidgetp, state);
  uiSpinboxSetState (gui->uiitem [CONFUI_SPINBOX_Q_MAX_PLAY_TIME].spinbox, state);
  uiSpinboxSetState (gui->uiitem [CONFUI_SPINBOX_Q_STOP_AT_TIME].spinbox, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiwidgetp, state);

  return UICB_CONT;
}

static bool
confuiMusicQDisplayChg (void *udata)
{
  confuigui_t *gui = udata;
  int         tval = 0;
  int         selidx;

  if (gui->inbuild) {
    return UICB_CONT;
  }
  if (gui->inchange) {
    return UICB_CONT;
  }

  tval = uiSwitchGetValue (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiwidgetp);

  /* force a no-change for ui backends where enable/disable are buggy */
  selidx = uiSpinboxTextGetValue (gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].spinbox);
  if (selidx == 0 && tval == 0) {
    uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiwidgetp, 1);
    return UICB_CONT;
  }

  return UICB_CONT;
}

static bool
confuiMusicQChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;
  int         widx;

  logProcBegin (LOG_PROC, "confuiMusicQChg");

  if (gui->inbuild) {
    return UICB_CONT;
  }
  if (gui->inchange) {
    return UICB_CONT;
  }

  /* must prevent the active-chg handler from executing, as the */
  /* toggle switches do not display correctly otherwise */
  gui->inchange = true;

  widx = CONFUI_SPINBOX_MUSIC_QUEUE;
  oselidx = gui->uiitem [widx].listidx;
  nselidx = uiSpinboxTextGetValue (gui->uiitem [widx].spinbox);
  if (oselidx != nselidx) {
    /* make sure the current selection gets saved to the options data */
    confuiPopulateOptions (gui);
    confuiSetMusicQList (gui);
    confuiUpdateMusicQList (gui);
    uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, nselidx);
  }
  gui->uiitem [widx].listidx = nselidx;

  /* set all of the display values for the queue specific items */
  uiEntrySetValue (gui->uiitem [CONFUI_ENTRY_QUEUE_NM].uiwidgetp,
      bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_ACTIVE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_DISPLAY, nselidx));
  uiSpinboxSetValue (gui->uiitem [CONFUI_WIDGET_Q_FADE_IN_TIME].uiwidgetp,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (gui->uiitem [CONFUI_WIDGET_Q_FADE_OUT_TIME].uiwidgetp,
      (double) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, nselidx) / 1000.0);
  uiSpinboxSetValue (gui->uiitem [CONFUI_WIDGET_Q_GAP].uiwidgetp,
      (double) bdjoptGetNumPerQueue (OPT_Q_GAP, nselidx) / 1000.0);
  uiSpinboxTimeSetValue (gui->uiitem [CONFUI_SPINBOX_Q_MAX_PLAY_TIME].spinbox,
      bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, nselidx));
  /* divide by to convert from hh:mm to mm:ss for the spinbox */
  uiSpinboxTimeSetValue (gui->uiitem [CONFUI_SPINBOX_Q_STOP_AT_TIME].spinbox,
      bdjoptGetNumPerQueue (OPT_Q_STOP_AT_TIME, nselidx) / 60);
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PAUSE_EACH_SONG].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PLAY_ANNOUNCE].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, nselidx));
  uiSwitchSetValue (gui->uiitem [CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE].uiwidgetp,
      bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, nselidx));

  gui->inchange = false;

  confuiMusicQActiveChg (gui);
  confuiMusicQUpdateState (gui, nselidx);

  logProcEnd (LOG_PROC, "confuiMusicQChg", "");
  return UICB_CONT;
}

static void
confuiMusicQUpdateState (confuigui_t *gui, int idx)
{
  int   state;

  if (strcmp (uiBackend (), "gtk3") == 0) {
    /* enabling and disabling these is buggy on gtk3 */
    /* skip it altogether */
    return;
  }

  state = UIWIDGET_ENABLE;
  if (idx == 0) {
    state = UIWIDGET_DISABLE;
  }

  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_ACTIVE].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SWITCH_Q_DISPLAY].uiwidgetp, state);
}

static void
confuiSetMusicQList (confuigui_t *gui)
{
  nlist_t     *tlist;
  int         widx;

  widx = CONFUI_SPINBOX_MUSIC_QUEUE;

  tlist = nlistAlloc ("queue-name", LIST_ORDERED, NULL);
  gui->uiitem [widx].listidx = 0;
  for (size_t i = 0; i < BDJ4_QUEUE_MAX; ++i) {
    nlistSetStr (tlist, i, bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, i));
  }
  nlistFree (gui->uiitem [widx].displist);
  gui->uiitem [widx].displist = tlist;
}

static void
confuiUpdateMusicQList (confuigui_t *gui)
{
  nlist_t     *tlist;
  int         widx;
  nlistidx_t  iteridx;
  nlistidx_t  key;
  const char  *val;
  size_t      maxWidth = 10;

  widx = CONFUI_SPINBOX_MUSIC_QUEUE;
  if (gui->uiitem [widx].spinbox == NULL) {
    return;
  }

  tlist = gui->uiitem [widx].displist;

  nlistStartIterator (tlist, &iteridx);
  while ((key = nlistIterateKey (tlist, &iteridx)) >= 0) {
    size_t      len;

    val = nlistGetStr (tlist, key);
    len = istrlen (val);
    maxWidth = len > maxWidth ? len : maxWidth;
  }

  uiSpinboxTextSet (gui->uiitem [widx].spinbox, 0,
      nlistGetCount (tlist), maxWidth, tlist, NULL, NULL);
}

