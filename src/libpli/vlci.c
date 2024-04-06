/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _lib_libvlc_new

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#if _hdr_stdatomic
# include <stdatomic.h>
#endif
#if defined(__STDC_NO_ATOMICS__)
# define _Atomic(type) type
#endif
#include <string.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pli.h"
#include "vlci.h"

#define VLCDEBUG 0
#define SILENCE_LOG 0
#define STATE_TO_VALUE 0

typedef struct vlcData {
  libvlc_instance_t       *inst;
  char                    version [40];
  libvlc_media_t          *media;
  libvlc_media_player_t   *mp;
  _Atomic(libvlc_state_t) state;
  int                     argc;
  char                    **argv;
  char                    *device;
} vlcData_t;

# if VLCDEBUG

typedef struct {
  libvlc_state_t        state;
  const char *          name;
} stateMap_t;

static const stateMap_t stateMap[] = {
  { libvlc_NothingSpecial,  "idle" },
  { libvlc_Opening,         "opening" },
  { libvlc_Buffering,       "buffering" },
  { libvlc_Playing,         "playing" },
  { libvlc_Paused,          "paused" },
  { libvlc_Stopped,         "stopped" },
  { libvlc_Ended,           "ended" },
  { libvlc_Error,           "error" }
};
enum {
  stateMapMax = (sizeof (stateMap) / sizeof (stateMap_t))
};

static const char *stateToStr (libvlc_state_t state); /* for debugging */

# endif /* VLCDEBUG */

static bool vlcHaveAudioDevList (void);
static void vlcEventHandler (const struct libvlc_event_t *event, void *);
static void vlcReleaseMedia (vlcData_t *vlcData);
static void vlcSetAudioOutput (vlcData_t *vlcData);
static void vlcCreateNewMediaPlayer (vlcData_t *vlcData);

#if STATE_TO_VALUE
static libvlc_state_t stateToValue (char *name);
#endif
#if SILENCE_LOG
static void silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args);
#endif

/* get media values */

ssize_t
vlcGetDuration (vlcData_t *vlcData)
{
  libvlc_time_t     tm;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  tm = libvlc_media_player_get_length (vlcData->mp);
  return tm;
}

ssize_t
vlcGetTime (vlcData_t *vlcData)
{
  libvlc_time_t     tm;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  tm = libvlc_media_player_get_time (vlcData->mp);
  return tm;
}

/* media commands */

int
vlcStop (vlcData_t *vlcData)
{
  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  libvlc_media_player_stop (vlcData->mp);
  return 0;
}

int
vlcPause (vlcData_t *vlcData)
{
  int   rc;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  rc = 1;
  if (vlcData->state == libvlc_Playing) {
    libvlc_media_player_set_pause (vlcData->mp, 1);
    rc = 0;
  } else if (vlcData->state == libvlc_Paused) {
    libvlc_media_player_set_pause (vlcData->mp, 0);
    rc = 0;
  }
  return rc;
}

int
vlcPlay (vlcData_t *vlcData)
{
  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  libvlc_media_player_play (vlcData->mp);
  return 0;
}

ssize_t
vlcSeek (vlcData_t *vlcData, ssize_t pos)
{
  libvlc_time_t     dur = 0;
  float             fpos = 0.0;
  ssize_t           newpos = pos;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  if ((vlcData->state == libvlc_Playing ||
      vlcData->state == libvlc_Paused) &&
      pos >= 0) {
    dur = libvlc_media_player_get_length (vlcData->mp);
    fpos = (float) ((double) pos / (double) dur);
    libvlc_media_player_set_position (vlcData->mp, fpos);
  }
  fpos = libvlc_media_player_get_position (vlcData->mp);
  newpos = (ssize_t) round ((double) fpos * (double) dur);
  // fprintf (stderr, "vlci: seek: dpos: %" PRId64 " new-pos: %.6f new-pos: %" PRId64 "\n", dpos, pos, newpos);
  return newpos;
}

double
vlcRate (vlcData_t *vlcData, double drate)
{
  float     rate;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  if (vlcData->state == libvlc_Playing) {
    rate = (float) drate;
    libvlc_media_player_set_rate (vlcData->mp, rate);
  }
  rate = libvlc_media_player_get_rate (vlcData->mp);
  return (double) rate;
}

/* other commands */

