/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
/*
 * pulse audio
 *
 * References:
 *   https://github.com/cdemoulins/pamixer/blob/master/pulseaudio.cc
 *   https://freedesktop.org/software/pulseaudio/doxygen/
 */

#include "config.h"

#if _hdr_pulse_pulseaudio

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include "bdjstring.h"
#include "tmutil.h"
#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

static void serverInfoCallback (pa_context *context, const pa_server_info *i, void *userdata);
static void sinkVolCallback (pa_context *context, const pa_sink_info *i, int eol, void *userdata);
static void connCallback (pa_context *pacontext, void* userdata);
static void nullCallback (pa_context* context, int success, void* userdata);
static void waitop (pa_operation *op);
static void pulse_close (void);
static void pulse_disconnect (void);
static void voliProcessFailure (char *name);
static void init_context (void);
static void getSinkCallback (pa_context *context, const pa_sink_info *i, int eol, void *userdata);

enum {
  STATE_OK = 0,
  STATE_WAIT = 1,
  STATE_FAIL = -1,
};

typedef struct {
  char      *defaultsink;
  bool      changed;
} pa_track_data_t;

typedef struct {
  pa_threaded_mainloop  *pamainloop;
  pa_context            *pacontext;
  pa_context_state_t    pastate;
  int                   state;
} state_t;

static state_t      gstate;
static int          ginit = 0;

typedef union {
  char            defname [500];
  pa_cvolume      *vol;
  volsinklist_t   *sinklist;
} pacallback_t;

const char *
voliDesc (void)
{
  return "Pulse Audio";
}

void
voliDisconnect (void)
{
  /*
   * If this program is made efficient by keeping a connection open
   * to pulseaudio, then pulseaudio will reset the audio track volume
   * when a new track starts.
   */
  pulse_disconnect ();
  pulse_close ();
}

void
voliCleanup (void **udata)
{
  if (*udata != NULL) {
    pa_track_data_t *trackdata = *udata;

    dataFree (trackdata->defaultsink);
    dataFree (trackdata);
    *udata = NULL;
  }
  return;
}

