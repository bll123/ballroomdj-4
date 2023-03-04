/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "mdebug.h"
#include "callback.h"

#include "ui/uikeys.h"

enum {
  KEY_EVENT_NONE,
  KEY_EVENT_PRESS,
  KEY_EVENT_RELEASE,
};

typedef struct uikey {
  callback_t    *presscb;
  callback_t    *releasecb;
  int           eventtype;
  bool          controlpressed;
  bool          shiftpressed;
  bool          ismaskedkey;
  guint         keyval;
} uikey_t;

static gboolean uiKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata);

uikey_t *
uiKeyAlloc (void)
{
  uikey_t *uikey;

  uikey = mdmalloc (sizeof (uikey_t));
  uikey->presscb = NULL;
  uikey->releasecb = NULL;
  uikey->eventtype = KEY_EVENT_NONE;
  uikey->controlpressed = false;
  uikey->shiftpressed = false;
  uikey->ismaskedkey = false;
  return uikey;
}

void
uiKeyFree (uikey_t *uikey)
{
  if (uikey != NULL) {
    mdfree (uikey);
  }
}

void
uiKeySetKeyCallback (uikey_t *uikey, UIWidget *uiwidgetp, callback_t *uicb)
{
  uikey->presscb = uicb;
  uikey->releasecb = uicb;
  g_signal_connect (uiwidgetp->widget, "key-press-event",
      G_CALLBACK (uiKeyCallback), uikey);
  g_signal_connect (uiwidgetp->widget, "key-release-event",
      G_CALLBACK (uiKeyCallback), uikey);
}

bool
uiKeyIsPressEvent (uikey_t *uikey)
{
  if (uikey == NULL) {
    return KEY_EVENT_NONE;
  }
  return uikey->eventtype == KEY_EVENT_PRESS;
}

bool
uiKeyIsReleaseEvent (uikey_t *uikey)
{
  if (uikey == NULL) {
    return KEY_EVENT_NONE;
  }
  return uikey->eventtype == KEY_EVENT_RELEASE;
}

bool
uiKeyIsMovementKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  /* the up and down arrows are spinbox increment controls */
  /* page up and down are spinbox increment controls */
  /* key up/down, page up/down are selection movement controls */
  if (uikey->keyval == GDK_KEY_Up ||
      uikey->keyval == GDK_KEY_KP_Up ||
      uikey->keyval == GDK_KEY_Down ||
      uikey->keyval == GDK_KEY_KP_Down ||
      uikey->keyval == GDK_KEY_Page_Up ||
      uikey->keyval == GDK_KEY_KP_Page_Up ||
      uikey->keyval == GDK_KEY_Page_Down ||
      uikey->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsKey (uikey_t *uikey, unsigned char keyval)
{
  bool  rc = false;
  guint tval1, tval2;

  if (uikey == NULL) {
    return rc;
  }

  tval1 = GDK_KEY_A - 'A' + toupper (keyval);
  tval2 = GDK_KEY_a - 'a' + tolower (keyval);

  if (uikey->keyval == tval1 || uikey->keyval == tval2) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPlayKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_AudioPlay) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPauseKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_AudioPause) {
    rc = true;
  }
  if (uikey->keyval == GDK_KEY_AudioStop) {
    rc = true;
  }


  return rc;
}

bool
uiKeyIsAudioNextKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_AudioNext) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPrevKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_AudioPrev) {
    rc = true;
  }

  return rc;
}

/* includes page up */
bool
uiKeyIsUpKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_Up ||
      uikey->keyval == GDK_KEY_KP_Up ||
      uikey->keyval == GDK_KEY_Page_Up ||
      uikey->keyval == GDK_KEY_KP_Page_Up
      ) {
    rc = true;
  }

  return rc;
}

/* includes page down */
bool
uiKeyIsDownKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_Down ||
      uikey->keyval == GDK_KEY_KP_Down ||
      uikey->keyval == GDK_KEY_Page_Down ||
      uikey->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsPageUpDownKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_Page_Up ||
      uikey->keyval == GDK_KEY_KP_Page_Up ||
      uikey->keyval == GDK_KEY_Page_Down ||
      uikey->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsNavKey (uikey_t *uikey)
{
  bool  rc = false;

  if (uikey == NULL) {
    return rc;
  }

  /* tab and left tab are navigation controls */
  if (uikey->keyval == GDK_KEY_Tab ||
      uikey->keyval == GDK_KEY_KP_Tab ||
      uikey->keyval == GDK_KEY_ISO_Left_Tab
      ) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsMaskedKey (uikey_t *uikey)
{
  if (uikey == NULL) {
    return false;
  }
  return uikey->ismaskedkey;
}

bool
uiKeyIsControlPressed (uikey_t *uikey)
{
  if (uikey == NULL) {
    return false;
  }
  return uikey->controlpressed;
}

bool
uiKeyIsShiftPressed (uikey_t *uikey)
{
  if (uikey == NULL) {
    return false;
  }
  return uikey->shiftpressed;
}

/* internal routines */

static gboolean
uiKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata)
{
  uikey_t *uikey = udata;
  guint   keyval;
  int     rc = UICB_CONT;

  gdk_event_get_keyval ((GdkEvent *) event, &keyval);
  uikey->keyval = keyval;

  uikey->eventtype = KEY_EVENT_NONE;
  if (event->type == GDK_KEY_PRESS) {
    uikey->eventtype = KEY_EVENT_PRESS;
  }
  if (event->type == GDK_KEY_RELEASE) {
    uikey->eventtype = KEY_EVENT_RELEASE;
  }

  if (event->type == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uikey->shiftpressed = true;
  }

  if (event->type == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uikey->controlpressed = true;
  }

  if (event->type == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uikey->shiftpressed = false;
  }
  if (event->type == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uikey->controlpressed = false;
  }

  uikey->ismaskedkey = false;
  if ((event->state & GDK_CONTROL_MASK) ||
      (event->state & GDK_META_MASK) ||
      (event->state & GDK_SUPER_MASK)) {
    uikey->ismaskedkey = true;
  }

  if (event->type == GDK_KEY_PRESS &&
      uikey->presscb != NULL) {
    rc = callbackHandler (uikey->presscb);
  }
  if (event->type == GDK_KEY_RELEASE &&
      uikey->releasecb != NULL) {
    rc = callbackHandler (uikey->releasecb);
  }

  return rc;
}

