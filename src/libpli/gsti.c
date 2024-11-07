/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * gst-launch-1.0 -vv playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3
 *
 * gst-launch-1.0 -vv playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 *
 * working mix:
 * gst-launch-1.0 \
 *     audiomixer name=mix ! audioconvert ! autoaudiosink \
 *     uridecodebin3 uri='file:///home/bll/s/bdj4/test-music/001-chacha.mp3' \
 *       caps='audio/x-raw' \
 *     ! audioconvert ! \
 *     scaletempo ! audioconvert ! audioresample ! volume volume="0.4" ! \
 *     mix. \
 *     audiotestsrc ! volume volume="0.3" ! mix.
 *
 * modifying a playbin pipeline:
 * https://gstreamer.freedesktop.org/documentation/tutorials/playback/custom-playbin-sinks.html?gi-language=c
 *
 * export GST_DEBUG_DUMP_DOT_DIR=/home/bll/s/bdj4/tmp/
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

# define GSTI_DEBUG 1
# define GSTI_NO_VOL 0.0
# define GSTI_FULL_VOL 1.0
# define GSTI_NORM_RATE 1.0

enum {
  GSTI_IDENT = 0xccbbaa0069747367,
};

enum {
  GSTI_MAX_SOURCE = 2,
};

typedef struct gsti {
  uint64_t          ident;
  GMainContext      *mainctx;
  GstElement        *pipeline;
  GstElement        *mix;
  GstElement        *currsource;
  GstElement        *source [GSTI_MAX_SOURCE];
  GstPad            *endpt [GSTI_MAX_SOURCE];
  GstPad            *mixsink [GSTI_MAX_SOURCE];
  GstElement        *deccvt [GSTI_MAX_SOURCE];
  GstState          gststate;
  guint             busId;
  plistate_t        state;
  double            vol [GSTI_MAX_SOURCE];
  double            rate [GSTI_MAX_SOURCE];
  int               curr;
  bool              active [GSTI_MAX_SOURCE];
  bool              isstopping : 1;
  bool              incrossfade : 1;
} gsti_t;

static void gstiRunOnce (gsti_t *gsti);
static gboolean gstiBusCallback (GstBus * bus, GstMessage * message, void *udata);
static void gstiProcessState (gsti_t *gsti, GstState state);
static void gstiWaitState (gsti_t *gsti, GstState want);
static void gstiMakeSource (gsti_t *gsti, GstElement *mix);
static void gstiDynamicLinkPad (GstElement *src, GstPad *newpad, gpointer udata);
static void gstiChangeVolume (gsti_t *gsti);
# if GSTI_DEBUG
static void gstiDebugDot (gsti_t *gsti, const char *fn);
# endif

gsti_t *
gstiInit (const char *plinm)
{
  gsti_t            *gsti;
  GstBus            *bus;
  GstElement        *mix;
  GstElement        *mixconvert;
  GstElement        *sink;

  gst_init (NULL, 0);

  gsti = mdmalloc (sizeof (gsti_t));
  gsti->ident = GSTI_IDENT;
  gsti->pipeline = NULL;
  gsti->currsource = NULL;
  gsti->busId = 0;
  gsti->state = PLI_STATE_IDLE;
  for (int i = 0; i < GSTI_MAX_SOURCE; ++i) {
    gsti->source [i] = NULL;
    gsti->mixsink [i] = NULL;
    gsti->deccvt [i] = NULL;
    gsti->rate [i] = GSTI_NORM_RATE;
    gsti->vol [i] = GSTI_NO_VOL;
    gsti->active [i] = false;
  }
  gsti->curr = 0;
  gsti->isstopping = false;
  gsti->incrossfade = false;

  gsti->mainctx = g_main_context_default ();
  gstiRunOnce (gsti);

  gsti->pipeline = gst_pipeline_new ("crossfade");
  mdextalloc (gsti->pipeline);
  gstiRunOnce (gsti);

  /* the end of the pipeline: */
  /*    audiomixer ! audioconvert ! autoaudiosink */
  mix = gst_element_factory_make ("audiomixer", "mix");
  gsti->mix = mix;

  mixconvert = gst_element_factory_make ("audioconvert", "mixcvt");
  sink = gst_element_factory_make ("autoaudiosink", "autosink");
  g_object_set (G_OBJECT (sink), "message-forward", true, NULL);

  gst_bin_add_many (GST_BIN (gsti->pipeline), mix, mixconvert, sink, NULL);
  if (! gst_element_link_many (mix, mixconvert, sink, NULL)) {
    fprintf (stderr, "ERR: link-many mix failed\n");
  }

  gsti->vol [gsti->curr] = GSTI_FULL_VOL;
  gstiMakeSource (gsti, mix);
  gsti->currsource = gsti->source [gsti->curr];

  bus = gst_pipeline_get_bus (GST_PIPELINE (gsti->pipeline));
  gsti->busId = gst_bus_add_watch (bus, gstiBusCallback, gsti);
  g_object_unref (bus);
  gstiRunOnce (gsti);

  gsti->active [gsti->curr] = true;

# if GSTI_DEBUG
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
    gsti->vol [gsti->curr] = GSTI_NO_VOL;
    gstiChangeVolume (gsti);
    gst_element_set_state (gsti->pipeline, GST_STATE_READY);
    gstiWaitState (gsti, GST_STATE_READY);
    gst_element_set_state (gsti->pipeline, GST_STATE_NULL);
    gstiWaitState (gsti, GST_STATE_NULL);
    gsti->vol [gsti->curr] = GSTI_FULL_VOL;
    gstiChangeVolume (gsti);
    mdextfree (gsti->pipeline);
    gst_object_unref (gsti->pipeline);
  }

  gstiRunOnce (gsti);

  mdfree (gsti);
}