int
voliProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  pa_operation    *op = NULL;
  pacallback_t    cbdata;
  unsigned int    tvol;
  int             count;
  char            defsinkname [200];
  pa_track_data_t *trackdata = NULL;


  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  count = 0;
  while (count < 40) {
    int     rc;

    if (! ginit) {
      gstate.pacontext = NULL;
      gstate.pamainloop = pa_threaded_mainloop_new();
      mdextalloc (gstate.pamainloop);
      pa_threaded_mainloop_start (gstate.pamainloop);
      ginit = 1;
    }

    if (gstate.pacontext == NULL) {
      init_context ();
    }

    pa_threaded_mainloop_lock (gstate.pamainloop);
    while (gstate.state == STATE_WAIT) {
      pa_threaded_mainloop_wait (gstate.pamainloop);
    }
    pa_threaded_mainloop_unlock (gstate.pamainloop);

    if (gstate.pastate == PA_CONTEXT_READY) {
      break;
    }

    rc = pa_context_errno (gstate.pacontext);
    voliProcessFailure ("init-context");

    if (rc == PA_ERR_CONNECTIONREFUSED) {
      (void) ! system ("/usr/bin/pulseaudio -D");
      mssleep (600);
    }

    mssleep (100);
    ++count;
  }

  if (gstate.pastate != PA_CONTEXT_READY) {
    voliProcessFailure ("init context");
    return -1;
  }

  /* need the default sink name for all actions */
  *defsinkname = '\0';
  pa_threaded_mainloop_lock (gstate.pamainloop);
  op = pa_context_get_server_info (
      gstate.pacontext, &serverInfoCallback, &cbdata);
  mdextalloc (op);
  if (! op) {
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    voliProcessFailure ("serverinfo");
    return -1;
  }
  waitop (op);
  pa_threaded_mainloop_unlock (gstate.pamainloop);
  strlcpy (defsinkname, cbdata.defname, sizeof (defsinkname));

  if (*udata == NULL) {
    trackdata = mdmalloc (sizeof (pa_track_data_t));
    trackdata->defaultsink = mdstrdup (defsinkname);
    trackdata->changed = false;
    *udata = trackdata;
  } else {
    trackdata = *udata;
    if (strcmp (trackdata->defaultsink, defsinkname) != 0) {
      trackdata->changed = true;
    }
  }

  if (action == VOL_GETSINKLIST) {
    sinklist->defname = mdstrdup (defsinkname);
    sinklist->sinklist = NULL;
    sinklist->count = 0;
    cbdata.sinklist = sinklist;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_list (
        gstate.pacontext, &getSinkCallback, &cbdata);
    mdextalloc (op);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      voliProcessFailure ("getsink");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    return 0;
  }

  if (sinkname != NULL && *sinkname && action == VOL_SET_SYSTEM_DFLT) {
    /* future: like macos, change the system default to match the output sink */
    return 0;
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    /* getvolume or setvolume */
    pa_volume_t     avgvol;
    pa_cvolume      pavol;

    /* the volume should be associated with the supplied sink, */
    /* unless it was changed by the user, in which case, use the default */
    if (trackdata->changed || sinkname == NULL || ! *sinkname) {
      sinkname = defsinkname;
    }

    cbdata.vol = &pavol;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_by_name (
        gstate.pacontext, sinkname, &sinkVolCallback, &cbdata);
    mdextalloc (op);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      voliProcessFailure ("getsinkbyname");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);

    if (action == VOL_SET) {
      pa_cvolume  *nvol;

      tvol = (unsigned int) round ((double) *vol * (double) PA_VOLUME_NORM / 100.0);
      if (tvol > PA_VOLUME_MAX) {
        tvol = PA_VOLUME_MAX;
      }

      if (pavol.channels <= 0) {
        pavol.channels = 2;
        /* make sure this is set properly       */
        /* otherwise pa_cvolume_set will fail   */
      }
      nvol = pa_cvolume_set (&pavol, pavol.channels, tvol);

      pa_threaded_mainloop_lock (gstate.pamainloop);
      op = pa_context_set_sink_volume_by_name (
          gstate.pacontext, sinkname, nvol, nullCallback, NULL);
      mdextalloc (op);
      if (! op) {
        pa_threaded_mainloop_unlock (gstate.pamainloop);
        voliProcessFailure ("setvol");
        return -1;
      }
      waitop (op);
      pa_threaded_mainloop_unlock (gstate.pamainloop);
    }

    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_by_name (
        gstate.pacontext, sinkname, &sinkVolCallback, &cbdata);
    mdextalloc (op);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      voliProcessFailure ("getvol");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);

    avgvol = pa_cvolume_avg (&pavol);
    *vol = (int) round ((double) avgvol / (double) PA_VOLUME_NORM * 100.0);

    return *vol;
  }

  return 0;
}

static void
serverInfoCallback (
  pa_context            *context,
  const pa_server_info  *i,
  void                  *userdata)
{
  pacallback_t  *cbdata = (pacallback_t *) userdata;

  strlcpy (cbdata->defname, i->default_sink_name, sizeof (cbdata->defname));
  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}

static void
sinkVolCallback (
  pa_context *context,
  const pa_sink_info *i,
  int eol,
  void *userdata)
{
  pacallback_t  *cbdata = (pacallback_t *) userdata;

  if (eol != 0) {
    pa_threaded_mainloop_signal (gstate.pamainloop, 0);
    return;
  }
  memcpy (cbdata->vol, &(i->volume), sizeof (pa_cvolume));
  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}

static void
connCallback (pa_context *pacontext, void* userdata)
{
  state_t     *stdata = (state_t *) userdata;

  if (pacontext == NULL) {
    stdata->pastate = PA_CONTEXT_FAILED;
    stdata->state = STATE_FAIL;
    pa_threaded_mainloop_signal (stdata->pamainloop, 0);
    return;
  }

  stdata->pastate = pa_context_get_state (pacontext);
  switch (stdata->pastate)
  {
    case PA_CONTEXT_READY:
      stdata->state = STATE_OK;
      break;
    case PA_CONTEXT_FAILED:
      stdata->state = STATE_FAIL;
      break;
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_TERMINATED:
      stdata->state = STATE_WAIT;
      break;
  }
  pa_threaded_mainloop_signal (stdata->pamainloop, 0);
}

