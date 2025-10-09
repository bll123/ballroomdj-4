/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 *
 * gst-launch-1.0 -vv \
 *    playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3
 *
 * gst-launch-1.0 -vv \
 *    playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 *
 * using adder
 * gst-launch-1.0 -vv \
 *    playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    audio_sink="scaletempo ! audioconvert ! audioresample ! adder name=mix ! audioconvert ! autoaudiosink"
 *
 * this works...2025-10-9
 * audiomixer doesn't work well with the test frequencies...
 * but everyone recommends it
 *
 * gst-launch-1.0 -vv \
 *    audiomixer name=mix ! audioconvert ! autoaudiosink \
 *    uridecodebin3 uri=file://$HOME/s/bdj4/test-music/003-chacha.mp3 \
 *    ! audioconvert ! scaletempo ! audioconvert ! \
 *    audioresample ! volume volume="0.15" ! mix. \
 *    uridecodebin3 uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *    ! audioconvert ! scaletempo ! audioconvert ! \
 *    audioresample ! volume volume="0.15" ! mix.
 *
 * this works...could be much easier than other methods...
 * a playbin is a pipeline, so we just have two pipelines set up.
 * gst-launch-1.0 -vv \
 *     playbin \
 *     uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3 \
 *     audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink" \
 *     playbin \
 *     uri=file://$HOME/s/bdj4/test-music/001-chacha.mp3 \
 *     audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 *
 * modifying a playbin pipeline:
 * https://gstreamer.freedesktop.org/documentation/tutorials/playback/custom-playbin-sinks.html?gi-language=c
 *
 * example gst-launch commands:
 * https://github.com/matthew1000/gstreamer-cheat-sheet/blob/master/mixing.md
 *
 * GST_DEBUG=2
 * GST_DEBUG_DUMP_DOT_DIR=<dir>
 *
 */
#if __has_include (<gst/gst.h>)

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstdebugutils.h>

#define GSTI_DEBUG 0
#define GSTI_DEBUG_DOT 0

#include "audiosrc.h"     // for audio-source type
#include "bdj4.h"
#include "bdjstring.h"
#if GSTI_DEBUG_DOT
# include "dirop.h"
#endif
#if GSTI_DEBUG
# include "log.h"
#endif
#include "mdebug.h"
#include "gsti.h"
#include "pli.h"
#include "tmutil.h"

// GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE
//    show caps-name on edges
// GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS
//    show caps-details on edges
// GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS
//    show modified parameters on elements
// GST_DEBUG_GRAPH_SHOW_STATES
//    show element states
// GST_DEBUG_GRAPH_SHOW_FULL_PARAMS
//    show full element parameter values even if they are very long
// GST_DEBUG_GRAPH_SHOW_ALL
//    show all the typical details that one might want
// GST_DEBUG_GRAPH_SHOW_VERBOSE
//    show all details regardless of how large or verbose they make the resulting output
#if GSTI_DEBUG_DOT
static int dbgdetails =
    GST_DEBUG_GRAPH_SHOW_STATES |
    GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS |
    GST_DEBUG_GRAPH_SHOW_FULL_PARAMS;
static int dbgcount = 0;
static plistate_t dbgstate = 0;
#endif

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

typedef struct gstiidx gstiidx_t;

typedef struct gstiidx {
  gsti_t        *gsti;
  double        rate;
  guint         busId;
  int           idx;
} gstiidx_t;

typedef struct gsti {
  uint64_t          ident;
  GMainContext      *mainctx;
  GstElement        *pipeline [PLI_MAX_SOURCE];
  gstiidx_t         gstiidx [PLI_MAX_SOURCE];
  int               curr;
  plistate_t        state;
  bool              isstopping;
  bool              inCrossFade;
} gsti_t;

static void gstiRunOnce (gsti_t *gsti);
static gboolean gstiBusCallback (GstBus * bus, GstMessage * message, void *udata);
static void gstiProcessState (gsti_t *gsti, GstState state);
static void gstiWaitState (gsti_t *gsti, GstState want);
#if GSTI_DEBUG_DOT
static void gstiDebugDot (gsti_t *gsti, GstElement *bin, const char *nm);
#endif

