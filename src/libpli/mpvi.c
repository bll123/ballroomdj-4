/*
 * Copyright 2018 Brad Lanam Walnut Creek CA US
 * Copyright 2020 Brad Lanam Pleasant Hill CA
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _lib_mpv_create

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#define MPV_ENABLE_DEPRECATED 0
#include <mpv/client.h>

#include "bdjstring.h"
#include "mdebug.h"
#include "mpvi.h"
#include "pli.h"
#include "volsink.h"

enum {
  MPVDEBUG = 1,
};

typedef struct {
  plistate_t        state;
  const char *      name;
} playStateMap_t;

static const playStateMap_t playStateMap [] = {
  { PLI_STATE_NONE, "none" },
  { PLI_STATE_IDLE, "idle" },
  { PLI_STATE_OPENING, "opening" },
  { PLI_STATE_BUFFERING, "buffering" },
  { PLI_STATE_PLAYING, "playing" },
  { PLI_STATE_PAUSED, "paused" },
  { PLI_STATE_STOPPED, "stopped" },
  { PLI_STATE_ERROR, "error" }
};
enum {
  playStateMapMax = sizeof (playStateMap) / sizeof (playStateMap_t),
};

typedef struct {
  mpv_event_id          state;
  const char *          name;
  plistate_t            plistate;
} stateMap_t;

static const stateMap_t stateMap [] = {
  { MPV_EVENT_NONE,             "none", PLI_STATE_NONE },
  /* Set as buffering.  When the first duration property message is */
  /* received, the state is set to playing.                         */
  { MPV_EVENT_START_FILE,       "opening", PLI_STATE_BUFFERING },
  { MPV_EVENT_FILE_LOADED,      "playing", PLI_STATE_PLAYING },
  { MPV_EVENT_PROPERTY_CHANGE,  "property-chg", PLI_STATE_NONE },
  { MPV_EVENT_SEEK,             "seeking", PLI_STATE_NONE },
  { MPV_EVENT_PLAYBACK_RESTART, "playafterseek", PLI_STATE_NONE },
  { MPV_EVENT_END_FILE,         "stopped", PLI_STATE_STOPPED },
  { MPV_EVENT_SHUTDOWN,         "ended", PLI_STATE_STOPPED },
  /* these next three are only for debugging */
 //  { MPV_EVENT_TRACKS_CHANGED,   "tracks-changed", PLI_STATE_NONE },
  { MPV_EVENT_AUDIO_RECONFIG,   "audio-reconf", PLI_STATE_NONE },
 //  { MPV_EVENT_METADATA_UPDATE,  "metadata-upd", PLI_STATE_NONE }
};
enum {
  stateMapMax = sizeof (stateMap) / sizeof(stateMap_t),
  stateMapIdxMax = 40,          /* mpv currently has 24 states coded */
};


typedef struct mpvData {
  mpv_handle            *inst;
  char                  version [40];
  plistate_t            state;
  const char            *device;
  double                duration;
  double                tm;
  int                   stateMapIdx [stateMapIdxMax];
  FILE                  *debugfh;
  bool                  paused : 1;
  bool                  hasEvent : 1;   /* flag to process mpv event */
} mpvData_t;

static void mpvCallbackHandler (void *udata);

/* get media values */

ssize_t
mpvGetDuration (mpvData_t *mpvData)
{
  double    tm = 0.0;
  ssize_t   dur;
  int       status;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return 0;
  }

  status = mpv_get_property (mpvData->inst, "duration", MPV_FORMAT_DOUBLE, &tm);
  dur = (ssize_t) (tm * 1000.0);
  return dur;
}

ssize_t
mpvGetTime (mpvData_t *mpvData)
{
  ssize_t   tm = 0;
  double    mpvtm;
  int       status;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return 0;
  }

  status = mpv_get_property (mpvData->inst, "time-pos", MPV_FORMAT_DOUBLE, &mpvtm);
  tm = (ssize_t) (mpvtm * 1000.0);
  return tm;
}

/* media checks */

int
mpvIsPlaying (mpvData_t *mpvData)
{
  int     rval = 0;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  if (mpvData->state == PLI_STATE_OPENING ||
      mpvData->state == PLI_STATE_BUFFERING ||
      mpvData->state == PLI_STATE_PLAYING) {
    rval = 1;
  }
  return rval;
}

int
mpvIsPaused (mpvData_t *mpvData)
{
  int       rval;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  rval = 0;
  if (mpvData->state == PLI_STATE_PAUSED) {
    rval = 1;
  }
  return rval;
}

/* media commands */