void
gstiCleanup (void)
{
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

  gsti->rate [gsti->curr] = GSTI_NORM_RATE;
# if GSTI_DEBUG
  logStderr ("uri: %s\n", tbuff);
# endif
  g_object_set (G_OBJECT (gsti->currsource), "uri", tbuff, NULL);
  rc = gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PAUSED);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (media)\n");
  }
  gsti->state = PLI_STATE_OPENING;
  gstiRunOnce (gsti);
# if GSTI_DEBUG
  gstiDebugDot (gsti, "gsti-media");
# endif
  return;
}

void
gstiCrossFade (gsti_t *gsti, const char *fulluri, int sourceType)
{
  return;
}

void
gstiCrossFadeVolume (gsti_t *gsti, int vol)
{
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

  if (! gst_element_query_duration (gsti->pipeline, GST_FORMAT_TIME, &ctm)) {
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

  if (! gst_element_query_position (gsti->pipeline, GST_FORMAT_TIME, &ctm)) {
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
  gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, &pending, 1);
  gstiProcessState (gsti, state);
# if GSTI_DEBUG
  if (gsti->gststate != state) {
    char    tmp [80];

    logStderr ("gsti-state: curr: %d pending: %d\n", state, pending);
    snprintf (tmp, sizeof (tmp), "gsti-state_%d", state);
    gstiDebugDot (gsti, tmp);
  }
# endif
  gsti->gststate = state;

  return gsti->state;
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
  rc = gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PAUSED);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (pause)\n");
  }
  gstiRunOnce (gsti);
# if GSTI_DEBUG
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
  rc = gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PLAYING);
  if (rc == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "ERR: unable to change state (pause)\n");
  }
  gstiRunOnce (gsti);
# if GSTI_DEBUG
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
  gsti->vol [gsti->curr] = GSTI_NO_VOL;
  gstiChangeVolume (gsti);

  gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_READY);
  gstiWaitState (gsti, GST_STATE_READY);
  gstiRunOnce (gsti);

  gsti->vol [gsti->curr] = GSTI_FULL_VOL;
  gstiChangeVolume (gsti);

  gsti->isstopping = false;