static void
nullCallback (
  pa_context* context,
  int success,
  void* userdata)
{
  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}


static void
waitop (pa_operation *op)
{
  while (pa_operation_get_state (op) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (gstate.pamainloop);
  }
  mdextfree (op);
  pa_operation_unref (op);
}

static void
pulse_close (void)
{
  if (ginit) {
    pa_threaded_mainloop_stop (gstate.pamainloop);
    mdextfree (gstate.pamainloop);
    pa_threaded_mainloop_free (gstate.pamainloop);
  }
  ginit = 0;
}

static void
pulse_disconnect (void)
{
  if (gstate.pacontext != NULL) {
    pa_threaded_mainloop_lock (gstate.pamainloop);
    pa_context_disconnect (gstate.pacontext);
    mdextfree (gstate.pacontext);
    pa_context_unref (gstate.pacontext);
    gstate.pacontext = NULL;
    pa_threaded_mainloop_unlock (gstate.pamainloop);
  }
}

static void
voliProcessFailure (char *name)
{
  int       rc;

  if (gstate.pacontext != NULL) {
    rc = pa_context_errno (gstate.pacontext);
    fprintf (stderr, "%s: err:%d %s\n", name, rc, pa_strerror(rc));
    pulse_disconnect ();
  }
  pulse_close ();
}

static void
init_context (void)
{
  pa_proplist           *paprop;
  pa_mainloop_api       *paapi;

  pa_threaded_mainloop_lock (gstate.pamainloop);
  paapi = pa_threaded_mainloop_get_api (gstate.pamainloop);
  paprop = pa_proplist_new();
  pa_proplist_sets (paprop, PA_PROP_APPLICATION_NAME, "bdj4");
  pa_proplist_sets (paprop, PA_PROP_MEDIA_ROLE, "music");
  gstate.pacontext = pa_context_new_with_proplist (paapi, "bdj4", paprop);
  mdextalloc (gstate.pacontext);
  pa_proplist_free (paprop);
  if (gstate.pacontext == NULL) {
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    voliProcessFailure ("new context");
    return;
  }

  gstate.pastate = PA_CONTEXT_UNCONNECTED;
  gstate.state = STATE_WAIT;
  pa_context_set_state_callback (gstate.pacontext, &connCallback, &gstate);

  pa_context_connect (gstate.pacontext, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
  pa_threaded_mainloop_unlock (gstate.pamainloop);
}

static void
getSinkCallback (
  pa_context            *context,
  const pa_sink_info    *i,
  int                   eol,
  void                  *userdata)
{
  pacallback_t  *cbdata = (pacallback_t *) userdata;
  int           defflag = 0;
  size_t        idx;

  if (eol != 0) {
    pa_threaded_mainloop_signal (gstate.pamainloop, 0);
    return;
  }
  if (strcmp (i->name, cbdata->sinklist->defname) == 0) {
    defflag = 1;
  }

  idx = cbdata->sinklist->count;
  ++cbdata->sinklist->count;
  cbdata->sinklist->sinklist = mdrealloc (cbdata->sinklist->sinklist,
      cbdata->sinklist->count * sizeof (volsinkitem_t));
  cbdata->sinklist->sinklist [idx].defaultFlag = defflag;
  cbdata->sinklist->sinklist [idx].idxNumber = i->index;
  cbdata->sinklist->sinklist [idx].name = mdstrdup (i->name);
  cbdata->sinklist->sinklist [idx].nmlen = strlen (cbdata->sinklist->sinklist [idx].name);
  cbdata->sinklist->sinklist [idx].description = mdstrdup (i->description);
  if (defflag) {
    dataFree (cbdata->sinklist->defname);
    cbdata->sinklist->defname = mdstrdup (cbdata->sinklist->sinklist [idx].name);
  }

  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}

#endif /* hdr_pulse_pulseaudio */
