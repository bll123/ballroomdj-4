/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _lib_libvlc3_new || _lib_libvlc4_new

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

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pli.h"
#include "vlci.h"

#define VLCDEBUG 0
#define SILENCE_LOG 1

typedef struct vlcdata {
  libvlc_instance_t       *inst;
  char                    version [40];
  libvlc_media_t          *media;
  libvlc_media_player_t   *mp;
  libvlc_state_t          state;
  int                     argc;
  char                    **argv;
  char                    *device;
  int                     devtype;
  FILE                    *logfh;
} vlcdata_t;

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
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
  { libvlc_Ended,           "ended" },
#endif
  { libvlc_Error,           "error" }
};
enum {
  stateMapMax = (sizeof (stateMap) / sizeof (stateMap_t))
};

static const char *stateToStr (libvlc_state_t state); /* for debugging */

# endif /* VLCDEBUG */

static bool vlcHaveAudioDevList (void);
static void vlcReleaseMedia (vlcdata_t *vlcdata);
static void vlcSetAudioOutput (vlcdata_t *vlcdata);
static void vlcCreateNewMediaPlayer (vlcdata_t *vlcdata);
static void vlcSetPosition (vlcdata_t *vlcdata, double dpos);

#if VLCDEBUG
static void vlclog (vlcdata_t *vlcdata, const char *msg, ...);
static const char * stateToStr (libvlc_state_t state);
#else
# define vlclog(vlcdata,msg,...)
#endif
#if SILENCE_LOG
static void silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args);
#endif

/* get media values */

ssize_t
vlcGetDuration (vlcdata_t *vlcdata)
{
  libvlc_time_t     tm;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  tm = libvlc_media_player_get_length (vlcdata->mp);
  vlclog (vlcdata, "get-dur: %zd\n", tm);
  return tm;
}

ssize_t
vlcGetTime (vlcdata_t *vlcdata)
{
  libvlc_time_t     tm;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  tm = libvlc_media_player_get_time (vlcdata->mp);
  vlclog (vlcdata, "get-tm: %zd\n", tm);
  return tm;
}

/* media commands */

int
vlcStop (vlcdata_t *vlcdata)
{
  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlclog (vlcdata, "stop\n");
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
  libvlc_media_player_stop (vlcdata->mp);
#endif
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4,0,0,0)
  libvlc_media_player_pause (vlcdata->mp);
#endif

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);
  return 0;
}

int
vlcPause (vlcdata_t *vlcdata)
{
  int   rc;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  vlclog (vlcdata, "pause state %d/%s\n", vlcdata->state, stateToStr (vlcdata->state));
  rc = 1;
  if (vlcdata->state == libvlc_Playing) {
    libvlc_media_player_set_pause (vlcdata->mp, 1);
    rc = 0;
  } else if (vlcdata->state == libvlc_Paused) {
    libvlc_media_player_set_pause (vlcdata->mp, 0);
    rc = 0;
  }
  return rc;
}

int
vlcPlay (vlcdata_t *vlcdata)
{
  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  vlclog (vlcdata, "play\n");
  libvlc_media_player_play (vlcdata->mp);
  return 0;
}

ssize_t
vlcSeek (vlcdata_t *vlcdata, ssize_t pos)
{
  libvlc_time_t     dur = 0;
  double            dpos = 0.0;
  ssize_t           newpos = pos;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  vlclog (vlcdata, "seek\n");
  if ((vlcdata->state == libvlc_Playing ||
      vlcdata->state == libvlc_Paused) &&
      pos >= 0) {
    dur = libvlc_media_player_get_length (vlcdata->mp);
    dpos = (double) pos / (double) dur;
    vlcSetPosition (vlcdata, dpos);
  }
  dpos = libvlc_media_player_get_position (vlcdata->mp);
  newpos = (ssize_t) round (dpos * (double) dur);
  vlclog (vlcdata, "vlci: seek: dpos: %.6f new-pos: %.6f new-pos: %" PRId64 "\n", dpos, pos, newpos);

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  return newpos;
}

double
vlcRate (vlcdata_t *vlcdata, double drate)
{
  float     rate;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  vlclog (vlcdata, "rate\n");
  if (vlcdata->state == libvlc_Playing) {
    rate = (float) drate;
    libvlc_media_player_set_rate (vlcdata->mp, rate);
  }
  rate = libvlc_media_player_get_rate (vlcdata->mp);
  return (double) rate;
}

/* other commands */

int
vlcSetAudioDev (vlcdata_t *vlcdata, const char *dev, int plidevtype)
{
  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return -1;
  }

  vlclog (vlcdata, "set-audio-dev: %d %s\n", plidevtype, dev);
  vlcdata->devtype = plidevtype;
  dataFree (vlcdata->device);
  vlcdata->device = NULL;
  if (dev != NULL && strlen (dev) > 0) {
    vlcdata->device = mdstrdup (dev);
  }

  return 0;
}