# if GSTI_DEBUG
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

  if (gsti->state == PLI_STATE_PAUSED ||
      gsti->state == PLI_STATE_PLAYING) {
    gint64    tpos;

    gpos = pos;
    gpos *= 1000;
    gpos *= 1000;

# if GSTI_DEBUG
    gst_element_query_position (gsti->pipeline, GST_FORMAT_TIME, &tpos);
    logStderr ("-- set-pos %ld %ld\n", (long) pos, (long) (tpos / 1000 / 1000));
# endif
    /* all seeks are based on the song's original duration, */
    /* not the adjusted duration */
    if (gst_element_seek (gsti->pipeline, gsti->rate [gsti->curr],
        GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
        GST_SEEK_TYPE_SET, gpos,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
      rc = true;
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

  if (gsti->state == PLI_STATE_PAUSED ||
      gsti->state == PLI_STATE_PLAYING) {
    gint64    pos;

# if GSTI_DEBUG
    logStderr ("-- set-rate %0.2f\n", rate);
# endif
    gsti->rate [gsti->curr] = rate;

    gst_element_query_position (gsti->pipeline, GST_FORMAT_TIME, &pos);

    if (gst_element_seek (gsti->pipeline, gsti->rate [gsti->curr],
        GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
        GST_SEEK_TYPE_SET, pos,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
      rc = true;
    }
  }

  gstiRunOnce (gsti);
  return rc;
}

int
gstiGetVolume (gsti_t *gsti)
{
//  double  dval;
  int     val;

//  g_object_get (G_OBJECT (gsti->mixsink [gsti->curr]), "volume", &dval, NULL);
//  val = round (dval * 100.0);
// ###
  val = 100;
# if GSTI_DEBUG
  logStderr ("-- get volume %d\n", val);
# endif
  return val;
}

/* internal routines */

static void
gstiRunOnce (gsti_t *gsti)
{
  while (g_main_context_iteration (gsti->mainctx, FALSE)) {
    ;
  }
}

static gboolean
gstiBusCallback (GstBus * bus, GstMessage * message, void *udata)
{
  gsti_t    *gsti = udata;

# if GSTI_DEBUG
    logStderr ("message %d %s %s\n", gsti->curr,
        GST_MESSAGE_SRC_NAME (message),
        GST_MESSAGE_TYPE_NAME (message));
# endif

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING: {
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      logStderr ("%d: %s %s %d %s\n", gsti->curr,
          GST_MESSAGE_SRC_NAME (message),
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
      if (percent < 100 && gsti->state == PLI_STATE_PLAYING) {
        gstiPause (gsti);
        gsti->state = PLI_STATE_BUFFERING;
      }
      if (percent == 100 &&
          (gsti->state == PLI_STATE_BUFFERING ||
          gsti->state == PLI_STATE_PAUSED)) {
        gstiPlay (gsti);
      }
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old, new, pending;

      gst_message_parse_state_changed (message, &old, &new, &pending);
# if GSTI_DEBUG
      logStderr ("  old: %d new: %d pending: %d\n", old, new, pending);
# endif
      gstiProcessState (gsti, new);
      break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
      break;
    }
    case GST_MESSAGE_EOS: {
      gsti->state = PLI_STATE_STOPPED;
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
gstiProcessState (gsti_t *gsti, GstState state)
{
  switch (state) {
    case GST_STATE_VOID_PENDING: {
      break;
    }
    case GST_STATE_NULL: {
      if (gsti->state != PLI_STATE_OPENING) {
        gsti->state = PLI_STATE_IDLE;
      }
      break;
    }
    case GST_STATE_READY: {
      if (gsti->state != PLI_STATE_IDLE &&
          gsti->state != PLI_STATE_OPENING) {
        gsti->state = PLI_STATE_STOPPED;
      }
      break;
    }
    case GST_STATE_PLAYING: {
      gsti->state = PLI_STATE_PLAYING;
      break;
    }
    case GST_STATE_PAUSED: {
      if (gsti->state == PLI_STATE_IDLE ||
          gsti->state == PLI_STATE_STOPPED ||
          gsti->state == PLI_STATE_OPENING) {
        if (! gsti->isstopping) {
          gsti->state = PLI_STATE_OPENING;
        }
      }
      if (gsti->state == PLI_STATE_PLAYING) {
        gsti->state = PLI_STATE_PAUSED;
      }
      break;
    }
  }
}

static void
gstiWaitState (gsti_t *gsti, GstState want)
{
  GstState    state;
  static int  maxcount = 1000000;
  int         count = 0;

  gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, NULL, 1);
# if GSTI_DEBUG
  logStderr ("== wait-state curr: %d want: %d\n", state, want);
# endif
  while (state != want && count < maxcount) {
    gstiRunOnce (gsti);
    gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, NULL, 1);
    ++count;
  }
  if (count >= maxcount) {
    fprintf (stderr, "ERR: gsti: fail wait-state: %d %d\n", state, want);
  }
  gstiProcessState (gsti, state);
}

static void
gstiMakeSource (gsti_t *gsti, GstElement *mix)
{
  GstElement        *decode;
  GstCaps           *caps;
  GstElement        *dec_convert;
  GstElement        *scaletempo;
  GstElement        *st_convert;
  GstElement        *resample;
  char              tmpnm [40];

  snprintf (tmpnm, sizeof (tmpnm), "decode_%d", gsti->curr);
  decode = gst_element_factory_make ("uridecodebin3", tmpnm);
  if (decode == NULL) {
    fprintf (stderr, "ERR: unable to instantiate decoder\n");
  }
  caps = gst_caps_from_string ("audio/x-raw");
  g_object_set (decode, "caps", caps, NULL);
  gst_caps_unref (caps);
  g_object_set (G_OBJECT (decode), "use-buffering", true, NULL);
  gsti->source [gsti->curr] = decode;

  snprintf (tmpnm, sizeof (tmpnm), "deccvt_%d", gsti->curr);
  dec_convert = gst_element_factory_make ("audioconvert", tmpnm);
  if (dec_convert == NULL) {
    fprintf (stderr, "ERR: unable to instantiate deccvt\n");
  }
  /* the element gets saved off rather than the sink, as gstreamer */
  /* will auto-dispose of the sink when uridecodebin3 disconnects */
  /* from the sink */
  gsti->deccvt [gsti->curr] = dec_convert;

  snprintf (tmpnm, sizeof (tmpnm), "st_%d", gsti->curr);
  scaletempo = gst_element_factory_make ("scaletempo", tmpnm);
  if (scaletempo == NULL) {
    fprintf (stderr, "ERR: unable to instantiate scaletempo\n");
  }

  snprintf (tmpnm, sizeof (tmpnm), "stcvt_%d", gsti->curr);
  st_convert = gst_element_factory_make ("audioconvert", tmpnm);
  if (st_convert == NULL) {
    fprintf (stderr, "ERR: unable to instantiate stcvt\n");
  }

  snprintf (tmpnm, sizeof (tmpnm), "resample_%d", gsti->curr);
  resample = gst_element_factory_make ("audioresample", tmpnm);
  if (resample == NULL) {
    fprintf (stderr, "ERR: unable to instantiate resample\n");
  }
  g_object_set (G_OBJECT (resample), "quality", 8, NULL);
  gsti->endpt [gsti->curr] = gst_element_get_static_pad (resample, "src");

  gst_bin_add_many (GST_BIN (gsti->pipeline),
      decode, dec_convert, scaletempo, st_convert, resample, NULL);
  /* uridecodebin3 does not have a static pad */
  /* do not link decode, it will be linked in the pad-added handler */
  if (! gst_element_link_many (dec_convert,
      scaletempo, st_convert, resample, NULL)) {
    fprintf (stderr, "ERR: link-many decoder %d failed\n", gsti->curr);
  }

  /* uridecodebin3 does not have a static pad */
  g_signal_connect (G_OBJECT (decode), "pad-added",
      G_CALLBACK (gstiDynamicLinkPad), gsti);

  gstiRunOnce (gsti);
}


static void
gstiDynamicLinkPad (GstElement *src, GstPad *newpad, gpointer udata)
{
  gsti_t            *gsti = udata;
  GstPad            *sinkpad = NULL;
  GstPad            *srcpad = NULL;
  GstPadLinkReturn  rc;

  sinkpad = gst_element_get_static_pad (gsti->deccvt [gsti->curr], "sink");

# if GSTI_DEBUG
  logStderr ("   link: newpad %s from %s %d\n", GST_PAD_NAME (newpad), GST_ELEMENT_NAME (src),
      gst_pad_is_linked (newpad));
  logStderr ("     sinkpad %s %d\n", GST_PAD_NAME (sinkpad),
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
  gstiDebugDot (gsti, "gsti-link_a");
# endif

  srcpad = gsti->endpt [gsti->curr];
  if (gsti->mixsink [gsti->curr] == NULL) {
    /* this will create a new audiomixer sink */
    gsti->mixsink [gsti->curr] =
        gst_element_request_pad_simple (gsti->mix, "sink_%u");
  }

# if GSTI_DEBUG
  logStderr ("   link: srcpad %s %d:\n", GST_PAD_NAME (srcpad),
      gst_pad_is_linked (srcpad));
  logStderr ("     mixsink %s %d:\n", GST_PAD_NAME (gsti->mixsink [gsti->curr]),
      gst_pad_is_linked (gsti->mixsink [gsti->curr]));
# endif

  if (! gst_pad_is_linked (srcpad)) {
    rc = gst_pad_link (srcpad, gsti->mixsink [gsti->curr]);
    if (GST_PAD_LINK_FAILED (rc)) {
      fprintf (stderr, "ERR: endpt link failed\n");
    }
  }

  gstiRunOnce (gsti);

  gstiChangeVolume (gsti);

# if GSTI_DEBUG
  gstiDebugDot (gsti, "gsti-link_b");
# endif
}

static void
gstiChangeVolume (gsti_t *gsti)
{
  double      dval;

  if (gsti->mixsink [gsti->curr] == NULL) {
    return;
  }

  g_object_set (G_OBJECT (gsti->mixsink [gsti->curr]),
      "volume", gsti->vol [gsti->curr], NULL);
  g_object_set (G_OBJECT (gsti->mixsink [gsti->curr]), "mute", false, NULL);
# if GSTI_DEBUG
  g_object_get (G_OBJECT (gsti->mixsink [gsti->curr]), "volume", &dval, NULL);
  logStderr ("-- set volume %d %.2f %.2f\n", gsti->curr, gsti->vol [gsti->curr], dval);
# endif
  gstiRunOnce (gsti);
}

# if GSTI_DEBUG
static void
gstiDebugDot (gsti_t *gsti, const char *fn)
{
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (gsti->pipeline),
      GST_DEBUG_GRAPH_SHOW_STATES |
      GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS |
      GST_DEBUG_GRAPH_SHOW_FULL_PARAMS, fn);
  gstiRunOnce (gsti);
}
# endif

#endif /* _hdr_gst */
