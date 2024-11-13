/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * gst-launch-1.0 playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3
 *
 * original version:
 * gst-launch-1.0 playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 * modifying a playbin pipeline:
 * https://gstreamer.freedesktop.org/documentation/tutorials/playback/custom-playbin-sinks.html?gi-language=c
 *
 * working mix:
 * gst-launch-1.0 \
 *   uridecodebin3 uri=file://$HOME/s/bdj4/test-music/003-chacha.mp3 \
 *   ! audioconvert ! scaletempo ! audioconvert ! audioresample \
 *   ! volume volume="0.3" ! autoaudiosink \
 *   uridecodebin3 uri=file://$HOME/s/bdj4/test-music/003-jive.mp3 \
 *   ! audioconvert ! scaletempo ! audioconvert ! audioresample \
 *   ! volume volume="0.3" ! autoaudiosink \
 *
 * export GST_DEBUG_DUMP_DOT_DIR=/home/bll/s/bdj4/tmp
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#if _hdr_gst_gst

# include <gst/gst.h>
# include <glib.h>

# include "audiosrc.h"     // for audio-source type
# include "bdj4.h"
# include "bdjstring.h"
# include "log.h"
# include "mdebug.h"
# include "gsti.h"
# include "pli.h"

# define GSTI_DEBUG 0
# define GSTI_DEBUG_DOT 0

# define GSTI_NO_VOL 0.0
# define GSTI_FULL_VOL 1.0
# define GSTI_NORM_RATE 1.0

enum {
  GSTI_IDENT = 0xccbbaa0069747367,
};

static const char *GSTI_PIPELINE_NAME = "pipeline";
static const char *GSTI_SRCBIN_PFX = "srcbin";
#define GSTI_SRCBIN_PFX_LEN (strlen (GSTI_SRCBIN_PFX))

enum {
  GSTI_MAX_SOURCE = 2,
};

typedef struct gsti {
  uint64_t          ident;
  GMainContext      *mainctx;
  GstElement        *pipeline;
  GstElement        *currsource;
  GstElement        *srcbin [GSTI_MAX_SOURCE];
  GstElement        *source [GSTI_MAX_SOURCE];
  GstElement        *deccvt [GSTI_MAX_SOURCE];
  GstElement        *volume [GSTI_MAX_SOURCE];
  GstState          gststate;
  guint             busId;
  plistate_t        plistate;
  double            vol [GSTI_MAX_SOURCE];
  double            rate [GSTI_MAX_SOURCE];
  int               curr;
  bool              isstopping : 1;
  bool              incrossfade : 1;
  bool              async : 1;
  bool              inrunonce : 1;
} gsti_t;

static bool initialized = false;

static void gstiRunOnce (gsti_t *gsti);
static gboolean gstiBusCallback (GstBus * bus, GstMessage * message, void *udata);
static void gstiProcessState (gsti_t *gsti, GstState state, GstState pending);
static void gstiWaitState (gsti_t *gsti, GstElement *elem, GstState want);
static void gstiMakeSource (gsti_t *gsti, int idx);
static void gstiAddSourceToPipeline (gsti_t *gsti, int idx);
// static void gstiRemoveSourceFromPipeline (gsti_t *gsti, int idx);
static void gstiDynamicLinkPad (GstElement *src, GstPad *newpad, gpointer udata);
static void gstiChangeVolume (gsti_t *gsti, int curr);
static void gstiEndCrossFade (gsti_t *gsti);
# if GSTI_DEBUG_DOT
static void gstiDebugDot (gsti_t *gsti, const char *fn);
# endif