int
vlcSetAudioDev (vlcData_t *vlcData, const char *dev)
{
  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  dataFree (vlcData->device);
  vlcData->device = NULL;
  if (dev != NULL && strlen (dev) > 0) {
    vlcData->device = mdstrdup (dev);
  }

  return 0;
}

const char *
vlcVersion (vlcData_t *vlcData)
{
  return vlcData->version;
}

plistate_t
vlcState (vlcData_t *vlcData)
{
  plistate_t  state = PLI_STATE_NONE;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return state;
  }

  switch ((int) vlcData->state) {
    case libvlc_NothingSpecial: { state = PLI_STATE_IDLE; break; }
    case libvlc_Opening: { state = PLI_STATE_OPENING; break; }
    case libvlc_Buffering: { state = PLI_STATE_BUFFERING; break; }
    case libvlc_Playing: { state = PLI_STATE_PLAYING; break; }
    case libvlc_Paused: { state = PLI_STATE_PAUSED; break; }
    case libvlc_Stopped: { state = PLI_STATE_STOPPED; break; }
    case libvlc_Ended: { state = PLI_STATE_ENDED; break; }
    case libvlc_Error: { state = PLI_STATE_ERROR; break; }
  }
  return state;
}

/* media commands */

int
vlcMedia (vlcData_t *vlcData, const char *fn)
{
  libvlc_event_manager_t  *em;

  if (vlcData == NULL || vlcData->inst == NULL) {
    return -1;
  }

  if (! fileopFileExists (fn)) {
    return -1;
  }

  vlcReleaseMedia (vlcData);

  vlcData->media = libvlc_media_new_path (vlcData->inst, fn);
  mdextalloc (vlcData->media);

  /* the windows audio sink works better if a new media player is */
  /* created for each new medium. if this is not here, windows will */
  /* switch audio sinks if the default is changed. even though the */
  /* vlc-audio-output is set and re-set. */
  vlcCreateNewMediaPlayer (vlcData);
  if (vlcData->mp == NULL) {
    return -1;
  }
  libvlc_media_player_set_rate (vlcData->mp, 1.0);
  libvlc_media_player_set_media (vlcData->mp, vlcData->media);

  em = libvlc_media_event_manager (vlcData->media);
  libvlc_event_attach (em, libvlc_MediaStateChanged,
      &vlcEventHandler, vlcData);

  vlcSetAudioOutput (vlcData);

  return 0;
}

/* initialization and cleanup */

vlcData_t *
vlcInit (int vlcargc, char *vlcargv [], char *vlcopt [])
{
  vlcData_t *   vlcData;
  char *        tptr;
  char *        nptr;
  int           i;
  int           j;

  vlcData = (vlcData_t *) mdmalloc (sizeof (vlcData_t));
  vlcData->inst = NULL;
  vlcData->media = NULL;
  vlcData->mp = NULL;
  vlcData->argv = NULL;
  vlcData->state = libvlc_NothingSpecial;
  vlcData->device = NULL;

  vlcData->argv = (char **) mdmalloc (sizeof (char *) * (size_t) (vlcargc + 1));

  for (i = 0; i < vlcargc; ++i) {
    nptr = mdstrdup (vlcargv [i]);
    vlcData->argv [i] = nptr;
  }

  j = 0;
  while ((tptr = vlcopt [j]) != NULL) {
    ++vlcargc;
    vlcData->argv = (char **) mdrealloc (vlcData->argv,
        sizeof (char *) * (size_t) (vlcargc + 1));
    nptr = mdstrdup (tptr);
    vlcData->argv [i] = nptr;
    ++i;
    ++j;
  }

  vlcData->argc = vlcargc;
  vlcData->argv [vlcData->argc] = NULL;

  strncpy (vlcData->version, libvlc_get_version (), sizeof (vlcData->version));
  tptr = strchr (vlcData->version, ' ');
  if (tptr != NULL) {
    *tptr = '\0';
  }

  if (vlcData->inst == NULL) {
    vlcData->inst = libvlc_new (vlcData->argc, (const char * const *) vlcData->argv);
    mdextalloc (vlcData->inst);
  }
#if SILENCE_LOG
  libvlc_log_set (vlcData->inst, silence, NULL);
#endif

  /* windows seems to need this, even though the media player is going */
  /* to be released and re-created. */
  vlcCreateNewMediaPlayer (vlcData);

  return vlcData;
}