const char *
vlcVersion (vlcdata_t *vlcdata)   /* KEEP */
{
  return vlcdata->version;
}

plistate_t
vlcState (vlcdata_t *vlcdata)
{
  plistate_t  state = PLI_STATE_NONE;

  if (vlcdata == NULL || vlcdata->inst == NULL || vlcdata->mp == NULL) {
    return state;
  }

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  switch ((int) vlcdata->state) {
    case libvlc_NothingSpecial: { state = PLI_STATE_IDLE; break; }
    case libvlc_Opening: { state = PLI_STATE_OPENING; break; }
    case libvlc_Buffering: { state = PLI_STATE_BUFFERING; break; }
    case libvlc_Playing: { state = PLI_STATE_PLAYING; break; }
    case libvlc_Paused: { state = PLI_STATE_PAUSED; break; }
    case libvlc_Stopped: { state = PLI_STATE_STOPPED; break; }
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
    case libvlc_Ended: { state = PLI_STATE_ENDED; break; }
#endif
    case libvlc_Error: { state = PLI_STATE_ERROR; break; }
  }
  return state;
}

/* media commands */

int
vlcMedia (vlcdata_t *vlcdata, const char *fn)
{
  if (vlcdata == NULL || vlcdata->inst == NULL) {
    return -1;
  }

  if (! fileopFileExists (fn)) {
    return -1;
  }

  vlclog (vlcdata, "media: %s\n", fn);
  vlcReleaseMedia (vlcdata);

#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
  vlcdata->media = libvlc_media_new_path (vlcdata->inst, fn);
#endif
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4,0,0,0)
  vlcdata->media = libvlc_media_new_path (fn);
#endif
  mdextalloc (vlcdata->media);

#if _WIN32
  /* on windows, when using a selected sink, */
  /* if the default windows audio sink is changed, a new media */
  /* player needs to be created for each medium. */
  /* if this is not here, windows will */
  /* switch audio sinks if the default is changed even though the */
  /* vlc-audio-output is set and re-set. */
  /* this call creates extra latency, so only do it if necessary */
  if (vlcdata->devtype == PLI_SELECTED_DEV) {
    vlcCreateNewMediaPlayer (vlcdata);
    if (vlcdata->mp == NULL) {
      return -1;
    }
  }
#endif
  libvlc_media_player_set_rate (vlcdata->mp, 1.0);
  libvlc_media_player_set_media (vlcdata->mp, vlcdata->media);

  vlcSetAudioOutput (vlcdata);

  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);

  return 0;
}

/* initialization and cleanup */

vlcdata_t *
vlcInit (int vlcargc, char *vlcargv [], char *vlcopt [])
{
  vlcdata_t *   vlcdata;
  char *        tptr;
  char *        nptr;
  int           i;
  int           j;

  vlcdata = (vlcdata_t *) mdmalloc (sizeof (vlcdata_t));
  vlcdata->inst = NULL;
  vlcdata->media = NULL;
  vlcdata->mp = NULL;
  vlcdata->argv = NULL;
  vlcdata->state = libvlc_NothingSpecial;
  vlcdata->device = NULL;
  vlcdata->devtype = PLI_DEFAULT_DEV;
  vlcdata->logfh = NULL;

#if VLCDEBUG
  vlcdata->logfh = fopen ("vlc-out.txt", "a");
#endif

  vlcdata->argv = (char **) mdmalloc (sizeof (char *) * (size_t) (vlcargc + 1));

  for (i = 0; i < vlcargc; ++i) {
    nptr = mdstrdup (vlcargv [i]);
    vlcdata->argv [i] = nptr;
  }

  j = 0;
  while ((tptr = vlcopt [j]) != NULL) {
    ++vlcargc;
    vlcdata->argv = (char **) mdrealloc (vlcdata->argv,
        sizeof (char *) * (size_t) (vlcargc + 1));
    nptr = mdstrdup (tptr);
    vlcdata->argv [i] = nptr;
    ++i;
    ++j;
  }

  vlcdata->argc = vlcargc;
  vlcdata->argv [vlcdata->argc] = NULL;

  stpecpy (vlcdata->version,
      vlcdata->version + sizeof (vlcdata->version), libvlc_get_version ());
  tptr = strchr (vlcdata->version, ' ');
  if (tptr != NULL) {
    *tptr = '\0';
  }

  if (vlcdata->inst == NULL) {
    vlcdata->inst = libvlc_new (vlcdata->argc, (const char * const *) vlcdata->argv);
    mdextalloc (vlcdata->inst);
  }
#if SILENCE_LOG
  libvlc_log_set (vlcdata->inst, silence, NULL);
#endif

  /* windows seems to need this, even though the media player is going */
  /* to be released and re-created per medium. */
  vlcCreateNewMediaPlayer (vlcdata);

  return vlcdata;
}

