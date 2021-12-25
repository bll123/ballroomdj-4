/*
 * pulse audio
 *   getsinklist
 *   getvolume <sink-name>
 *   setvolume <sink-name> <volume-perc>
 *   close
 *
 * References:
 *   https://github.com/cdemoulins/pamixer/blob/master/pulseaudio.cc
 *   https://freedesktop.org/software/pulseaudio/doxygen/
 */

#include "config.h"

#if _hdr_pulse_pulseaudio

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <memory.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include "volume.h"

static void getSinkCallback (pa_context *context, const pa_sink_info *i, int eol, void *userdata);

typedef struct {
  char      *defname;
  void      *data;
} getSinkData_t;

#define STATE_OK    0
#define STATE_WAIT  1
#define STATE_FAIL  -1
typedef struct {
  pa_threaded_mainloop  *pamainloop;
  pa_context            *pacontext;
  pa_context_state_t    pastate;
  int                   state;
} state_t;

static state_t      gstate;
static int          ginit = 0;

typedef union {
  char            *defname;
  pa_cvolume      *vol;
  getSinkData_t   *sinkdata;
} callback_t;

static void
serverInfoCallback (
  pa_context            *context,
  const pa_server_info  *i,
  void                  *userdata)
{
  callback_t    *cbdata = (callback_t *) userdata;

  cbdata->defname = strdup (i->default_sink_name);
  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}

static void
sinkVolCallback (
  pa_context *context,
  const pa_sink_info *i,
  int eol,
  void *userdata)
{
  callback_t    *cbdata = (callback_t *) userdata;

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
  state_t   *stdata = (state_t *) userdata;

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
  pa_operation_unref (op);
}

static void
pulse_close (void)
{
  if (ginit) {
    pa_threaded_mainloop_stop (gstate.pamainloop);
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
    pa_context_unref (gstate.pacontext);
    gstate.pacontext = NULL;
    pa_threaded_mainloop_unlock (gstate.pamainloop);
  }
}

static void
processfailure (char *name)
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
init_context (void) {
  pa_proplist           *paprop;
  pa_mainloop_api       *paapi;

  pa_threaded_mainloop_lock (gstate.pamainloop);
  paapi = pa_threaded_mainloop_get_api (gstate.pamainloop);
  paprop = pa_proplist_new();
  pa_proplist_sets (paprop, PA_PROP_APPLICATION_NAME, "ballroomdj");
  pa_proplist_sets (paprop, PA_PROP_MEDIA_ROLE, "music");
  gstate.pacontext = pa_context_new_with_proplist (paapi, "ballroomdj", paprop);
  pa_proplist_free (paprop);
  if (gstate.pacontext == NULL) {
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    processfailure ("new context");
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
  callback_t    *cbdata = (callback_t *) userdata;

  char *defflag = "no";

  if (eol != 0) {
    pa_threaded_mainloop_signal (gstate.pamainloop, 0);
    return;
  }
  if (strcmp (i->name, cbdata->sinkdata->defname) == 0) {
    defflag = "yes";
  }

  printf ("%s %d {%s} {%s}\n", defflag, i->index,
       i->name, i->description);
  pa_threaded_mainloop_signal (gstate.pamainloop, 0);
}

void
audioDisconnect (void) {
  /*
   * If this program is made efficient by keeping a connection open
   * to pulseaudio, then pulseaudio will reset the audio track volume
   * when a new track starts.
   */
  pulse_disconnect ();
  pulse_close ();
}

int
process (volaction_t action, char *sinkname, int *vol, char **sinklist)
{
  pa_operation          *op;
  callback_t            cbdata;
  unsigned int          tvol;

  if (! ginit) {
    gstate.pacontext = NULL;
    gstate.pamainloop = pa_threaded_mainloop_new();
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

  if (gstate.pastate != PA_CONTEXT_READY) {
    processfailure ("init context");
    return -1;
  }

  if (action == VOL_GETSINKLIST) {
    char            *defsinkname;
    getSinkData_t   sinkdata;

    defsinkname = NULL;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_server_info (
        gstate.pacontext, &serverInfoCallback, &cbdata);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      processfailure ("serverinfo");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    defsinkname = cbdata.defname;

    cbdata.sinkdata = &sinkdata;
    sinkdata.defname = defsinkname;
    sinkdata.data = sinklist;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_list (
        gstate.pacontext, &getSinkCallback, &cbdata);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      processfailure ("getsink");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);
    if (defsinkname != NULL) {
      free (defsinkname);
    }
  } else {
    /* getvolume or setvolume */
    pa_volume_t     avgvol;
    pa_cvolume      pavol;

    cbdata.vol = &pavol;
    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_by_name (
        gstate.pacontext, sinkname, &sinkVolCallback, &cbdata);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      processfailure ("getsinkbyname");
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
          gstate.pacontext, sinkname, nvol, nullCallback, &cbdata);
      if (! op) {
        pa_threaded_mainloop_unlock (gstate.pamainloop);
        processfailure ("setvol");
        return -1;
      }
      waitop (op);
      pa_threaded_mainloop_unlock (gstate.pamainloop);
    }

    pa_threaded_mainloop_lock (gstate.pamainloop);
    op = pa_context_get_sink_info_by_name (
        gstate.pacontext, sinkname, &sinkVolCallback, &cbdata);
    if (! op) {
      pa_threaded_mainloop_unlock (gstate.pamainloop);
      processfailure ("getvol");
      return -1;
    }
    waitop (op);
    pa_threaded_mainloop_unlock (gstate.pamainloop);

    avgvol = pa_cvolume_avg (&pavol);
    *vol = (int) round ((double) avgvol / (double) PA_VOLUME_NORM * 100.0);
  }

  return 0;
}

#endif /* hdr_pulse_pulseaudio */