void
vlcClose (vlcData_t *vlcData)
{
  int                     i;

  if (vlcData != NULL) {
    vlcReleaseMedia (vlcData);
    if (vlcData->mp != NULL) {
      libvlc_media_player_stop (vlcData->mp);
      mdextfree (vlcData->mp);
      libvlc_media_player_release (vlcData->mp);
      vlcData->mp = NULL;
    }
    if (vlcData->inst != NULL) {
      mdextfree (vlcData->inst);
      libvlc_release (vlcData->inst);
      vlcData->inst = NULL;
    }
    if (vlcData->argv != NULL) {
      for (i = 0; i < vlcData->argc; ++i) {
        dataFree (vlcData->argv [i]);
      }
      mdfree (vlcData->argv);
      vlcData->argv = NULL;
    }
    dataFree (vlcData->device);
    vlcData->device = NULL;
    vlcData->state = libvlc_NothingSpecial;
    mdfree (vlcData);
  }
}

void
vlcRelease (vlcData_t *vlcData)
{
  vlcClose (vlcData);
}

/* event handlers */

static void
vlcEventHandler (const struct libvlc_event_t *event, void *userdata)
{
  vlcData_t     *vlcData = (vlcData_t *) userdata;

  if (event->type == libvlc_MediaStateChanged) {
    vlcData->state = (libvlc_state_t) event->u.media_state_changed.new_state;
  }
}

/* internal routines */

static bool
vlcHaveAudioDevList (void)
{
  bool    rc;

  rc = false;
#if _lib_libvlc_audio_output_device_enum && \
    LIBVLC_VERSION_INT >= LIBVLC_VERSION(2,2,0,0)
  rc = true;
#endif
  return rc;
}

static void
vlcReleaseMedia (vlcData_t *vlcData)
{
  libvlc_event_manager_t  *em;

  if (vlcData->media != NULL) {
    em = libvlc_media_event_manager (vlcData->media);
    libvlc_event_detach (em, libvlc_MediaStateChanged,
        &vlcEventHandler, vlcData);
    mdextfree (vlcData->media);
    libvlc_media_release (vlcData->media);
    vlcData->media = NULL;
  }
}

static void
vlcSetAudioOutput (vlcData_t *vlcData)
{
  /* Does not work on linux. 3.x documentation says pulseaudio does not */
  /* have a device parameter. pulseaudio uses the PULSE_SINK env var. */
  /* macos: need to call this to switch audio devices on macos. */
  /* windows: seems to need this for setup and switching. */
  if (vlcHaveAudioDevList ()) {
    if (vlcData->device != NULL) {
      const char  *carg = vlcData->device;

      {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4,0,0,0)
        int   rc = 0;
        rc = libvlc_audio_output_device_set (vlcData->mp, carg);
#else
        libvlc_audio_output_device_set (vlcData->mp, NULL, carg);
#endif
      }
    }
  }
}

static void
vlcCreateNewMediaPlayer (vlcData_t *vlcData)
{
  if (vlcData->inst == NULL) {
    return;
  }

  if (vlcData->mp != NULL) {
    mdextfree (vlcData->mp);
    libvlc_media_player_release (vlcData->mp);
  }
  vlcData->mp = libvlc_media_player_new (vlcData->inst);
  mdextalloc (vlcData->mp);

  if (vlcData->inst != NULL && vlcData->mp != NULL) {
    libvlc_audio_set_volume (vlcData->mp, 100);
#if ! defined(VLC_NO_ROLE) && \
      LIBVLC_VERSION_INT >= LIBVLC_VERSION(3,0,0,0)
    libvlc_media_player_set_role (vlcData->mp, libvlc_role_Music);
#endif
  }
}


/* for debugging */
# if VLCDEBUG

static const char *
stateToStr (libvlc_state_t state)
{
  size_t      i;
  const char  *tptr;

  tptr = "";
  for (i = 0; i < stateMapMax; ++i) {
    if (state == stateMap[i].state) {
      tptr = stateMap[i].name;
      break;
    }
  }
  return tptr;
}

# endif /* VLCDEBUG */

# if STATE_TO_VALUE

static libvlc_state_t
stateToValue (char *name)
{
  size_t          i;
  libvlc_state_t  state;

  state = libvlc_NothingSpecial;
  for (i = 0; i < stateMapMax; ++i) {
    if (strcmp (name, stateMap[i].name) == 0) {
      state = stateMap[i].state;
      break;
    }
  }
  return state;
}

# endif

# if SILENCE_LOG
static void
silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args)
{
  return;
}
# endif

#endif /* have libvlc_new() */