void
vlcClose (vlcdata_t *vlcdata)
{
  int                     i;

  if (vlcdata != NULL) {
    if (vlcdata->logfh != NULL) {
      fclose (vlcdata->logfh);
    }
    vlcReleaseMedia (vlcdata);
    if (vlcdata->mp != NULL) {
      vlcStop (vlcdata);
      mdextfree (vlcdata->mp);
      libvlc_media_player_release (vlcdata->mp);
      vlcdata->mp = NULL;
    }
    if (vlcdata->inst != NULL) {
      mdextfree (vlcdata->inst);
      libvlc_release (vlcdata->inst);
      vlcdata->inst = NULL;
    }
    if (vlcdata->argv != NULL) {
      for (i = 0; i < vlcdata->argc; ++i) {
        dataFree (vlcdata->argv [i]);
      }
      mdfree (vlcdata->argv);
      vlcdata->argv = NULL;
    }
    dataFree (vlcdata->device);
    vlcdata->device = NULL;
    vlcdata->state = libvlc_NothingSpecial;
    mdfree (vlcdata);
  }
}


/* internal routines */

static bool
vlcHaveAudioDevList (void)
{
  return true;
}

static void
vlcReleaseMedia (vlcdata_t *vlcdata)
{
  if (vlcdata == NULL) {
    return;
  }

  if (vlcdata->media != NULL) {
    mdextfree (vlcdata->media);
    libvlc_media_release (vlcdata->media);
    vlcdata->media = NULL;
  }
}

static void
vlcSetAudioOutput (vlcdata_t *vlcdata)
{
  /* Does not work on linux. 3.x documentation says pulseaudio does not */
  /* have a device parameter. pulseaudio uses the PULSE_SINK env var. */
  /* macos: need to call this to switch audio devices on macos. */
  /* windows: seems to need this for setup and switching. */

  if (vlcHaveAudioDevList ()) {
    if (vlcdata->device != NULL) {
      const char  *carg = vlcdata->device;

      {
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
        libvlc_audio_output_device_set (vlcdata->mp, NULL, carg);
#endif
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4,0,0,0)
        libvlc_audio_output_device_set (vlcdata->mp, carg);
#endif
      }
    }
  }
}

static void
vlcCreateNewMediaPlayer (vlcdata_t *vlcdata)
{
  if (vlcdata->inst == NULL) {
    return;
  }

  if (vlcdata->mp != NULL) {
    mdextfree (vlcdata->mp);
    libvlc_media_player_release (vlcdata->mp);
  }
  vlcdata->mp = libvlc_media_player_new (vlcdata->inst);
  mdextalloc (vlcdata->mp);

  if (vlcdata->inst != NULL && vlcdata->mp != NULL) {
    libvlc_audio_set_volume (vlcdata->mp, 100);
#if ! defined(VLC_NO_ROLE) && \
      LIBVLC_VERSION_INT >= LIBVLC_VERSION(3,0,0,0)
    libvlc_media_player_set_role (vlcdata->mp, libvlc_role_Music);
#endif
  }
}

static void
vlcSetPosition (vlcdata_t *vlcdata, double dpos)
{
  float   fpos;

  vlclog (vlcdata, "set-pos %.2f\n", dpos);
  fpos = (float) dpos;
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4,0,0,0)
  libvlc_media_player_set_position (vlcdata->mp, fpos);
#endif
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4,0,0,0)
  libvlc_media_player_set_position (vlcdata->mp, fpos, false);
#endif
  vlcdata->state = libvlc_media_player_get_state (vlcdata->mp);
}

/* tests if the interface was linked with the correct libvlc version */
bool
vlcVersionLinkCheck (void)
{
  bool    valid;

  valid = (LIBVLC_VERSION_MAJOR == BDJ4_VLC_VERS);
  return valid;
}

/* tests if the loaded version matches the wanted version */
bool
vlcVersionCheck (void)
{
  bool    valid;
  int     vers;

  vers = atoi (libvlc_get_version());
  valid = BDJ4_VLC_VERS == vers;
  return valid;
}

/* for debugging */

int
vlcGetVolume (vlcdata_t *vlcdata)
{
  int     val;

  val = libvlc_audio_get_volume (vlcdata->mp);
  return val;
}

#if VLCDEBUG

static void
vlclog (vlcdata_t *vlcdata, const char *msg, ...)
{
  va_list   args;

  if (vlcdata->logfh != NULL) {
    va_start (args, msg);
    vfprintf (vlcdata->logfh, msg, args);
    va_end (args);
  }
}

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

#if SILENCE_LOG
static void
silence (void *data, int level, const libvlc_log_t *ctx,
    const char *fmt, va_list args)
{
  return;
}
#endif

#endif /* libvlc_new() */
