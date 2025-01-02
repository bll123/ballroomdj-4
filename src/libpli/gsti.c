/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 *
 * gst-launch-1.0 -vv playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3
 *
 * gst-launch-1.0 -vv playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 *
 * modifying a playbin pipeline:
 * https://gstreamer.freedesktop.org/documentation/tutorials/playback/custom-playbin-sinks.html?gi-language=c
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

#include "audiosrc.h"     // for audio-source type
# include "bdj4.h"
# include "bdjstring.h"
# include "mdebug.h"
# include "gsti.h"
# include "pli.h"

enum {
  GSTI_IDENT = 0xccbbaa0069747367,
};

/* gstreamer doesn't define these */
typedef enum {
  GST_PLAY_FLAG_VIDEO = (1 << 0),
  GST_PLAY_FLAG_AUDIO = (1 << 1),
  GST_PLAY_FLAG_TEXT  = (1 << 2),
  GST_PLAY_FLAG_VIS  = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOL  = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD = (1 << 7),
  GST_PLAY_FLAG_BUFFERING = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE = (1 << 9),
  GST_PLAY_FLAG_SOFT_COL_BAL = (1 << 10),
  GST_PLAY_FLAG_FORCE_FILTER = (1 << 11),
  GST_PLAY_FLAG_FORCE_SW_DECODE = (1 << 12),
} GstPlayFlags;

typedef struct gsti {
  uint64_t          ident;
  GMainContext      *mainctx;
  GstElement        *pipeline;
  guint             busId;
  plistate_t        state;
  double            rate;
  bool              isstopping : 1;
} gsti_t;

static void gstiRunOnce (gsti_t *gsti);
static gboolean gstiBusCallback (GstBus * bus, GstMessage * message, void *udata);
static void gstiProcessState (gsti_t *gsti, GstState state);
static void gstiWaitState (gsti_t *gsti, GstState want);

gsti_t *
gstiInit (const char *plinm)
{
  gsti_t            *gsti;
  GstBus            *bus;
  GstPad            *pad;
  GstPad            *ghost_pad;
  GstElement        *sinkbin;
  GstElement        *convert;
  GstElement        *scaletempo;
  GstElement        *resample;
  GstElement        *sink;
  GstPlayFlags      flags;

  gst_init (NULL, 0);

  gsti = mdmalloc (sizeof (gsti_t));
  gsti->ident = GSTI_IDENT;
  gsti->pipeline = NULL;
  gsti->busId = 0;
  gsti->state = PLI_STATE_IDLE;
  gsti->rate = 1.0;
  gsti->isstopping = false;

  gsti->mainctx = g_main_context_default ();
  gstiRunOnce (gsti);

  gsti->pipeline = gst_element_factory_make ("playbin", "play");
  mdextalloc (gsti->pipeline);
  gstiRunOnce (gsti);

  g_object_get (G_OBJECT (gsti->pipeline), "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_AUDIO;
  flags &= ~GST_PLAY_FLAG_VIDEO;
  flags &= ~GST_PLAY_FLAG_TEXT;
  flags &= ~GST_PLAY_FLAG_VIS;
  g_object_set (G_OBJECT (gsti->pipeline), "flags", flags, NULL);
  gstiRunOnce (gsti);

  g_object_set (G_OBJECT (gsti->pipeline), "volume", 1.0, NULL);
  gstiRunOnce (gsti);

  scaletempo = gst_element_factory_make ("scaletempo", "scaletempo");
  convert = gst_element_factory_make ("audioconvert", "convert");
  resample = gst_element_factory_make ("audioresample", "resample");
  g_object_set (G_OBJECT (resample), "quality", 8, NULL);
  gstiRunOnce (gsti);

  sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
  sinkbin = gst_bin_new ("audio_sink_bin");
  gst_bin_add_many (GST_BIN (sinkbin), scaletempo, convert, resample, sink, NULL);
  gst_element_link_many (scaletempo, convert, resample, sink, NULL);

  pad = gst_element_get_static_pad (scaletempo, "sink");
  ghost_pad = gst_ghost_pad_new ("sink", pad);
  gst_pad_set_active (ghost_pad, TRUE);
  gst_element_add_pad (sinkbin, ghost_pad);
  gst_object_unref (pad);

  g_object_set (G_OBJECT (gsti->pipeline), "audio-sink", sinkbin, NULL);
  gstiRunOnce (gsti);

  bus = gst_pipeline_get_bus (GST_PIPELINE (gsti->pipeline));
  gsti->busId = gst_bus_add_watch (bus, gstiBusCallback, gsti);
  g_object_unref (bus);
  gstiRunOnce (gsti);

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
    g_object_set (G_OBJECT (gsti->pipeline), "volume", 0.0, NULL);
    gstiRunOnce (gsti);
    gst_element_set_state (gsti->pipeline, GST_STATE_READY);
    gstiWaitState (gsti, GST_STATE_READY);
    gst_element_set_state (gsti->pipeline, GST_STATE_NULL);
    gstiWaitState (gsti, GST_STATE_NULL);
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

  gsti->rate = 1.0;
  g_object_set (G_OBJECT (gsti->pipeline), "uri", tbuff, NULL);
  gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PAUSED);
  gsti->state = PLI_STATE_OPENING;
  gstiRunOnce (gsti);
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

  if (! gst_element_query_duration (gsti->pipeline,
      GST_FORMAT_TIME, &ctm)) {
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

  if (! gst_element_query_position (gsti->pipeline,
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
  gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, &pending, 1);
  gstiProcessState (gsti, state);

  return gsti->state;
}

void
gstiPause (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PAUSED);
  gstiRunOnce (gsti);
  return;
}

void
gstiPlay (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_PLAYING);
  gstiRunOnce (gsti);
  return;
}

void
gstiStop (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gsti->isstopping = true;
  g_object_set (G_OBJECT (gsti->pipeline), "volume", 0.0, NULL);
  gstiRunOnce (gsti);

  gst_element_set_state (GST_ELEMENT (gsti->pipeline), GST_STATE_READY);
  gstiWaitState (gsti, GST_STATE_READY);
  gstiRunOnce (gsti);

  g_object_set (G_OBJECT (gsti->pipeline), "volume", 1.0, NULL);
  gstiRunOnce (gsti);

  gsti->isstopping = false;

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
    gpos = pos;
    gpos *= 1000;
    gpos *= 1000;

    /* all seeks are based on the song's original duration, */
    /* not the adjusted duration */
    if (gst_element_seek (gsti->pipeline, gsti->rate,
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

    gsti->rate = rate;

    gst_element_query_position (gsti->pipeline, GST_FORMAT_TIME, &pos);

    if (gst_element_seek (gsti->pipeline, gsti->rate,
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
  double  dval;
  int     val;

  g_object_get (G_OBJECT (gsti->pipeline), "volume", &dval, NULL);
  val = round (dval * 100.0);
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

  // fprintf (stderr, "message %s\n", GST_MESSAGE_TYPE_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING: {
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      fprintf (stderr, "%d: %s\n", GST_MESSAGE_TYPE (message), err->message);
      g_error_free (err);
      g_free (debug);
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
      GstState old_state, new_state, pending_state;

      gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);
      gstiProcessState (gsti, new_state);
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

  gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, NULL, 1);
  while (state != want) {
    gstiRunOnce (gsti);
    gst_element_get_state (GST_ELEMENT (gsti->pipeline), &state, NULL, 1);
  }
  gstiProcessState (gsti, state);
}

#endif /* _hdr_gst */