int
mpvStop (mpvData_t *mpvData)
{
  int         status;
  const char  *cmd [] = { "stop", NULL };

  if (mpvData->inst == NULL) {
    return -1;
  }

  /* stop: stops playback and clears playlist */
  /* difference: vlc's stop command does not clear the playlist */
  status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
  if (mpvData->debugfh != NULL) {
    fprintf (mpvData->debugfh, "stop:status:%d %s\n", status, mpv_error_string(status));
  }
#endif
  return 0;
}

int
mpvPause (mpvData_t *mpvData)
{
  int       rc = 1;
  int       status;
  int       val;

  if (mpvData->inst == NULL) {
    return -1;
  }

  if (mpvData->state == PLI_STATE_PLAYING && mpvData->paused == 0) {
    val = 1;
    status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "pause-%d:status:%d %s\n", val, status, mpv_error_string(status));
    }
#endif
    mpvData->paused = true;
    mpvData->state = PLI_STATE_PAUSED;
    rc = 0;
  } else if (mpvData->state == PLI_STATE_PAUSED && mpvData->paused == 1) {
    val = 0;
    status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "pause-%d:status:%d %s\n", val, status, mpv_error_string(status));
    }
#endif
    mpvData->paused = false;
    mpvData->state = PLI_STATE_PLAYING;
    rc = 0;
  }
  return rc;
}

int
mpvPlay (mpvData_t *mpvData)
{
  int       rc;
  int       status;
  int       val;

  if (mpvData->inst == NULL) {
    return -1;
  }

  rc = 1;
  if (mpvData->state == PLI_STATE_PAUSED && mpvData->paused == 1) {
    val = 0;
    status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "play:status:%d %s\n", status, mpv_error_string(status));
    }
#endif
    mpvData->paused = false;
    mpvData->state = PLI_STATE_PLAYING;
    rc = 0;
  }
  return rc;
}

ssize_t
mpvSeek (mpvData_t *mpvData, ssize_t pos)
{
  int         status;
  char        oldlocale [100];


  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  /* work around programming incompetence */
  strlcpy (oldlocale, setlocale (LC_NUMERIC, NULL), sizeof (oldlocale));
  setlocale (LC_NUMERIC, "C");

  if ((mpvData->state == PLI_STATE_PLAYING ||
       mpvData->state == PLI_STATE_PAUSED) &&
      pos >= 0) {
    double      dpos;
    double      dur;
    char        spos [40];
    const char  *cmd [] = { "seek", spos, "absolute", NULL };

    dur = mpvData->duration;
    dpos = (double) pos / dur;
    snprintf (spos, sizeof (spos), "%.1f", dpos);
    status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "seek-%s:status:%d %s\n", spos, status, mpv_error_string(status));
    }
#endif
  }

  setlocale (LC_NUMERIC, oldlocale);

  return mpvGetTime (mpvData);
}

double
mpvRate (mpvData_t *mpvData, double drate)
{
  int         status;
  double      rate;
  char        oldlocale [100];


  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  /* work around programming incompetence */
  strlcpy (oldlocale, setlocale (LC_NUMERIC, NULL), sizeof (oldlocale));
  setlocale (LC_NUMERIC, "C");

  if (mpvData->state == PLI_STATE_PLAYING) {
    rate = drate;
    status = mpv_set_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &rate);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "speed-%.2f:status:%d %s\n", rate, status, mpv_error_string(status));
    }
#endif
  }

  status = mpv_get_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &rate);
#if MPVDEBUG
  if (mpvData->debugfh != NULL) {
    fprintf (mpvData->debugfh, "speed-get:status:%d %s\n", status, mpv_error_string(status));
  }
#endif
  setlocale (LC_NUMERIC, oldlocale);

  return rate;
}

/* other commands */

bool
mpvHaveAudioDevList (void)
{
  return true;
}

int
mpvAudioDevSet (mpvData_t *mpvData, const char *dev)
{
  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  if (mpvData->device != NULL) {
    mdfree ((void *) mpvData->device);
  }

  mpvData->device = NULL;
  if (strlen (dev) > 0) {
    mpvData->device = mdstrdup (dev);
  }

  return 0;
}

