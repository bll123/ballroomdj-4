/*
 * Copyright 2016 Brad Lanam Walnut Creek CA
 * Copyright 2020 Brad Lanam Pleasant Hill CA
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
/*
 * pulse audio
 *
 * References:
 *   https://github.com/cdemoulins/pamixer/blob/master/src/pulseaudio.cc
 *   https://freedesktop.org/software/pulseaudio/doxygen/
 */

#if __has_include (<pulse/pulseaudio.h>)

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>
#include <stdatomic.h>

#include "bdjstring.h"
#include "tmutil.h"
#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

enum {
  STATE_OK = 0,
  STATE_WAIT = 1,
  STATE_FAIL = -1,
  DEFNM_MAX_SZ = 300,
};

typedef struct {
  char      *defaultsink;
  bool      changed;
} pa_track_data_t;

typedef struct {
  pa_threaded_mainloop  *pamainloop;
  pa_context            *context;
  pa_context_state_t    pastate;
  _Atomic(int)          state;
} state_t;

static state_t    gstate;
static bool       ginit = false;

typedef union {
  char            defname [DEFNM_MAX_SZ];
  pa_cvolume      *vol;
  volsinklist_t   *sinklist;
} pacallback_t;

static void serverInfoCallback (pa_context *context, const pa_server_info *i, void *userdata);
static void getDefaultSink (char *defsinkname, size_t sz);
static void sinkVolCallback (pa_context *context, const pa_sink_info *i, int eol, void *userdata);
static void connCallback (pa_context *context, void *userdata);
static void nullCallback (pa_context* context, int success, void* userdata);
static void waitop (pa_operation *op);
static void pulse_close (void);
static void pulse_disconnect (void);
static void voliProcessFailure (char *name);
static void init_context (void);
static void getSinkCallback (pa_context *context, const pa_sink_info *i, int eol, void *userdata);

void
voliDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Pulse Audio";
  ret [c++] = NULL;
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
  pa_track_data_t *trackdata;

  if (udata == NULL || *udata == NULL) {
    return;
  }

  trackdata = *udata;

  dataFree (trackdata->defaultsink);
  dataFree (trackdata);
  *udata = NULL;

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
  char            defsinkname [DEFNM_MAX_SZ];
  pa_track_data_t *trackdata = NULL;


  if (udata == NULL) {
    return -1;
  }

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  count = 0;
  while (count < 40) {
    int     rc;

    if (! ginit) {
      gstate.context = NULL;
      gstate.pamainloop = pa_threaded_mainloop_new();
      mdextalloc (gstate.pamainloop);
      pa_threaded_mainloop_start (gstate.pamainloop);
      ginit = true;
    }

    if (gstate.context == NULL) {
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

    rc = pa_context_errno (gstate.context);
    voliProcessFailure ("init-context");

    if (rc == PA_ERR_CONNECTIONREFUSED) {
      /* this, of course, does not work in a pipewire situation */
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
  /* this will also let us know if the default sink has changed */
  getDefaultSink (defsinkname, sizeof (defsinkname));

  if (*udata == NULL) {
    trackdata = mdmalloc (sizeof (*trackdata));
    trackdata->defaultsink = mdstrdup (defsinkname);
    trackdata->changed = false;
    *udata = trackdata;
  } else {
    trackdata = *udata;

    /* leave the changed flag set until the user fetches it */
    if (strcmp (trackdata->defaultsink, defsinkname) != 0) {
      dataFree (trackdata->defaultsink);
      trackdata->defaultsink = mdstrdup (defsinkname);
      trackdata->changed = true;
    }
  }

  if (action == VOL_CHK_SINK) {
    bool    orig;

    /* leave the changed flag set until the user fetches it */
    orig = trackdata->changed;
    trackdata->changed = false;
    return orig;
  }

  if (action == VOL_GETSINKLIST) {
    /* unfortunately, linux pulseaudio/pipewire does not have a */
    /* concept of use-the-default-output */
    dataFree (sinklist->defname);
    sinklist->defname = mdstrdup (defsinkname);
    sinklist->sinklist = NULL;
    sinklist->count = 0;
    cbdata.sinklist = sinklist;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_list (
        gstate.context, &getSinkCallback, &cbdata);
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

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    /* getvolume or setvolume */
    pa_volume_t     avgvol;
    pa_cvolume      pavol;

    /* the volume should be associated with the supplied sink, */
    if (sinkname == NULL || ! *sinkname) {
      sinkname = defsinkname;
    }

    cbdata.vol = &pavol;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_by_name (
        gstate.context, sinkname, &sinkVolCallback, &cbdata);
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
          gstate.context, sinkname, nvol, nullCallback, NULL);
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
        gstate.context, sinkname, &sinkVolCallback, &cbdata);
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
getDefaultSink (char *defsinkname, size_t sz)
{
  pa_operation    *op = NULL;
  pacallback_t    cbdata;

  if (gstate.pastate != PA_CONTEXT_READY) {
    return;
  }

  *defsinkname = '\0';
  pa_threaded_mainloop_lock (gstate.pamainloop);
  op = pa_context_get_server_info (
      gstate.context, &serverInfoCallback, &cbdata);
  mdextalloc (op);
  if (! op) {
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    voliProcessFailure ("serverinfo");
    return;
  }
  waitop (op);
  pa_threaded_mainloop_unlock (gstate.pamainloop);
  stpecpy (defsinkname, defsinkname + sz, cbdata.defname);
}

static void
serverInfoCallback (
  pa_context            *context,
  const pa_server_info  *i,
  void                  *userdata)
{
  pacallback_t  *cbdata = (pacallback_t *) userdata;

  stpecpy (cbdata->defname, cbdata->defname + sizeof (cbdata->defname),
      i->default_sink_name);
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
connCallback (pa_context *context, void* userdata)
{
  state_t     *stdata = (state_t *) userdata;

  if (context == NULL) {
    stdata->pastate = PA_CONTEXT_FAILED;
    stdata->state = STATE_FAIL;
    pa_threaded_mainloop_signal (stdata->pamainloop, 0);
    return;
  }

  stdata->pastate = pa_context_get_state (context);
  pa_threaded_mainloop_signal (stdata->pamainloop, 0);

  switch (stdata->pastate)
  {
    case PA_CONTEXT_READY: {
      stdata->state = STATE_OK;
      break;
    }
    case PA_CONTEXT_FAILED: {
      stdata->state = STATE_FAIL;
      break;
    }
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_TERMINATED: {
      stdata->state = STATE_WAIT;
      break;
    }
  }
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
  ginit = false;
}

static void
pulse_disconnect (void)
{
  if (gstate.context != NULL) {
    pa_threaded_mainloop_lock (gstate.pamainloop);
    pa_context_disconnect (gstate.context);
    mdextfree (gstate.context);
    pa_context_unref (gstate.context);
    gstate.context = NULL;
    pa_threaded_mainloop_unlock (gstate.pamainloop);
  }
}

static void
voliProcessFailure (char *name)
{
  int       rc;

  if (gstate.context != NULL) {
    rc = pa_context_errno (gstate.context);
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
  gstate.context = pa_context_new_with_proplist (paapi, "bdj4", paprop);
  mdextalloc (gstate.context);
  pa_proplist_free (paprop);
  if (gstate.context == NULL) {
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    voliProcessFailure ("new context");
    return;
  }

  gstate.pastate = PA_CONTEXT_UNCONNECTED;
  gstate.state = STATE_WAIT;
  pa_context_set_state_callback (gstate.context, &connCallback, &gstate);

  pa_context_connect (gstate.context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
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