gsti_t *
gstiInit (const char *plinm)
{
  gsti_t            *gsti;
  GstBus            *bus;
  char              *tmp;

  if (! initialized) {
    gst_init (NULL, 0);
    initialized = true;
  }

  tmp = gst_version_string ();
  mdextalloc (tmp);
  logMsg (LOG_DBG, LOG_IMPORTANT, "gst: version %s", tmp);
  mdextfree (tmp);
  free (tmp);

  gsti = mdmalloc (sizeof (gsti_t));
  gsti->ident = GSTI_IDENT;
  gsti->pipeline = NULL;
  gsti->currsource = NULL;
  gsti->busId = 0;
  gsti->plistate = PLI_STATE_IDLE;
  for (int i = 0; i < GSTI_MAX_SOURCE; ++i) {
    gsti->srcbin [i] = NULL;
    gsti->source [i] = NULL;
    gsti->deccvt [i] = NULL;
    gsti->volume [i] = NULL;
    gsti->rate [i] = GSTI_NORM_RATE;
    gsti->vol [i] = GSTI_NO_VOL;
  }
  gsti->curr = 0;
  gsti->isstopping = false;
  gsti->incrossfade = false;
  gsti->async = false;
  gsti->inrunonce = false;

  gsti->mainctx = g_main_context_default ();
  gstiRunOnce (gsti);

  gsti->pipeline = gst_pipeline_new (GSTI_PIPELINE_NAME);
  mdextalloc (gsti->pipeline);
  gstiRunOnce (gsti);

  gsti->vol [gsti->curr] = GSTI_FULL_VOL;
  gstiMakeSource (gsti, gsti->curr);

  /* make both source bins */
  gsti->vol [1] = GSTI_NO_VOL;
  gstiMakeSource (gsti, 1);

  gstiAddSourceToPipeline (gsti, gsti->curr);
  gsti->currsource = gsti->source [gsti->curr];

  bus = gst_pipeline_get_bus (GST_PIPELINE (gsti->pipeline));
  gsti->busId = gst_bus_add_watch (bus, gstiBusCallback, gsti);
  g_object_unref (bus);
  gstiRunOnce (gsti);

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-init");
# endif

  return gsti;
}

void
gstiFree (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT) {
    return;
  }

  gsti->ident = BDJ4_IDENT_FREE;

  gstiRunOnce (gsti);

  if (gsti->pipeline != NULL) {
    gsti->isstopping = true;

    for (int i = 0; i < GSTI_MAX_SOURCE; ++i) {
      gsti->vol [i] = GSTI_NO_VOL;
      gstiChangeVolume (gsti, i);
    }

    gst_element_set_state (gsti->pipeline, GST_STATE_READY);
    gstiWaitState (gsti, gsti->pipeline, GST_STATE_READY);

    gst_element_set_state (gsti->pipeline, GST_STATE_NULL);
    gstiWaitState (gsti, gsti->pipeline, GST_STATE_NULL);

    mdextfree (gsti->pipeline);
    gst_object_unref (gsti->pipeline);
  }

  gstiRunOnce (gsti);

  mdfree (gsti);
}

void
gstiCleanup (void)
{
  if (initialized) {
    gst_deinit ();
    initialized = false;
  }

  return;
}

void
gstiMedia (gsti_t *gsti, const char *fulluri, int sourceType)
{
  int     rc;
  char    tbuff [MAXPATHLEN];

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);

  if (sourceType == AUDIOSRC_TYPE_FILE) {
    snprintf (tbuff, sizeof (tbuff), "%s%s", AS_FILE_PFX, fulluri);
  } else {
    stpecpy (tbuff, tbuff + sizeof (tbuff), fulluri);
  }

# if GSTI_DEBUG
  logStderr ("-- set std uri: %d %s\n", gsti->curr, tbuff);
# endif

  gsti->rate [gsti->curr] = GSTI_NORM_RATE;
  g_object_set (G_OBJECT (gsti->currsource), "uri", tbuff, NULL);
  rc = gst_element_set_state (gsti->srcbin [gsti->curr], GST_STATE_PAUSED);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (media)\n");
  }
  if (rc == GST_STATE_CHANGE_ASYNC) {
logStderr ("    in-async\n");
    gsti->async = true;
  }
  gstiRunOnce (gsti);
  gsti->plistate = PLI_STATE_OPENING;
logStderr ("gsti: media: == opening\n");
  gstiRunOnce (gsti);
# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-media");
# endif
  return;
}