int
mpvAudioDevList (mpvData_t *mpvData, volsinklist_t *sinklist)
{
  mpv_node      anodes;
  mpv_node_list *infolist;
  mpv_node_list *nodelist;
  char          *nmptr = NULL;
  char          *descptr = NULL;
  int           status;
  int           count = 0;

  sinklist->defname = NULL;
  sinklist->sinklist = NULL;
  sinklist->count = 0;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return 0;
  }

  status = mpv_get_property (mpvData->inst, "audio-device-list", MPV_FORMAT_NODE, &anodes);
  if (anodes.format != MPV_FORMAT_NODE_ARRAY) {
    return 0;
  }

  nodelist = anodes.u.list;
  for (int i = 0; i < nodelist->num; ++i) {
    infolist = nodelist->values [i].u.list;

    nmptr = NULL;
    descptr = NULL;

    for (int j = 0; j < infolist->num; ++j) {
      if (strcmp (infolist->keys [j], "name") == 0) {
        nmptr = infolist->values [j].u.string;
      } else if (strcmp (infolist->keys [j], "description") == 0) {
        descptr = infolist->values [j].u.string;
      } else {
#if MPVDEBUG
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "dev: %s\n", infolist->keys [j]);
        }
#endif
      }
    }

    ++sinklist->count;
    sinklist->sinklist = mdrealloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));
    sinklist->sinklist [count].defaultFlag = 0;
    sinklist->sinklist [count].idxNumber = count;
    sinklist->sinklist [count].name = mdstrdup (nmptr);
    sinklist->sinklist [count].description = mdstrdup (descptr);
    if (count == 0) {
      sinklist->defname = mdstrdup (nmptr);
    }
    ++count;
  }

  mpv_free_node_contents (&anodes);

  return 0;
}

const char *
mpvVersion (mpvData_t *mpvData)
{
  return mpvData->version;
}

plistate_t
mpvState (mpvData_t *mpvData)
{
  if (mpvData == NULL || mpvData->inst == NULL) {
    return PLI_STATE_NONE;
  }

  return mpvData->state;
}

/* media commands */

int
mpvMedia (mpvData_t *mpvData, const char *fn)
{
  int           status;
  struct stat   statbuf;
  double        dval;
  const char    *cmd [] = {"loadfile", fn, "replace", NULL};

  if (mpvData == NULL || mpvData->inst == NULL) {
    return -1;
  }

  if (stat (fn, &statbuf) != 0) {
    return -1;
  }

  if (mpvData->device != NULL) {
    status = mpv_set_property (mpvData->inst, "audio-device", MPV_FORMAT_STRING, (void *) mpvData->device);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "set-ad:status:%d %s\n", status, mpv_error_string(status));
    }
#endif
  }

    /* reset the duration and time */
  mpvData->duration = 0.0;
  mpvData->tm = 0.0;

  /* like many players, mpv will start playing when the 'loadfile' */
  /* command is executed. */

  status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
  if (mpvData->debugfh != NULL) {
    fprintf (mpvData->debugfh, "loadfile:status:%d %s\n", status, mpv_error_string(status));
  }
#endif

  dval = 1.0;
  status = mpv_set_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &dval);
#if MPVDEBUG
  if (mpvData->debugfh != NULL) {
    fprintf (mpvData->debugfh, "speed-1:status:%d %s\n", status, mpv_error_string(status));
  }
#endif
  return 0;
}

/* initialization and cleanup */

mpvData_t *
mpvInit (void)
{
  int           status;
  int           gstatus;
  mpvData_t     *mpvData = NULL;
  unsigned int  ivers;
  char          tvers [20];
  char          oldlocale [100];


  /* work around programming incompetence */
  strlcpy (oldlocale, setlocale (LC_NUMERIC, NULL), sizeof (oldlocale));
  setlocale (LC_NUMERIC, "C");

  mpvData = mdmalloc (sizeof (mpvData_t));
  mpvData->inst = NULL;
  mpvData->state = PLI_STATE_NONE;
  mpvData->device = NULL;
  mpvData->paused = false;
  mpvData->duration = 0.0;
  mpvData->tm = 0.0;
  mpvData->hasEvent = false;
  for (int i = 0; i < stateMapIdxMax; ++i) {
    mpvData->stateMapIdx [i] = 0;
  }
  for (int i = 0; i < stateMapMax; ++i) {
    mpvData->stateMapIdx [stateMap [i].state] = i;
  }

  mpvData->debugfh = NULL;
#if MPVDEBUG
  mpvData->debugfh = fopen ("mpvdebug.txt", "w+");
#endif

  ivers = mpv_client_api_version();
  snprintf (tvers, sizeof (tvers), "%d.%d", ivers >> 16, ivers & 0xFF);
  strlcpy (mpvData->version, tvers, sizeof (mpvData->version));
fprintf (stderr, "vers: %s\n", mpvData->version);

  gstatus = 0;

  if (mpvData->inst == NULL) {
    mpvData->inst = mpv_create ();
fprintf (stderr, "create: %s\n", mpvData->inst == NULL ? "ng" : "ok");

    if (mpvData->inst != NULL) {
      double vol = 100.0;

      mpv_set_wakeup_callback (mpvData->inst, &mpvCallbackHandler, mpvData);

      status = mpv_initialize (mpvData->inst);
fprintf (stderr, "init: %d\n", status);
printf("mpv API error: %s\n", mpv_error_string (status));
      if (status < 0) {
        gstatus = status;
      } else {
        mpv_set_property (mpvData->inst, "volume", MPV_FORMAT_DOUBLE, &vol);
//        mpv_observe_property (mpvData->inst, 0, "duration", MPV_FORMAT_DOUBLE);
//        mpv_observe_property (mpvData->inst, 0, "time-pos", MPV_FORMAT_DOUBLE);
      }
    }
  }

  if (mpvData->inst == NULL || gstatus != 0) {
    mdfree (mpvData);
    mpvData = NULL;
  }

  setlocale (LC_NUMERIC, oldlocale);

  return mpvData;
}