gsti_t *
gstiInit (const char *plinm)
{
  gsti_t            *gsti;
  GstBus            *bus = NULL;
  GstPlayFlags      flags;

#if GSTI_DEBUG_DOT
  diropMakeDir ("tmp/gst");
#endif

  gst_init (NULL, 0);

  gsti = mdmalloc (sizeof (gsti_t));
  gsti->ident = GSTI_IDENT;
  gsti->state = PLI_STATE_IDLE;
#if GST_DEBUG_DOT
  gsti->dbgstate = PLI_STATE_IDLE;
#endif
  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    gsti->pipeline [i] = NULL;
    gsti->gstiidx [i].gsti = gsti;
    gsti->gstiidx [i].busId = 0;
    gsti->gstiidx [i].idx = i;
    gsti->gstiidx [i].rate = 1.0;
  }
  gsti->curr = 0;
  gsti->isstopping = false;
  gsti->inCrossFade = false;

  gsti->mainctx = g_main_context_default ();
  gstiRunOnce (gsti);

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    GstElement        *sinkbin = NULL;
    GstElement        *scaletempo = NULL;
    GstElement        *convert = NULL;
    GstElement        *resample = NULL;
    GstElement        *audiosink = NULL;
    GstPad            *pad = NULL;
    GstPad            *ghost_pad = NULL;
    char              tmp [100];

    snprintf (tmp, sizeof (tmp), "play_%d", i);
    gsti->pipeline [i] = gst_element_factory_make ("playbin", tmp);
    if (gsti->pipeline [i] == NULL) {
      fprintf (stderr, "ERR: unable to instantiate playbin pipeline\n");
    }
    mdextalloc (gsti->pipeline [i]);
    gstiRunOnce (gsti);

    bus = gst_pipeline_get_bus (GST_PIPELINE (gsti->pipeline [i]));
    gsti->gstiidx [i].busId = gst_bus_add_watch (bus, gstiBusCallback, &gsti->gstiidx [i]);
    g_object_unref (bus);
    gstiRunOnce (gsti);

    snprintf (tmp, sizeof (tmp), "scaletempo_%d", i);
    scaletempo = gst_element_factory_make ("scaletempo", tmp);
    if (scaletempo == NULL) {
      fprintf (stderr, "ERR: unable to instantiate scaletempo\n");
    }

    snprintf (tmp, sizeof (tmp), "convert_%d", i);
    convert = gst_element_factory_make ("audioconvert", tmp);
    if (convert == NULL) {
      fprintf (stderr, "ERR: unable to instantiate convert\n");
    }

    snprintf (tmp, sizeof (tmp), "resample_%d", i);
    resample = gst_element_factory_make ("audioresample", tmp);
    if (resample == NULL) {
      fprintf (stderr, "ERR: unable to instantiate resample\n");
    }
    g_object_set (G_OBJECT (resample), "quality", 8, NULL);

    snprintf (tmp, sizeof (tmp), "audiosink_%d", i);
    audiosink = gst_element_factory_make ("autoaudiosink", tmp);
    if (audiosink == NULL) {
      fprintf (stderr, "ERR: unable to instantiate audiosink\n");
    }

    snprintf (tmp, sizeof (tmp), "sinkbin_%d", i);
    sinkbin = gst_bin_new (tmp);
    if (sinkbin == NULL) {
      fprintf (stderr, "ERR: unable to instantiate sinkbin\n");
    }
    gst_bin_add_many (GST_BIN (sinkbin), scaletempo, convert, resample, audiosink, NULL);
    if (! gst_element_link_many (scaletempo, convert, resample, audiosink, NULL)) {
      fprintf (stderr, "ERR: link-many sinkbin %d failed\n", i);
    }
    gstiRunOnce (gsti);
#if GSTI_DEBUG_DOT
    snprintf (tmp, sizeof (tmp), "sinkbin_%d", i);
    gstiDebugDot (gsti, sinkbin, tmp);