void
gstiCrossFade (gsti_t *gsti, const char *fulluri, int sourceType)
{
  char    tbuff [MAXPATHLEN];
  int     idx;
  int     rc;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

logStderr ("-- gsti: crossfade\n");

  if (gsti->plistate != PLI_STATE_PLAYING) {
logStderr ("-- not playing: pli-state: %d\n", gsti->plistate);
    gstiMedia (gsti, fulluri, sourceType);
    return;
  }

  gstiRunOnce (gsti);

  idx = 1 - gsti->curr;

  if (sourceType == AUDIOSRC_TYPE_FILE) {
    snprintf (tbuff, sizeof (tbuff), "%s%s", AS_FILE_PFX, fulluri);
  } else {
    stpecpy (tbuff, tbuff + sizeof (tbuff), fulluri);
  }

# if GSTI_DEBUG
  logStderr ("-- set xfade uri: %d %s\n", idx, tbuff);
# endif

  /* add the bin to the pipeline */
  gstiAddSourceToPipeline (gsti, idx);

  gsti->rate [idx] = GSTI_NORM_RATE;
  /* start with zero volume */
  gsti->vol [idx] = GSTI_NO_VOL;
  gstiChangeVolume (gsti, idx);
  gstiRunOnce (gsti);

logStderr ("gsti: start crossfade %s\n", tbuff);
  g_object_set (G_OBJECT (gsti->source [idx]), "uri", tbuff, NULL);
  rc = gst_element_set_state (gsti->srcbin [idx], GST_STATE_PLAYING);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (crossfade)\n");
  }
  if (rc == GST_STATE_CHANGE_ASYNC) {
logStderr ("    in-async\n");
    gsti->async = true;
  }
  gstiRunOnce (gsti);

logStderr ("gsti: new current %d\n", idx);
  gsti->curr = idx;
  gsti->currsource = gsti->source [gsti->curr];
  gsti->incrossfade = true;
  gstiRunOnce (gsti);

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-crossfade");
# endif
  return;
}

void
gstiCrossFadeVolume (gsti_t *gsti, int vol)
{
  double      dvol;
  int         idx;

  if (gsti->incrossfade == false) {
    return;
  }

  /* current is pointing at the new song */
  idx = 1 - gsti->curr;
  dvol = (double) vol / 100.0;

  gsti->vol [idx] = dvol;
  gstiChangeVolume (gsti, idx);
  gstiRunOnce (gsti);

logStderr ("-- xfade vol %d: %.2f / %d: %.2f\n", idx, dvol, gsti->curr, 1.0 - dvol);
  if (dvol <= 0.0) {
    gstiEndCrossFade (gsti);
  } else {
    dvol = 1.0 - dvol;
    gsti->vol [gsti->curr] = dvol;
    gstiChangeVolume (gsti, gsti->curr);
    gstiRunOnce (gsti);
  }

  return;
}

int64_t
gstiGetDuration (gsti_t *gsti)
{
  gint64      ctm;
  int64_t     tm = 0;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return 0;
  }

  gstiRunOnce (gsti);
  if (! gst_element_query_duration (gsti->srcbin [gsti->curr], GST_FORMAT_TIME, &ctm)) {
    return 0;
  }

  tm = ctm / 1000 / 1000;
  return tm;
}

int64_t
gstiGetPosition (gsti_t *gsti)
{
  gint64      ctm;
  int64_t     tm = 0;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return 0;
  }

  gstiRunOnce (gsti);
  if (! gst_element_query_position (gsti->srcbin [gsti->curr],
      GST_FORMAT_TIME, &ctm)) {
    return 0;
  }

  tm = ctm / 1000 / 1000;
  return tm;
}

plistate_t
gstiState (gsti_t *gsti)
{
  GstState    state, pending;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return PLI_STATE_NONE;
  }

  gstiRunOnce (gsti);

  gst_element_get_state (gsti->srcbin [gsti->curr], &state, &pending, 1);
  if (gsti->gststate != state) {
# if GSTI_DEBUG
    char    tmp [80];

logStderr ("gsti-state: curr: %d/%s pending: %d/%s\n",
state, gst_element_state_get_name (state),
pending, gst_element_state_get_name (pending));
    snprintf (tmp, sizeof (tmp), "gsti-state_%d", state);
#  if GSTI_DEBUG_DOT
    gstiDebugDot (gsti, tmp);
#  endif
# endif
    gsti->gststate = state;
  }

  gstiProcessState (gsti, state, pending);

  return gsti->plistate;
}

