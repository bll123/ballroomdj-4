
#define SILENCE_LOG       0 /* vlc */
#define STATE_TO_VALUE    0 /* vlc */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#if _hdr_windows
# include <windows.h>
#endif

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "vlci.h"
#include "list.h"
#include "bdjstring.h"

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
#define stateMapMax (sizeof(stateMap)/sizeof(stateMap_t))

static const char *   stateToStr (libvlc_state_t state);
#if STATE_TO_VALUE
static libvlc_state_t stateToValue (char *name);
#endif
#if SILENCE_LOG
static void silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args)
#endif

/* get media values */

double
vlcGetDuration (vlcData_t *vlcData)
{
  libvlc_time_t     tm;
  double            dtm;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1.0;
  }

  tm = libvlc_media_player_get_length (vlcData->mp);
  dtm = (double) tm / 1000.0;
  return dtm;
}

double
vlcGetTime (vlcData_t *vlcData)
{
  libvlc_time_t     tm;
  double            dtm;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1.0;
  }

  tm = libvlc_media_player_get_time (vlcData->mp);
  dtm = (double) tm / 1000.0;
  return dtm;
}

/* media checks */

int
vlcIsPlaying (vlcData_t *vlcData)
{
  int       rval;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  rval = 0;
  /*
   * VLC's internal isplaying command returns true if the player is paused.
   * Our implementation is different.
   */
  if (vlcData->state == libvlc_Opening ||
      vlcData->state == libvlc_Buffering ||
      vlcData->state == libvlc_Playing) {
    rval = 1;
  }
  return 0;
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

double
vlcSeek (vlcData_t *vlcData, double dpos)
{
  libvlc_time_t     tm;
  float             pos;
  double            dtm;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1.0;
  }

  if (vlcData->state == libvlc_Playing) {
    tm = libvlc_media_player_get_length (vlcData->mp);
    dtm = (double) tm / 1000.0;
    dpos = dpos / dtm;
    pos = (float) dpos;
    libvlc_media_player_set_position (vlcData->mp, pos);
  }
  pos = libvlc_media_player_get_position (vlcData->mp);
  return (double) pos;
}

/* other commands */

int
vlcHaveAudioDevList (void)
{
  int       rc;

  rc = 0;
#if _lib_libvlc_audio_output_device_enum && \
    LIBVLC_VERSION_INT >= LIBVLC_VERSION(2,2,0,0)
  rc = 1;
#endif
  return rc;
}

int
vlcAudioDevSet (vlcData_t *vlcData, char *dev)
{
  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  if (vlcData->device != NULL) {
    free (vlcData->device);
  }
  vlcData->device = NULL;
  if (strlen (dev) > 0) {
    vlcData->device = strdup (dev);
  }

  return 0;
}

#if _lib_libvlc_audio_output_device_enum

list_t *
vlcAudioDevList (vlcData_t *vlcData)
{
  libvlc_audio_output_device_t  *adevlist;
  libvlc_audio_output_device_t  *adevlistptr;
  list_t                        *devlist;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL ||
      strcmp (vlcData->version, "2.2.0") < 0) {
    return NULL;
  }

  devlist = slistAlloc (LIST_UNORDERED, stringCompare, free, free);

  adevlist = libvlc_audio_output_device_enum (vlcData->mp);
  adevlistptr = adevlist;
  while (adevlistptr != (libvlc_audio_output_device_t *) NULL) {
    slistSetData (devlist, adevlistptr->psz_device,
        strdup (adevlistptr->psz_description));
    adevlistptr = adevlistptr->p_next;
  }

  libvlc_audio_output_device_list_release (adevlist);
  return devlist;
}

#endif /* have libvlc_audio_output_device_enum */

char *
vlcVersion (vlcData_t *vlcData)
{
  return vlcData->version;
}

const char *
vlcState (vlcData_t *vlcData)
{
  libvlc_state_t    plstate;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return NULL;
  }

  plstate = vlcData->state;
  return (stateToStr(plstate));
}