#endif

    pad = gst_element_get_static_pad (scaletempo, "sink");
    ghost_pad = gst_ghost_pad_new ("sink", pad);
    gst_pad_set_active (ghost_pad, TRUE);
    gst_element_add_pad (sinkbin, ghost_pad);
    gst_object_unref (pad);
    gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
    snprintf (tmp, sizeof (tmp), "sinkbin_ghost_%d", i);
    gstiDebugDot (gsti, sinkbin, tmp);
#endif

    g_object_set (G_OBJECT (gsti->pipeline [i]), "audio-sink", sinkbin, NULL);
    gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
    snprintf (tmp, sizeof (tmp), "playbin_set_%d", i);
    gstiDebugDot (gsti, playbin, tmp);
#endif

    gstiRunOnce (gsti);
  }

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    g_object_get (G_OBJECT (gsti->pipeline [i]), "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_AUDIO;
    flags &= ~GST_PLAY_FLAG_VIDEO;
    flags &= ~GST_PLAY_FLAG_TEXT;
    flags &= ~GST_PLAY_FLAG_VIS;
    g_object_set (G_OBJECT (gsti->pipeline [i]), "flags", flags, NULL);
    gstiRunOnce (gsti);

    g_object_set (G_OBJECT (gsti->pipeline [i]), "volume", 1.0, NULL);
    gstiRunOnce (gsti);

    gst_element_set_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), GST_STATE_READY);
    gstiRunOnce (gsti);
  }

#if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, NULL, "init");
#endif

  return gsti;
}

void
gstiFree (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT) {
    return;
  }

  gsti->isstopping = true;
  gstiRunOnce (gsti);

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    if (gsti->pipeline [i] == NULL) {
      continue;
    }

    g_object_set (G_OBJECT (gsti->pipeline [i]), "volume", 0.0, NULL);
    gstiRunOnce (gsti);
    gst_element_set_state (GST_ELEMENT (gsti->pipeline [i]), GST_STATE_READY);
    gstiWaitState (gsti, GST_STATE_READY);
    gst_element_set_state (GST_ELEMENT (gsti->pipeline [i]), GST_STATE_NULL);
    gstiWaitState (gsti, GST_STATE_NULL);
  }

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    mdextfree (gsti->pipeline [i]);
    gst_object_unref (gsti->pipeline [i]);
  }

//  gstiRunOnce (gsti);
  gsti->ident = BDJ4_IDENT_FREE;

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

  gsti->gstiidx [gsti->curr].rate = 1.0;
  g_object_set (G_OBJECT (gsti->pipeline [gsti->curr]), "uri", tbuff, NULL);
//  if (! gst_element_set_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), GST_STATE_PAUSED)) {
//    fprintf (stderr, "ERR: unable to set state (media-paused) %d\n", gsti->curr);
//  }
  gsti->state = PLI_STATE_OPENING;
  gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, NULL, "media");
#endif

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

  if (! gst_element_query_duration (gsti->pipeline [gsti->curr],
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

  if (! gst_element_query_position (gsti->pipeline [gsti->curr],
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
  gst_element_get_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), &state, &pending, 1);
  gstiProcessState (gsti, state);

#if GSTI_DEBUG_DOT
  if (gsti->state != dbgstate) {
    char    tmp [40];

    snprintf (tmp, sizeof (tmp), "state_%d", gsti->state);
    gstiDebugDot (gsti, NULL, tmp);
    dbgstate = gsti->state;
  }
#endif
  return gsti->state;
}

void
gstiPause (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  if (! gst_element_set_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), GST_STATE_PAUSED)) {
    fprintf (stderr, "ERR: unable to set state (paused) %d\n", gsti->curr);
  }
  gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, NULL, "pause");
#endif

  return;
}

void
gstiPlay (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  if (! gst_element_set_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), GST_STATE_PLAYING)) {
    fprintf (stderr, "ERR: unable to set state (play) %d\n", gsti->curr);
  }
  gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, NULL, "play");
#endif

  return;
}

