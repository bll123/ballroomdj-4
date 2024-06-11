/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "mdebug.h"
#include "callback.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uikeys.h"

enum {
  KEY_EVENT_NONE,
  KEY_EVENT_PRESS,
  KEY_EVENT_RELEASE,
  BUTTON_EVENT_PRESS,
  BUTTON_EVENT_DOUBLE_PRESS,
  BUTTON_EVENT_RELEASE,
};

typedef struct uikey {
  callback_t    *presscb;
  callback_t    *releasecb;
  int           eventtype;
  guint         keyval;
  bool          controlpressed : 1;
  bool          shiftpressed : 1;
  bool          altpressed : 1;
  bool          metapressed : 1;
  bool          superpressed : 1;
  bool          hyperpressed : 1;
  bool          ismaskedkey : 1;
  bool          button1press : 1;
  bool          button1release : 1;
  bool          button2press : 1;
  bool          button2release : 1;
  bool          button3press : 1;
  bool          button3release : 1;
} uikey_t;

static gboolean uiKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata);
static gboolean uiButtonCallback (GtkWidget *w, GdkEventButton *event, gpointer udata);

uiwcont_t *
uiKeyAlloc (void)
{
  uiwcont_t *uiwidget;
  uikey_t   *uikey;

  uiwidget = uiwcontAlloc ();

  uikey = mdmalloc (sizeof (uikey_t));
  uikey->presscb = NULL;
  uikey->releasecb = NULL;
  uikey->eventtype = KEY_EVENT_NONE;
  uikey->controlpressed = false;
  uikey->shiftpressed = false;
  uikey->altpressed = false;
  uikey->metapressed = false;
  uikey->superpressed = false;
  uikey->hyperpressed = false;
  uikey->ismaskedkey = false;

  uiwidget->wbasetype = WCONT_T_KEY;
  uiwidget->wtype = WCONT_T_KEY;
  /* empty widget is used so that the validity check works */
  uiwidget->uidata.widget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  uiwidget->uidata.packwidget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  uiwidget->uiint.uikey = uikey;

  return uiwidget;
}

void
uiKeyFree (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-free")) {
    return;
  }

  uikey = uiwidget->uiint.uikey;
  mdfree (uikey);
  /* the container is freed by uiwcontFree() */
}

uiwcont_t *
uiKeyCreateEventBox (uiwcont_t *uiwidgetp)
{
  GtkWidget *widget;
  uiwcont_t *wcont;

  widget = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (widget), uiwidgetp->uidata.widget);

  wcont = uiwcontAlloc ();
  wcont->wbasetype = WCONT_T_EVENT_BOX;
  wcont->wtype = WCONT_T_EVENT_BOX;
  wcont->uidata.widget = widget;
  wcont->uidata.packwidget = widget;

  return wcont;
}

void
uiKeySetKeyCallback (uiwcont_t *uikeywidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uikeywidget, WCONT_T_KEY, "key-set-cb")) {
    return;
  }

  uikey = uikeywidget->uiint.uikey;

  uikey->presscb = uicb;
  uikey->releasecb = uicb;
  g_signal_connect (uiwidgetp->uidata.widget, "key-press-event",
      G_CALLBACK (uiKeyCallback), uikeywidget);
  g_signal_connect (uiwidgetp->uidata.widget, "key-release-event",
      G_CALLBACK (uiKeyCallback), uikeywidget);
}

void
uiKeySetButtonCallback (uiwcont_t *uikeywidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uikeywidget, WCONT_T_KEY, "key-set-button-cb")) {
    return;
  }

  uikey = uikeywidget->uiint.uikey;

  uikey->presscb = uicb;
  uikey->releasecb = uicb;
  gtk_widget_add_events (uiwidgetp->uidata.widget, GDK_BUTTON_PRESS_MASK);
  gtk_widget_add_events (uiwidgetp->uidata.widget, GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (uiwidgetp->uidata.widget, "button-press-event",
      G_CALLBACK (uiButtonCallback), uikeywidget);
  g_signal_connect (uiwidgetp->uidata.widget, "button-release-event",
      G_CALLBACK (uiButtonCallback), uikeywidget);
}

bool
uiKeyIsPressEvent (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-press")) {
    return KEY_EVENT_NONE;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->eventtype == KEY_EVENT_PRESS;
}

bool
uiKeyIsReleaseEvent (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-release")) {
    return KEY_EVENT_NONE;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->eventtype == KEY_EVENT_RELEASE;
}

bool
uiKeyIsMovementKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-move")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

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
uiKeyIsKey (uiwcont_t *uiwidget, unsigned char keyval)
{
  uikey_t   *uikey;
  bool      rc = false;
  guint     tval1, tval2;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-key")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  tval1 = GDK_KEY_A - 'A' + toupper (keyval);
  tval2 = GDK_KEY_a - 'a' + tolower (keyval);

  if (uikey->keyval == tval1 || uikey->keyval == tval2) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsEnterKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-enter")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (uikey->keyval == GDK_KEY_ISO_Enter ||
      uikey->keyval == GDK_KEY_KP_Enter ||
      uikey->keyval == GDK_KEY_Return) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPlayKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-play")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (uikey->keyval == GDK_KEY_AudioPlay) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPauseKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-pause")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (uikey->keyval == GDK_KEY_AudioPause) {
    rc = true;
  }
  if (uikey->keyval == GDK_KEY_AudioStop) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioNextKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-next")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (uikey->keyval == GDK_KEY_AudioNext) {
    rc = true;
  }

  return rc;
}

bool
uiKeyIsAudioPrevKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-prev")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (uikey->keyval == GDK_KEY_AudioPrev) {
    rc = true;
  }

  return rc;
}

