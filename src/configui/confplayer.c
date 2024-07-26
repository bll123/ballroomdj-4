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
#include "controller.h"
#include "log.h"
#include "ilist.h"
#include "istring.h"
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
static void confuiLoadControllerIntfcList (confuigui_t *gui);
static void confuiPlayerAudioSinkChg (void *udata);
static void confuiPlayerLoadSinkList (confuigui_t *gui, const char *intfc);

void
confuiInitPlayer (confuigui_t *gui)
{
  char            *volintfc;

  confuiLoadPlayerIntfcList (gui);
  confuiLoadControllerIntfcList (gui);

  volintfc = volumeCheckInterface (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  bdjoptSetStr (OPT_M_VOLUME_INTFC, volintfc);
  confuiLoadVolIntfcList (gui);
  confuiPlayerLoadSinkList (gui, volintfc);

  dataFree (volintfc);
}

void
confuiBuildUIPlayer (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;
  uiwcont_t    *szgrpB;

  logProcBegin ();
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
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx,
      confuiPlayerAudioSinkChg);

  /* CONTEXT: configuration: which audio sink (output) to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_MP_AUDIOSINK,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx, NULL);

  /* CONTEXT: configuration: which controller to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Controller"),
      CONFUI_SPINBOX_CONTROLLER, OPT_M_CONTROLLER_INTFC,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_CONTROLLER].listidx, NULL);

  /* CONTEXT: configuration: the volume used when starting the player */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, szgrpB, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      1, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME), NULL);

  /* CONTEXT: (noun) configuration: the number of items loaded into the music queue */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, szgrpB, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN), NULL);

  /* CONTEXT: configuration: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: whether or not to show the speed reset button */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Show Speed Reset"),
      CONFUI_SWITCH_SHOW_SPD_CONTROL, OPT_P_SHOW_SPD_CONTROL,
      bdjoptGetNum (OPT_P_SHOW_SPD_CONTROL), NULL, 0);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);

  logProcEnd ("");
}

static void
confuiLoadVolIntfcList (confuigui_t *gui)
{
  ilist_t     *interfaces;

  /* the volumeInterfaceList() call will choose a proper default */
  /* if there is no configuration file */
  interfaces = volumeInterfaceList ();
  confuiLoadIntfcList (gui, interfaces, OPT_M_VOLUME_INTFC,
      CONFUI_OPT_NONE, CONFUI_SPINBOX_VOL_INTFC, 0);
  ilistFree (interfaces);
}

static void
confuiLoadPlayerIntfcList (confuigui_t *gui)
{
  ilist_t     *interfaces;

  interfaces = pliInterfaceList ();
  confuiLoadIntfcList (gui, interfaces, OPT_M_PLAYER_INTFC,
      OPT_M_PLAYER_INTFC_NM, CONFUI_SPINBOX_PLI, 0);
  ilistFree (interfaces);
}

static void
confuiPlayerAudioSinkChg (void *udata)
{
  confuigui_t *gui = udata;
  int         nval;
  const char  *sval;
  nlist_t     *tlist;
  nlist_t     *keylist;
  size_t      maxWidth = 10;
  int         widx;

  if (gui == NULL) {
    return;
  }

  widx = CONFUI_SPINBOX_VOL_INTFC;
  if (gui->uiitem [widx].uiwidgetp == NULL) {
    return;
  }

  nval = uiSpinboxTextGetValue (gui->uiitem [widx].uiwidgetp);
  sval = nlistGetStr (gui->uiitem [widx].sbkeylist, nval);
  bdjoptSetStr (OPT_M_VOLUME_INTFC, sval);
  confuiPlayerLoadSinkList (gui, sval);

  widx = CONFUI_SPINBOX_AUDIO_OUTPUT;
  if (gui->uiitem [widx].uiwidgetp == NULL) {
    return;
  }

  tlist = gui->uiitem [widx].displist;
  keylist = gui->uiitem [widx].sbkeylist;

  nlistCalcMaxValueWidth (tlist);
  maxWidth = nlistGetMaxValueWidth (tlist);

  uiSpinboxTextSet (gui->uiitem [widx].uiwidgetp, 0,
      nlistGetCount (tlist), maxWidth, tlist, keylist, NULL);
}

static void
confuiPlayerLoadSinkList (confuigui_t *gui, const char *intfc)
{
  volsinklist_t   sinklist;
  volume_t        *volume = NULL;
  nlist_t         *tlist = NULL;
  nlist_t         *llist = NULL;
  const char      *audiosink;

  volume = volumeInit (intfc);

  volumeInitSinkList (&sinklist);
  volumeGetSinkList (volume, "", &sinklist);
  if (! volumeHaveSinkList (volume)) {
    pli_t     *pli;

    pli = pliInit (bdjoptGetStr (OPT_M_PLAYER_INTFC),
        bdjoptGetStr (OPT_M_PLAYER_INTFC_NM));
    pliAudioDeviceList (pli, &sinklist);
    pliFree (pli);
  }

  tlist = nlistAlloc ("cu-audio-out", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, NULL);
  /* CONTEXT: configuration: audio: The default audio sink (audio output) */
  nlistSetStr (tlist, 0, _("Default"));
  nlistSetStr (llist, 0, VOL_DEFAULT_NAME);
  gui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = 0;
  audiosink = bdjoptGetStr (OPT_MP_AUDIOSINK);
  for (int i = 0; i < sinklist.count; ++i) {
    if (audiosink != NULL &&
        strcmp (sinklist.sinklist [i].name, audiosink) == 0) {
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

static void
confuiLoadControllerIntfcList (confuigui_t *gui)
{
  ilist_t     *interfaces;

  interfaces = controllerInterfaceList ();
  confuiLoadIntfcList (gui, interfaces, OPT_M_CONTROLLER_INTFC,
      CONFUI_OPT_NONE, CONFUI_SPINBOX_CONTROLLER, 1);
  ilistFree (interfaces);

  nlistSetStr (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].displist, 0,
      /* CONTEXT: configuration: controller: no controller */
      _("None"));
  nlistSetStr (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].sbkeylist, 0, "");

#if 1
{
nlistidx_t iter;
nlistidx_t key;
  nlistStartIterator (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].displist, &iter);
  while ((key = nlistIterateKey (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].displist, &iter)) != LIST_LOC_INVALID) {
    fprintf (stderr, "key: %d\n", key);
    fprintf (stderr, "  disp: %s\n", nlistGetStr (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].displist, key));
    fprintf (stderr, "  sbkey: %s\n", nlistGetStr (gui->uiitem [CONFUI_SPINBOX_CONTROLLER].sbkeylist, key));
  }
}
#endif
}
