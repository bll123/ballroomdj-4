/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _lib_libvlc_new

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

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "bdjstring.h"
#include "mdebug.h"
#include "pli.h"
#include "vlci.h"
#include "volsink.h"

typedef struct vlcData {
  libvlc_instance_t     *inst;
  char                  version [40];
  libvlc_media_t        *m;
  libvlc_media_player_t *mp;
  libvlc_state_t        state;
  int                   argc;
  char                  **argv;
  char                  *device;
} vlcData_t;

enum {
  SILENCE_LOG = 0,
  STATE_TO_VALUE = 0,
};

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

/* static const char *stateToStr (libvlc_state_t state); */ /* for debugging */
static void       vlcEventHandler (const struct libvlc_event_t *event, void *);

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

/* media checks */

int
vlcIsPlaying (vlcData_t *vlcData)
{
  int     rval;

  rval = 0;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  /*
   * VLC's internal isplaying command returns true if the player is paused.
   * Our implementation is different.
   */
  if (vlcData->state == libvlc_Opening ||
      vlcData->state == libvlc_Buffering ||
      vlcData->state == libvlc_Playing) {
    rval = 1;
  }
  return rval;
}

int
vlcIsPaused (vlcData_t *vlcData)
{
  int       rval;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  rval = 0;
  if (vlcData->state == libvlc_Paused) {
    rval = 1;
  }
  return rval;
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

bool
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

int
vlcAudioDevSet (vlcData_t *vlcData, const char *dev)
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

#if _lib_libvlc_audio_output_device_enum

int
vlcAudioDevList (vlcData_t *vlcData, volsinklist_t *sinklist)
{
  libvlc_audio_output_device_t  *adevlist;
  libvlc_audio_output_device_t  *adevlistptr;
  int                           count = 0;

  sinklist->defname = NULL;
  sinklist->sinklist = NULL;
  sinklist->count = 0;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL ||
      strcmp (vlcData->version, "2.2.0") < 0) {
    return 0;
  }

  adevlist = libvlc_audio_output_device_enum (vlcData->mp);
  adevlistptr = adevlist;
  while (adevlistptr != (libvlc_audio_output_device_t *) NULL) {
    ++sinklist->count;
    sinklist->sinklist = mdrealloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));
    sinklist->sinklist [count].defaultFlag = 0;
    sinklist->sinklist [count].idxNumber = count;
    sinklist->sinklist [count].name = mdstrdup (adevlistptr->psz_device);
    sinklist->sinklist [count].description = mdstrdup (adevlistptr->psz_description);
    if (count == 0) {
      sinklist->defname = mdstrdup (adevlistptr->psz_device);
    }
    ++count;
    adevlistptr = adevlistptr->p_next;
  }

  libvlc_audio_output_device_list_release (adevlist);
  return 0;
}

#endif /* have libvlc_audio_output_device_enum */

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

  switch (vlcData->state) {
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
  struct stat             statbuf;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  if (stat (fn, &statbuf) != 0) {
    return -1;
  }

  if (vlcData->m != NULL) {
    em = libvlc_media_event_manager (vlcData->m);
    libvlc_event_detach (em, libvlc_MediaStateChanged,
        &vlcEventHandler, vlcData);
    libvlc_media_release (vlcData->m);
    vlcData->m = NULL;
  }

  vlcData->m = libvlc_media_new_path (vlcData->inst, fn);
  libvlc_media_player_set_rate (vlcData->mp, 1.0);
  libvlc_media_player_set_media (vlcData->mp, vlcData->m);

  em = libvlc_media_event_manager (vlcData->m);
  libvlc_event_attach (em, libvlc_MediaStateChanged,
      &vlcEventHandler, vlcData);

#if ! defined(VLC_NO_DEVLIST)
  /* on mac os x, the device has to be set after the media is set */
  if (vlcData->device != NULL) {
    libvlc_audio_output_device_set (vlcData->mp, NULL, vlcData->device);
  }
#endif
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
  vlcData->m = NULL;
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
  }
#if SILENCE_LOG
  libvlc_log_set (vlcData->inst, silence, NULL);
#endif

  if (vlcData->inst != NULL && vlcData->mp == NULL) {
    vlcData->mp = libvlc_media_player_new (vlcData->inst);
  }

  if (vlcData->inst != NULL && vlcData->mp != NULL) {
    libvlc_audio_set_volume (vlcData->mp, 100);
#if ! defined(VLC_NO_ROLE) && \
      LIBVLC_VERSION_INT >= LIBVLC_VERSION(3,0,0,0)
    libvlc_media_player_set_role (vlcData->mp, libvlc_role_Music);
#endif
  }

  return vlcData;
}

void
vlcClose (vlcData_t *vlcData)
{
  libvlc_event_manager_t  *em;
  int                     i;

  if (vlcData != NULL) {
    if (vlcData->m != NULL) {
      em = libvlc_media_event_manager (vlcData->m);
      libvlc_event_detach (em, libvlc_MediaStateChanged,
          &vlcEventHandler, vlcData);
      libvlc_media_release (vlcData->m);
      vlcData->m = NULL;
    }
    if (vlcData->mp != NULL) {
      libvlc_media_player_stop (vlcData->mp);
      libvlc_media_player_release (vlcData->mp);
      vlcData->mp = NULL;
    }
    if (vlcData->inst != NULL) {
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

/* for debugging */
#if 0
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
#endif

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