/* includes page up */
bool
uiKeyIsUpKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-up")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

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
uiKeyIsDownKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-down")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

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
uiKeyIsPageUpDownKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-page-updown")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

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
uiKeyIsNavKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-nav")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

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
uiKeyIsPasteCutKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-paste-cut")) {
    return rc;
  }

  uikey = uiwidget->uiint.uikey;

  if (! uikey->controlpressed) {
    return rc;
  }

  if (uikey->keyval == GDK_KEY_V ||
      uikey->keyval == GDK_KEY_v ||
      uikey->keyval == GDK_KEY_X ||
      uikey->keyval == GDK_KEY_x) {
    rc = true;
  }

  if (uikey->shiftpressed) {
    if (uikey->keyval == GDK_KEY_Insert ||
        uikey->keyval == GDK_KEY_KP_Insert ||
        uikey->keyval == GDK_KEY_Delete ||
        uikey->keyval == GDK_KEY_KP_Delete) {
      rc = true;
    }
  }

  return rc;
}

bool
uiKeyIsMaskedKey (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-masked")) {
    return false;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->ismaskedkey;
}

bool
uiKeyIsAltPressed (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-alt")) {
    return false;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->altpressed;
}

bool
uiKeyIsControlPressed (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-control")) {
    return false;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->controlpressed;
}

bool
uiKeyIsShiftPressed (uiwcont_t *uiwidget)
{
  uikey_t   *uikey;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-shift")) {
    return false;
  }

  uikey = uiwidget->uiint.uikey;
  return uikey->shiftpressed;
}

bool
uiKeyButtonPressed (uiwcont_t *uiwidget, uint button)
{
  uikey_t   *uikey;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-button-pressed")) {
    return false;
  }

  uikey = uiwidget->uiint.uikey;
  if (uikey->keyval == button) {
    rc = true;
  }

  return rc;
}

/* internal routines */

static gboolean
uiKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata)
{
  uiwcont_t *uiwidget = udata;
  uikey_t   *uikey;
  guint     keyval;
  guint     ttype;
  int       rc = UICB_CONT;
  bool      skip = false;

  gdk_event_get_keyval ((GdkEvent *) event, &keyval);
  uikey = uiwidget->uiint.uikey;
  uikey->keyval = keyval;

  uikey->eventtype = KEY_EVENT_NONE;
  ttype = gdk_event_get_event_type ((GdkEvent *) event);
  if (ttype == GDK_KEY_PRESS) {
    uikey->eventtype = KEY_EVENT_PRESS;
  }
  if (ttype == GDK_KEY_RELEASE) {
    uikey->eventtype = KEY_EVENT_RELEASE;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uikey->shiftpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uikey->shiftpressed = false;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uikey->controlpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uikey->controlpressed = false;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R)) {
    uikey->altpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R)) {
    uikey->altpressed = false;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Meta_L || keyval == GDK_KEY_Meta_R)) {
    uikey->metapressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Meta_L || keyval == GDK_KEY_Meta_R)) {
    uikey->metapressed = false;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R)) {
    uikey->superpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R)) {
    uikey->superpressed = false;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Hyper_L || keyval == GDK_KEY_Hyper_R)) {
    uikey->hyperpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Hyper_L || keyval == GDK_KEY_Hyper_R)) {
    uikey->hyperpressed = false;
  }

  uikey->ismaskedkey = false;
  /* do not test for shift */
  if (((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) ||
      ((event->state & GDK_META_MASK) == GDK_META_MASK) ||
      ((event->state & GDK_SUPER_MASK) == GDK_SUPER_MASK) ||
      ((event->state & GDK_HYPER_MASK) == GDK_HYPER_MASK)) {
    uikey->ismaskedkey = true;
  }

  if (skip) {
    /* a press of a mask key does not need */
    /* to be processed.  this key handler gets called twice, and processing */
    /* a mask key will cause issues with the callbacks */
    return rc;
  }

  if (ttype == GDK_KEY_PRESS &&
      uikey->presscb != NULL) {
    rc = callbackHandler (uikey->presscb);
  }
  if (ttype == GDK_KEY_RELEASE &&
      uikey->releasecb != NULL) {
    rc = callbackHandler (uikey->releasecb);
  }

  return rc;
}

static gboolean
uiButtonCallback (GtkWidget *w, GdkEventButton *event, gpointer udata)
{
  uiwcont_t *uiwidget = udata;
  uikey_t   *uikey;
  guint     button;
  guint     ttype;
  int       rc = UICB_CONT;
  bool      skip = false;

  uikey = uiwidget->uiint.uikey;

  gdk_event_get_button ((GdkEvent *) event, &button);
  uikey->keyval = button;

  uikey->eventtype = KEY_EVENT_NONE;
  ttype = gdk_event_get_event_type ((GdkEvent *) event);
  if (ttype == GDK_BUTTON_PRESS) {
    uikey->eventtype = BUTTON_EVENT_PRESS;
  }
  if (ttype == GDK_2BUTTON_PRESS) {
    uikey->eventtype = BUTTON_EVENT_DOUBLE_PRESS;
  }
  if (ttype == GDK_BUTTON_RELEASE) {
    uikey->eventtype = BUTTON_EVENT_RELEASE;
  }

  if (skip) {
    /* a press of a mask key does not need */
    /* to be processed.  this key handler gets called twice, and processing */
    /* a mask key will cause issues with the callbacks */
    return rc;
  }

  if ((ttype == GDK_BUTTON_PRESS ||
      ttype == GDK_2BUTTON_PRESS) &&
      uikey->presscb != NULL) {
    rc = callbackHandler (uikey->presscb);
  }
  if (ttype == GDK_BUTTON_RELEASE &&
      uikey->releasecb != NULL) {
    rc = callbackHandler (uikey->releasecb);
  }

  return rc;
}