void
noInitialVolume (vlcData_t *vlcData)
{
  if (vlcData == NULL) {
    return;
  }

  vlcData->initialvolume = 0;
}

/* media commands */

int
vlcMedia (vlcData_t *vlcData, char *fn)
{
  libvlc_media_t            *m;
  libvlc_event_manager_t    *em;
  struct stat               statbuf;

  if (vlcData == NULL || vlcData->inst == NULL || vlcData->mp == NULL) {
    return -1;
  }

  if (stat (fn, &statbuf) != 0) {
    return -1;
  }

  m = libvlc_media_new_path (vlcData->inst, fn);
  libvlc_media_player_set_rate (vlcData->mp, 1.0);
  em = libvlc_media_event_manager (m);
  libvlc_event_attach (em, libvlc_MediaStateChanged,
      &vlcEventHandler, vlcData);
  libvlc_media_player_set_media (vlcData->mp, m);
#if ! defined(VLC_NO_DEVLIST)
  /* on mac os x, the device has to be set after the media is set */
  if (vlcData->device != NULL) {
    libvlc_audio_output_device_set (vlcData->mp, NULL, vlcData->device);
  }
#endif
  libvlc_media_release (m);
  return 0;
}

/* initialization and cleanup */

vlcData_t *
vlcInit (int vlcargc, char *vlcargv[])
{
  vlcData_t *   vlcData;
  char *        tptr;
  char *        nptr;
  int           rc;
  int           i;

  rc = 1;

  vlcData = (vlcData_t *) malloc (sizeof (vlcData_t));
  assert (vlcData != NULL);
  vlcData->inst = NULL;
  vlcData->mp = NULL;
  vlcData->argv = NULL;
  vlcData->state = libvlc_NothingSpecial;
  vlcData->device = NULL;
  vlcData->initialvolume = 1;

  vlcData->argv = (char **) malloc (sizeof (char *) * (size_t) (vlcargc + 1));
  assert (vlcData->argv != NULL);

  for (i = 0; i < vlcargc; ++i) {
    nptr = strdup (vlcargv [i]);
    vlcData->argv [i] = nptr;
  }
  vlcData->argc = vlcargc;
  vlcData->argv [vlcData->argc] = NULL;

  strlcpy (vlcData->version, libvlc_get_version (), sizeof (vlcData->version));
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
    if (vlcData->initialvolume) {
      libvlc_audio_set_volume (vlcData->mp, 100);
    }
#if ! defined(VLC_NO_ROLE) && \
      LIBVLC_VERSION_INT >= LIBVLC_VERSION(3,0,0,0)
    libvlc_media_player_set_role (vlcData->mp, libvlc_role_Music);
#endif
    rc = 0;
  }

  return vlcData;
}

void
vlcClose (vlcData_t *vlcData)
{
  int   i;

  if (vlcData != NULL) {
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
        free (vlcData->argv [i]);
      }
      free (vlcData->argv);
      vlcData->argv = NULL;
    }
    if (vlcData->device != NULL) {
      free (vlcData->device);
      vlcData->device = NULL;
    }
    vlcData->state = libvlc_NothingSpecial;
    free (vlcData);
  }
}

void
vlcRelease (vlcData_t *vlcData)
{
  vlcClose (vlcData);
}

/* event handlers */

void
vlcExitHandler (vlcData_t *vlcData)
{
  vlcClose (vlcData);
}

void
vlcEventHandler (const struct libvlc_event_t *event, void *userdata)
{
  vlcData_t     *vlcData = (vlcData_t *) userdata;

  if (event->type == libvlc_MediaStateChanged) {
    vlcData->state = (libvlc_state_t) event->u.media_state_changed.new_state;
  }
}

/* internal routines */

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

#if STATE_TO_VALUE

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

#endif

#if SILENCE_LOG
static void
silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args)
{
}
#endif

