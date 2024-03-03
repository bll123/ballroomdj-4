/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * gst-launch-1.0 -vv playbin \
 *    uri=file://$HOME/s/bdj4/test-music/001-argentinetango.mp3
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#if _hdr_gst_gst

# include <gst/gst.h>
# include <glib.h>

# include "bdj4.h"
# include "mdebug.h"
# include "gsti.h"
# include "pli.h"

enum {
  GSTI_IDENT = 0x6773746900aabbcc,
};

typedef struct gsti {
  int64_t       ident;
  GMainContext  *mainctx;
  GstElement    *play;
  guint         busId;
  plistate_t    state;
} gsti_t;

static void gstiRunOnce (gsti_t *gsti);
static gboolean gstiBusCallback (GstBus * bus, GstMessage * message, void *udata);

gsti_t *
gstiInit (const char *plinm)
{
  gsti_t      *gsti;

  gst_init (NULL, 0);

  gsti = mdmalloc (sizeof (gsti_t));
  gsti->ident = GSTI_IDENT;
  gsti->play = NULL;
  gsti->busId = 0;
  gsti->state = PLI_STATE_NONE;

  gsti->mainctx = g_main_context_default ();
fprintf (stderr, "mainctx: %p\n", gsti->mainctx);
//  g_main_context_acquire (gsti->mainctx);
  gstiRunOnce (gsti);

  return gsti;
}

void
gstiFree (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT) {
    return;
  }

  gsti->ident = 0;

  if (gsti->play != NULL) {
    gst_element_set_state (gsti->play, GST_STATE_NULL);
    gst_object_unref (gsti->play);
  }

//  g_main_context_release (gsti->mainctx);
//  if (G_IS_OBJECT (gsti->mainctx)) {
//    g_object_unref (gsti->mainctx);
//  }

  mdfree (gsti);
}

void
gstiCleanup (void)
{
  return;
}

bool
gstiHasSpeed (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  return false;
}

void
gstiMedia (gsti_t *gsti, const char *fulluri)
{
  char    tbuff [MAXPATHLEN];
  GstBus  *bus;

  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "file://%s", fulluri);

  gstiRunOnce (gsti);
  gsti->play = gst_element_factory_make ("playbin", "play");
fprintf (stderr, "play: %p\n", gsti->play);
  g_object_set (G_OBJECT (gsti->play), "uri", tbuff, NULL);
  gst_element_set_state (GST_ELEMENT (gsti->play), GST_STATE_READY);

  bus = gst_pipeline_get_bus (GST_PIPELINE (gsti->play));
  gsti->busId = gst_bus_add_watch (bus, gstiBusCallback, gsti);
  g_object_unref (bus);

  gstiRunOnce (gsti);
  return;
}

int64_t
gstiGetDuration (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return 0;
  }

  gstiRunOnce (gsti);
  return 0;
}

int64_t
gstiGetPosition (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return 0;
  }

  gstiRunOnce (gsti);
  return 0;
}

plistate_t
gstiState (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return PLI_STATE_NONE;
  }

  gstiRunOnce (gsti);
  return gsti->state;
}

void
gstiPause (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gst_element_set_state (GST_ELEMENT (gsti->play), GST_STATE_PAUSED);
  gstiRunOnce (gsti);
  gsti->state = PLI_STATE_PAUSED;
  return;
}

void
gstiPlay (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gstiRunOnce (gsti);
  gst_element_set_state (GST_ELEMENT (gsti->play), GST_STATE_PLAYING);
  gstiRunOnce (gsti);
  gsti->state = PLI_STATE_PLAYING;
  return;
}

void
gstiStop (gsti_t *gsti)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return;
  }

  gst_element_set_state (GST_ELEMENT (gsti->play), GST_STATE_PAUSED);
  gstiRunOnce (gsti);
  gsti->state = PLI_STATE_STOPPED;
  return;
}

bool
gstiSetPosition (gsti_t *gsti, int64_t pos)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  return false;
}

bool
gstiSetVolume (gsti_t *gsti, double vol)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  return false;
}

bool
gstiSetRate (gsti_t *gsti, double rate)
{
  if (gsti == NULL || gsti->ident != GSTI_IDENT || gsti->mainctx == NULL) {
    return false;
  }

  return false;
}

/* internal routines */

static void
gstiRunOnce (gsti_t *gsti)
{
  while (g_main_context_iteration (gsti->mainctx, FALSE)) {
fprintf (stderr, "  iter\n");
    ;
  }
}

static gboolean
gstiBusCallback (GstBus * bus, GstMessage * message, void *udata)
{
  gsti_t    *gsti = udata;

  g_print ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING: {
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("%d: %s\n", GST_MESSAGE_TYPE (message), err->message);
      g_error_free (err);
      g_free (debug);
      break;
    }
    case GST_MESSAGE_BUFFERING: {
fprintf (stderr, "  buffering\n");
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
fprintf (stderr, "  state-chg\n");
      break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
fprintf (stderr, "  dur-chg\n");
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

  return TRUE;
}

#endif /* _hdr_gst */