void
gstiPause (gsti_t *gsti)
{
  int     rc;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
# if GSTI_DEBUG
  logStderr ("-- pause\n");
# endif
  rc = gst_element_set_state (gsti->srcbin [gsti->curr], GST_STATE_PAUSED);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (pause)\n");
  }
  if (rc == GST_STATE_CHANGE_ASYNC) {
logStderr ("    in-async\n");
    gsti->async = true;
  }
  gstiRunOnce (gsti);
# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-pause");
# endif
  return;
}

void
gstiPlay (gsti_t *gsti)
{
  int     rc;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
# if GSTI_DEBUG
  logStderr ("-- play\n");
# endif
  /* the entire pipeline must be change to state playing */
  rc = gst_element_set_state (gsti->pipeline, GST_STATE_PLAYING);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (play)\n");
  }
  if (rc == GST_STATE_CHANGE_ASYNC) {
logStderr ("    in-async\n");
    gsti->async = true;
  }
  gstiRunOnce (gsti);
# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-play");
# endif
  return;
}

void
gstiStop (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

# if GSTI_DEBUG
  logStderr ("-- stop\n");
# endif
  gsti->isstopping = true;

  /* a stop might be received during a cross-fade */
  for (int i = 0; i < GSTI_MAX_SOURCE; ++i) {
    gsti->vol [i] = GSTI_NO_VOL;
    gstiChangeVolume (gsti, i);
  }
  gstiRunOnce (gsti);

  gst_element_set_state (gsti->pipeline, GST_STATE_READY);
  gstiWaitState (gsti, gsti->pipeline, GST_STATE_READY);

  for (int i = 0; i < GSTI_MAX_SOURCE; ++i) {
    gsti->vol [i] = GSTI_FULL_VOL;
    gstiChangeVolume (gsti, i);
  }
  gstiRunOnce (gsti);

  gsti->isstopping = false;

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-stop");
# endif
  return;
}

