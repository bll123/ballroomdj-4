/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
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

void
confuiInitPlayer (confuigui_t *gui)
{
  volsinklist_t   sinklist;
  volume_t        *volume = NULL;
  nlist_t         *tlist = NULL;
  nlist_t         *llist = NULL;
  char            *volintfc;

  volintfc = volumeCheckInterface (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  bdjoptSetStr (OPT_M_VOLUME_INTFC, volintfc);
  mdfree (volintfc);

  confuiLoadVolIntfcList (gui);
  confuiLoadPlayerIntfcList (gui);

  volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  volumeSinklistInit (&sinklist);
  volumeGetSinkList (volume, "", &sinklist);
  if (! volumeHaveSinkList (volume)) {
    pli_t     *pli;

    pli = pliInit (bdjoptGetStr (OPT_M_VOLUME_INTFC), "default");
    pliAudioDeviceList (pli, &sinklist);
  }

  tlist = nlistAlloc ("cu-audio-out", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, NULL);
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
}

void
confuiBuildUIPlayer (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;
  uiwcont_t    *szgrpB;

  logProcBegin (LOG_PROC, "confuiBuildUIPlayer");
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();

  /* player */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with the player */
      _("Player"), CONFUI_ID_NONE);

  /* CONTEXT: configuration: which player interface to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Player"),
      CONFUI_SPINBOX_PLI, OPT_M_PLAYER_INTFC,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_PLI].listidx, NULL);

  /* CONTEXT: configuration: which audio interface to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Audio"),
      CONFUI_SPINBOX_VOL_INTFC, OPT_M_VOLUME_INTFC,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx, NULL);

  /* CONTEXT: configuration: which audio sink (output) to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_MP_AUDIOSINK,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx, NULL);

  /* CONTEXT: configuration: the volume used when starting the player */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, szgrpB, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME), NULL);

  /* CONTEXT: (noun) configuration: the number of items loaded into the music queue */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, szgrpB, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN), NULL);

  /* CONTEXT: configuration: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG), CONFUI_NO_INDENT);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);

  logProcEnd (LOG_PROC, "confuiBuildUIPlayer", "");
}

static void
confuiLoadVolIntfcList (confuigui_t *gui)
{
  slist_t     *interfaces;

  interfaces = volumeInterfaceList ();
  confuiLoadIntfcList (gui, interfaces, OPT_M_VOLUME_INTFC, CONFUI_SPINBOX_VOL_INTFC);
}

static void
confuiLoadPlayerIntfcList (confuigui_t *gui)
{
  slist_t     *interfaces;

  interfaces = pliInterfaceList ();
  confuiLoadIntfcList (gui, interfaces, OPT_M_PLAYER_INTFC, CONFUI_SPINBOX_PLI);
}