void
gstiStop (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gsti->isstopping = true;
  g_object_set (G_OBJECT (gsti->pipeline [gsti->curr]), "volume", 0.0, NULL);
  gstiRunOnce (gsti);

  if (! gst_element_set_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), GST_STATE_READY)) {
    fprintf (stderr, "ERR: unable to set state (stop) %d\n", gsti->curr);
  }
  gstiWaitState (gsti, GST_STATE_READY);
  gstiRunOnce (gsti);

  g_object_set (G_OBJECT (gsti->pipeline [gsti->curr]), "volume", 1.0, NULL);
  gstiRunOnce (gsti);

#if GSTI_DEBUG_DOT
  gstiDebugDot (gsti, NULL, "stop");
#endif

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
    if (gst_element_seek (gsti->pipeline [gsti->curr], gsti->gstiidx [gsti->curr].rate,
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

    gsti->gstiidx [gsti->curr].rate = rate;

    gst_element_query_position (gsti->pipeline [gsti->curr], GST_FORMAT_TIME, &pos);

    if (gst_element_seek (gsti->pipeline [gsti->curr], gsti->gstiidx [gsti->curr].rate,
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

  gstiRunOnce (gsti);
  g_object_get (G_OBJECT (gsti->pipeline [gsti->curr]), "volume", &dval, NULL);
  val = round (dval * 100.0);
  return val;
}

int
gstiCrossFade (gsti_t *gsti, const char *fn, int sourceType)
{
  if (gsti == NULL) {
    return 1;
  }

  gsti->curr = (PLI_MAX_SOURCE - 1) - gsti->curr;
  gsti->inCrossFade = true;
  gstiMedia (gsti, fn, sourceType);

  return 0;
}

void
gstiCrossFadeVolume (gsti_t *gsti, int vol)
{
  int     previdx;
  double  dvol;

  if (gsti == NULL) {
    return;
  }
  if (gsti->pipeline [gsti->curr] == NULL) {
    return;
  }
  if (gsti->inCrossFade == false) {
    return;
  }

  previdx = (PLI_MAX_SOURCE - 1) - gsti->curr;
  dvol = (double) vol / 100.0;
  g_object_set (G_OBJECT (gsti->pipeline [previdx]), "volume", dvol, NULL);
  if (dvol <= 0.0) {
    if (! gst_element_set_state (GST_ELEMENT (gsti->pipeline [previdx]), GST_STATE_READY)) {
      fprintf (stderr, "ERR: unable to set state (crossfade-stop) %d\n", previdx);
    }
    gsti->inCrossFade = false;
  }

  dvol = 1.0 - dvol;
  g_object_set (G_OBJECT (gsti->pipeline [gsti->curr]), "volume", dvol, NULL);
}

#if 0
void
gstiSetVolume (gsti_t *gsti, double vol)
{
  double  dval = vol;

  gstiRunOnce (gsti);
  g_object_set (G_OBJECT (gsti->pipeline [gsti->curr]), "volume", &dval, NULL);
  gstiRunOnce (gsti);
}
#endif

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
  gstiidx_t   *gstiidx = udata;
  gsti_t      *gsti = gstiidx->gsti;

  // fprintf (stderr, "message %s\n", GST_MESSAGE_TYPE_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING: {
      GError  *err;
      gchar   *debug;

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

  gstiRunOnce (gsti);
  gst_element_get_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), &state, NULL, 1);
  while (state != want) {
    if (want == GST_STATE_READY && state == GST_STATE_NULL) {
      break;
    }
    gstiRunOnce (gsti);
    gst_element_get_state (GST_ELEMENT (gsti->pipeline [gsti->curr]), &state, NULL, 1);
    mssleep (1);
  }
  gstiProcessState (gsti, state);
}


#if GSTI_DEBUG_DOT
static void
gstiDebugDot (gsti_t *gsti, GstElement *bin, const char *nm)
{
  char  tmp [100];

  snprintf (tmp, sizeof (tmp), "%04d_%s", dbgcount, nm);
  ++dbgcount;
  if (bin != NULL) {
    GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (bin), dbgdetails, tmp);
  } else {
    GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (gsti->pipeline [gsti->curr]), dbgdetails, tmp);
  }
}
#endif

#endif /* gst/gst.h */