bool
gstiSetPosition (gsti_t *gsti, int64_t pos)
{
  int       rc = false;
  gint64    gpos;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  if (gsti->plistate == PLI_STATE_PAUSED ||
      gsti->plistate == PLI_STATE_PLAYING) {
    gpos = pos;
    gpos *= 1000;
    gpos *= 1000;

# if GSTI_DEBUG
    {
      gint64    tpos;

      gst_element_query_position (gsti->srcbin [gsti->curr], GST_FORMAT_TIME, &tpos);
      logStderr ("-- set-pos %ld %ld\n", (long) pos, (long) (tpos / 1000 / 1000));
      logStderr ("-- set-pos: rate: %.4f\n", gsti->rate [gsti->curr]);
    }
# endif
    /* all seeks are based on the song's original duration, */
    /* not the adjusted duration */
    if (gst_element_seek (gsti->srcbin [gsti->curr], gsti->rate [gsti->curr],
        GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
        GST_SEEK_TYPE_SET, gpos,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
      rc = true;
logStderr ("    in-async\n");
      gsti->async = true;
    }
  }

  gstiRunOnce (gsti);
  return rc;
}

bool
gstiSetRate (gsti_t *gsti, double rate)
{
  int     rc = false;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  if (gsti->plistate == PLI_STATE_PAUSED ||
      gsti->plistate == PLI_STATE_PLAYING) {
    gint64    pos;

# if GSTI_DEBUG
    logStderr ("-- set-rate: gst-state: %d/%s\n", gsti->gststate, gst_element_state_get_name (gsti->gststate));
    logStderr ("-- set-rate %0.4f\n", rate);
# endif
    gsti->rate [gsti->curr] = rate;

    gst_element_query_position (gsti->srcbin [gsti->curr], GST_FORMAT_TIME, &pos);

    if (gst_element_seek (gsti->srcbin [gsti->curr], gsti->rate [gsti->curr],
        GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
        GST_SEEK_TYPE_SET, pos,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
      rc = true;
logStderr ("    in-async\n");
      gsti->async = true;
    }
  }

  gstiRunOnce (gsti);
  return rc;
}

int
gstiGetVolume (gsti_t *gsti)
{
  double  dval;
  int     val;

  gstiRunOnce (gsti);
  g_object_get (G_OBJECT (gsti->volume [gsti->curr]), "volume", &dval, NULL);
  val = round (dval * 100.0);
# if GSTI_DEBUG
  logStderr ("-- get volume %d %d\n", gsti->curr, val);
# endif
  return val;
}

/* internal routines */

static void
gstiRunOnce (gsti_t *gsti)
{
  if (gsti->inrunonce) {
logStderr ("-- nested runonce\n");
//    return;
  }

  gsti->inrunonce = true;
  while (g_main_context_iteration (gsti->mainctx, FALSE)) {
    ;
  }
// logStderr ("-- run-once: %d\n", gsti->async);
  gsti->inrunonce = false;
}

static gboolean
gstiBusCallback (GstBus * bus, GstMessage * message, void *udata)
{
  gsti_t      *gsti = udata;
  const char  *msgsrc = NULL;

  msgsrc = GST_MESSAGE_SRC_NAME (message);
# if GSTI_DEBUG
  logStderr ("message %d %s %s\n", gsti->curr,
      msgsrc,
      GST_MESSAGE_TYPE_NAME (message));
# endif

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING: {
      GError *err;
      gchar *debug;

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-error");
# endif

      gst_message_parse_error (message, &err, &debug);
      logStderr ("%d: %s %s %d %s\n", gsti->curr,
          msgsrc,
          GST_MESSAGE_TYPE_NAME (message),
          GST_MESSAGE_TYPE (message), err->message);
      if (debug != NULL) {
        logStderr ("   %s\n", debug);
        g_free (debug);
      }
      g_error_free (err);
      break;
    }
    case GST_MESSAGE_BUFFERING: {
      gint percent = 0;

      gst_message_parse_buffering (message, &percent);
      if (percent < 100 && gsti->plistate == PLI_STATE_PLAYING) {
        gstiPause (gsti);
        gsti->plistate = PLI_STATE_BUFFERING;
logStderr ("gsti: == buffering\n");
      }
      if (percent == 100 &&
          (gsti->plistate == PLI_STATE_BUFFERING ||
          gsti->plistate == PLI_STATE_PAUSED)) {
        gstiPlay (gsti);
      }
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState  old, new, pending;
      bool      skip = false;

      gst_message_parse_state_changed (message, &old, &new, &pending);
# if GSTI_DEBUG
      logStderr ("  old: %d/%s new: %d/%s pending: %d/%s async: %d\n",
          old, gst_element_state_get_name (old),
          new, gst_element_state_get_name (new),
          pending, gst_element_state_get_name (pending), gsti->async);
# endif
      if (gsti->async == false) {
        /* after a set-position, there are often spurious state changes */
        /* received from gstreamer that don't appear to be real, */
        /* as gstreamer continues to play */
        if (old == GST_STATE_PAUSED &&
            new == GST_STATE_PAUSED &&
            pending == GST_STATE_PAUSED) {
logStderr ("gsti: ppp state: set skip\n");
          skip = true;
        }
      }
      if (skip == false) {
        /* ignore any messages except from the source bins */
        if (strncmp (msgsrc,
            GSTI_SRCBIN_PFX, GSTI_SRCBIN_PFX_LEN) == 0) {
          int   binidx;

          binidx = atoi (msgsrc + GSTI_SRCBIN_PFX_LEN + 1);
logStderr ("gsti: proc-state chk: %s %d curr: %d\n", msgsrc + GSTI_SRCBIN_PFX_LEN + 1, binidx, gsti->curr);
          /* and only process the current source bin messages */
          if (gsti->curr == binidx) {
            gstiProcessState (gsti, new, pending);
          }
        }
      }
      break;
    }
    case GST_MESSAGE_CLOCK_LOST: {
      break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
      gsti->async = false;
      break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
      break;
    }
    case GST_MESSAGE_EOS: {
# if GSTI_DEBUG
      logStderr ("  eos curr: %d\n", gsti->curr);
# endif
      if (gsti->incrossfade) {
        gstiEndCrossFade (gsti);
      }

      if (! gsti->incrossfade) {
        gsti->plistate = PLI_STATE_STOPPED;
logStderr ("gsti: == stopped\n");
      }
      break;
    }
    default: {
      /* unhandled message */
      break;
    }
  }

  return G_SOURCE_CONTINUE;
}

/* while stopping states get messed up */
static void
gstiProcessState (gsti_t *gsti, GstState state, GstState pending)
{
logStderr ("gsti: process state: %d/%s %d/%s\n",
state, gst_element_state_get_name (state),
pending, gst_element_state_get_name (pending));
  switch (state) {
    case GST_STATE_VOID_PENDING: {
      break;
    }
    case GST_STATE_NULL: {
      if (pending == GST_STATE_VOID_PENDING) {
        gsti->plistate = PLI_STATE_IDLE;
logStderr ("gsti: == idle\n");
      }
      break;
    }
    case GST_STATE_READY: {
      if (pending == GST_STATE_VOID_PENDING) {
        gsti->plistate = PLI_STATE_STOPPED;
logStderr ("gsti: == stopped\n");
      }
      break;
    }
    case GST_STATE_PLAYING: {
      if (pending == GST_STATE_VOID_PENDING) {
        gsti->plistate = PLI_STATE_PLAYING;
logStderr ("gsti: == playing\n");
      }
      break;
    }
    case GST_STATE_PAUSED: {
      if (gsti->plistate == PLI_STATE_PLAYING) {
        gsti->plistate = PLI_STATE_PAUSED;
logStderr ("gsti: == paused\n");
      }
      break;
    }
  }

  if (state != GST_STATE_PLAYING && pending == GST_STATE_PLAYING) {
    gsti->plistate = PLI_STATE_OPENING;
logStderr ("gsti: == opening\n");
  }
}

/* wait-state is only used for the pipeline */
static void
gstiWaitState (gsti_t *gsti, GstElement *elem, GstState want)
{
  GstState    state;
  static int  maxcount = 1000000;
  int         count = 0;

  gstiRunOnce (gsti);
  gst_element_get_state (elem, &state, NULL, 1);
# if GSTI_DEBUG
  logStderr ("== wait-state curr: %d/%s want: %d/%s\n",
      state, gst_element_state_get_name (state),
      want, gst_element_state_get_name (want));
# endif
  while (state != want && count < maxcount) {
    gstiRunOnce (gsti);
    gst_element_get_state (elem, &state, NULL, 1);
    ++count;
  }
  if (count >= maxcount) {
    fprintf (stderr, "ERR: gsti: fail wait-state %d/%s want: %d/%s\n",
        state, gst_element_state_get_name (state),
        want, gst_element_state_get_name (want));
  }
  gstiProcessState (gsti, state, GST_STATE_VOID_PENDING);
  gstiRunOnce (gsti);
}

static void
gstiMakeSource (gsti_t *gsti, int idx)
{
  GstCaps           *caps;
  GstElement        *bin;
  GstElement        *decode;
  GstElement        *convert;
  GstElement        *scaletempo;
  GstElement        *st_convert;
  GstElement        *resample;
  GstElement        *volume;
  GstElement        *sink;
  char              tmpnm [40];

  snprintf (tmpnm, sizeof (tmpnm), "decode_%d", idx);
  decode = gst_element_factory_make ("uridecodebin3", tmpnm);
  if (decode == NULL) {
    fprintf (stderr, "ERR: unable to instantiate decoder\n");
  }
  caps = gst_caps_from_string ("audio/x-raw");
  g_object_set (decode, "caps", caps, NULL);
  gst_caps_unref (caps);
  gsti->source [idx] = decode;

  snprintf (tmpnm, sizeof (tmpnm), "cvt_%d", idx);
  convert = gst_element_factory_make ("audioconvert", tmpnm);
  if (convert == NULL) {
    fprintf (stderr, "ERR: unable to instantiate stcvt\n");
  }
  gsti->deccvt [idx] = convert;

  snprintf (tmpnm, sizeof (tmpnm), "st_%d", idx);
  scaletempo = gst_element_factory_make ("scaletempo", tmpnm);
  if (scaletempo == NULL) {
    fprintf (stderr, "ERR: unable to instantiate scaletempo\n");
  }

  snprintf (tmpnm, sizeof (tmpnm), "stcvt_%d", idx);
  st_convert = gst_element_factory_make ("audioconvert", tmpnm);
  if (st_convert == NULL) {
    fprintf (stderr, "ERR: unable to instantiate stcvt\n");
  }

  snprintf (tmpnm, sizeof (tmpnm), "resample_%d", idx);
  resample = gst_element_factory_make ("audioresample", tmpnm);
  if (resample == NULL) {
    fprintf (stderr, "ERR: unable to instantiate resample\n");
  }
  g_object_set (G_OBJECT (resample), "quality", 8, NULL);

  snprintf (tmpnm, sizeof (tmpnm), "autosink_%d", idx);
  sink = gst_element_factory_make ("autoaudiosink", tmpnm);
  if (sink == NULL) {
    fprintf (stderr, "ERR: unable to instantiate autoaudiosink\n");
  }

  snprintf (tmpnm, sizeof (tmpnm), "vol_%d", idx);
  volume = gst_element_factory_make ("volume", tmpnm);
  if (volume == NULL) {
    fprintf (stderr, "ERR: unable to instantiate volume\n");
  }
  gsti->volume [idx] = volume;

  snprintf (tmpnm, sizeof (tmpnm), "%s_%d", GSTI_SRCBIN_PFX, idx);
  bin = gst_bin_new (tmpnm);
  gsti->srcbin [idx] = bin;
  gst_bin_add_many (GST_BIN (bin),
      decode, convert, scaletempo, st_convert, resample, volume, sink, NULL);
  /* uridecodebin3 does not have a static pad */
  /* do not link decode, it will be linked in the pad-added handler */
  if (! gst_element_link_many (convert, scaletempo, st_convert,
      resample, volume, sink, NULL)) {
    fprintf (stderr, "ERR: link-many %d failed\n", idx);
  }

  /* uridecodebin3 does not have a static pad */
  g_signal_connect (G_OBJECT (decode), "pad-added",
      G_CALLBACK (gstiDynamicLinkPad), gsti);

  gstiRunOnce (gsti);
}

static void
gstiAddSourceToPipeline (gsti_t *gsti, int idx)
{
logStderr ("gsti: add bin %d\n", idx);
  gst_bin_add (GST_BIN (gsti->pipeline), gsti->srcbin [idx]);
  gstiRunOnce (gsti);
}

#if 0
static void
gstiRemoveSourceFromPipeline (gsti_t *gsti, int idx)
{
logStderr ("gsti: remove bin %d\n", idx);
  gst_object_ref (gsti->srcbin [idx]);
  gst_bin_remove (GST_BIN (gsti->pipeline), gsti->srcbin [idx]);
  gstiRunOnce (gsti);
}
#endif

static void
gstiDynamicLinkPad (GstElement *src, GstPad *newpad, gpointer udata)
{
  gsti_t            *gsti = udata;
  const char        *nm;
  int               idx;
  GstPad            *sinkpad = NULL;
  GstPadLinkReturn  rc;

  if (gsti->deccvt [gsti->curr] == NULL) {
    return;
  }

# if GSTI_DEBUG
  logStderr ("   link: newpad %s from %s %d\n", GST_PAD_NAME (newpad), GST_ELEMENT_NAME (src),
      gst_pad_is_linked (newpad));
# endif

  nm = GST_ELEMENT_NAME (src);
  nm = strstr (nm, "_");
  if (nm == NULL) {
    return;
  }
  ++nm;
  idx = atoi (nm);
  sinkpad = gst_element_get_static_pad (gsti->deccvt [idx], "sink");

# if GSTI_DEBUG
  logStderr ("     sinkpad idx:%d %s %d\n", idx, GST_PAD_NAME (sinkpad),
      gst_pad_is_linked (sinkpad));
# endif

  if (gst_pad_is_linked (newpad)) {
    return;
  }
  if (newpad == NULL) {
    return;
  }

  rc = gst_pad_link (newpad, sinkpad);
  if (GST_PAD_LINK_FAILED (rc)) {
    fprintf (stderr, "ERR: pad link failed\n");
  }
  gst_object_unref (sinkpad);

# if GSTI_DEBUG
  logStderr ("-- linked\n");
# endif

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-link");
# endif
}

static void
gstiChangeVolume (gsti_t *gsti, int curr)
{
  if (gsti->volume [curr] == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  g_object_set (G_OBJECT (gsti->volume [curr]),
      "volume", gsti->vol [gsti->curr], NULL);
  g_object_set (G_OBJECT (gsti->volume [curr]), "mute", false, NULL);
# if GSTI_DEBUG
  {
    double      dval;

    g_object_get (G_OBJECT (gsti->volume [curr]), "volume", &dval, NULL);
    logStderr ("gsti: chg volume %d %.2f new: %.2f\n", curr, gsti->vol [curr], dval);
  }
# endif
}

static void
gstiEndCrossFade (gsti_t *gsti)
{
  int       idx;
  gint64    pos;

logStderr ("gsti: end crossfade\n");
  /* current has been reset to point at the new song */
  idx = 1 - gsti->curr;

  gsti->vol [gsti->curr] = GSTI_FULL_VOL;
  gstiChangeVolume (gsti, gsti->curr);
  gstiRunOnce (gsti);

logStderr ("gsti: ecf bbb\n");
  gst_element_set_state (gsti->srcbin [idx], GST_STATE_READY);
  gstiRunOnce (gsti);
logStderr ("gsti: ecf bbcc\n");
  g_object_set (G_OBJECT (gsti->currsource), "uri", NULL, NULL);
//  gst_element_query_position (gsti->srcbin [idx], GST_FORMAT_TIME, &pos);
//  gst_element_seek (gsti->srcbin [idx], gsti->rate [idx],
//      GST_FORMAT_TIME,
//      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
//      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
//      GST_SEEK_TYPE_SET, pos);
  gstiRunOnce (gsti);
logStderr ("gsti: ecf ccc\n");

  /* the clock is lost, and the pipeline must be restarted */
  gstiSetRate (gsti, gsti->rate [gsti->curr]);
#if 0
  gst_element_set_state (gsti->srcbin [gsti->curr], GST_STATE_PAUSED);
  gstiWaitState (gsti, gsti->srcbin [gsti->curr], GST_STATE_PAUSED);
logStderr ("gsti: ecf ddd\n");
  gst_element_set_state (gsti->srcbin [gsti->curr], GST_STATE_PLAYING);
  gstiWaitState (gsti, gsti->srcbin [gsti->curr], GST_STATE_PLAYING);
logStderr ("gsti: ecf eee\n");
#endif

  gsti->vol [idx] = GSTI_NO_VOL;
  gsti->incrossfade = false;

# if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, "gsti-endcrossfade");
# endif
}


# if GSTI_DEBUG_DOT
static void
gstiDebugDot (gsti_t *gsti, const char *fn)
{
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (gsti->pipeline),
      GST_DEBUG_GRAPH_SHOW_STATES |
      GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS |
      GST_DEBUG_GRAPH_SHOW_FULL_PARAMS, fn);
  //    GST_DEBUG_GRAPH_SHOW_ALL |
  //    GST_DEBUG_GRAPH_SHOW_FULL_PARAMS, fn);
  gstiRunOnce (gsti);
}
# endif

#endif /* _hdr_gst */