void
mpvClose (mpvData_t *mpvData)
{
  if (mpvData == NULL) {
    return;
  }

  if (mpvData->inst != NULL) {
    mpv_terminate_destroy (mpvData->inst);
    mpvData->inst = NULL;
  }
  if (mpvData->device != NULL) {
    free ((void *) mpvData->device);
    mpvData->device = NULL;
  }
  if (mpvData->debugfh != NULL) {
    fclose (mpvData->debugfh);
    mpvData->debugfh = NULL;
  }

  mpvData->state = PLI_STATE_STOPPED;
  mdfree (mpvData);
}

void
mpvRelease (mpvData_t *mpvData)
{
  mpvClose (mpvData);
}

/* event handlers */

/* executed in some arbitrary thread */
static void
mpvCallbackHandler (void *udata)
{
  mpvData_t   *mpvData = (mpvData_t *) udata;
  mpv_event   *event = NULL;
  plistate_t  state;

  if (mpvData == NULL || mpvData->inst == NULL) {
    return;
  }

fprintf (stderr, "b4 wait-event:\n");
  event = mpv_wait_event (mpvData->inst, 0.1);
fprintf (stderr, "after wait-event: %d\n", event->event_id);
  state = stateMap [(int) mpvData->stateMapIdx [event->event_id]].plistate;
fprintf (stderr, "  state: %d\n", state);

  while (event->event_id != MPV_EVENT_NONE) {
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "mpv: ev: %d/%s %s\n", mpv_event_name(event->event_id), stateToStr (state));
    }
#endif
#if 0
    if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
      mpv_event_property *prop = (mpv_event_property *) event->data;
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "mpv: ev: prop: %s\n", prop->name);
      }
#endif
      if (strcmp (prop->name, "time-pos") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
          mpvData->tm = * (double *) prop->data;
        }
#if MPVDEBUG
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "mpv: ev: tm: %.2f\n", mpvData->tm);
        }
#endif
      } else if (strcmp (prop->name, "duration") == 0) {
        if (mpvData->state == PLI_STATE_BUFFERING) {
          mpvData->state = PLI_STATE_PLAYING;
#if MPVDEBUG
          if (mpvData->debugfh != NULL) {
            fprintf (mpvData->debugfh, "mpv: state: %s\n", stateToStr (mpvData->state));
          }
#endif
        }
        if (prop->format == MPV_FORMAT_DOUBLE) {
          mpvData->duration = * (double *) prop->data;
        }
#if MPVDEBUG
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "mpv: ev: dur: %.2f\n", mpvData->duration);
        }
#endif
      }
    } else
#endif
    if (state != PLI_STATE_NONE) {
      mpvData->state = state;
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "mpv: state: %s\n", stateToStr (mpvData->state));
      }
#endif
    }

fprintf (stderr, "b4 wait-event:\n");
    event = mpv_wait_event (mpvData->inst, 0.1);
fprintf (stderr, "after wait-event: %d\n", event->event_id);
    state = stateMap [(int) mpvData->stateMapIdx [event->event_id]].plistate;
fprintf (stderr, "  state: %d\n", state);
  }
}

/* internal routines */

#if MPVDEBUG

static const char *
stateToStr (plistate_t state)
{
  int        i;
  const char *tptr;

  tptr = "";
  for (i = 0; i < playStateMapMax; ++i) {
    if (state == playStateMap [i].plistate) {
      tptr = playStateMap [i].name;
      break;
    }
  }
  return tptr;
}

#endif /* MPVDEBUG */

#endif /* _libmpv_create */
