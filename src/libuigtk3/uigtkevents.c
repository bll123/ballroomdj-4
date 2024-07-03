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

#include "ui/uievents.h"

typedef struct uievent {
  callback_t    *keypresscb;
  callback_t    *keyreleasecb;
  callback_t    *mbuttonpresscb;
  callback_t    *mbuttonreleasecb;
  callback_t    *scrollcb;
  callback_t    *elcb;
  int           eventtype;
  guint         keyval;
  guint         button;
  void          *savewidget;
  bool          controlpressed : 1;
  bool          shiftpressed : 1;
  bool          altpressed : 1;
  bool          metapressed : 1;
  bool          superpressed : 1;
  bool          hyperpressed : 1;
  bool          ismaskedkey : 1;
  bool          buttonpressed : 1;
  bool          buttonreleased : 1;
} uievent_t;

static gboolean uiEventKeyHandler (GtkWidget *w, GdkEventKey *event, gpointer udata);
static gboolean uiEventButtonHandler (GtkWidget *w, GdkEventButton *event, gpointer udata);
static gboolean uiEventScrollHandler (GtkWidget *w, GdkEventScroll *event, gpointer udata);

uiwcont_t *
uiEventAlloc (void)
{
  uiwcont_t   *uiwidget;
  uievent_t   *uievent;

  uiwidget = uiwcontAlloc ();

  uievent = mdmalloc (sizeof (uievent_t));
  uievent->keypresscb = NULL;
  uievent->keyreleasecb = NULL;
  uievent->mbuttonpresscb = NULL;
  uievent->mbuttonreleasecb = NULL;
  uievent->scrollcb = NULL;
  uievent->elcb = NULL;
  uievent->eventtype = UIEVENT_EV_NONE;
  uievent->controlpressed = false;
  uievent->shiftpressed = false;
  uievent->altpressed = false;
  uievent->metapressed = false;
  uievent->superpressed = false;
  uievent->hyperpressed = false;
  uievent->ismaskedkey = false;
  uievent->buttonpressed = false;
  uievent->buttonreleased = false;

  uiwidget->wbasetype = WCONT_T_KEY;
  uiwidget->wtype = WCONT_T_KEY;
  /* empty widget is used so that the validity check works */
  uiwidget->uidata.widget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  uiwidget->uidata.packwidget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  uiwidget->uiint.uievent = uievent;

  return uiwidget;
}

void
uiEventFree (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-free")) {
    return;
  }

  uievent = uiwidget->uiint.uievent;
  mdfree (uievent);
  /* the container is freed by uiwcontFree() */
}

uiwcont_t *
uiEventCreateEventBox (uiwcont_t *uiwidgetp)
{
  GtkWidget *widget;
  uiwcont_t *wcont;

  widget = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (widget), uiwidgetp->uidata.widget);
  /* the uivirtlist module creates an event box overlaying the */
  /* entire listing. */
  /* the event box must be below the children so that */
  /* entry widgets, et. al. work properly */
  gtk_event_box_set_above_child (GTK_EVENT_BOX (widget), false);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (widget), false);

  gtk_widget_set_can_focus (widget, false);
  gtk_widget_set_focus_on_click (widget, false);

  wcont = uiwcontAlloc ();
  wcont->wbasetype = WCONT_T_BOX;
  wcont->wtype = WCONT_T_EVENT_BOX;
  wcont->uidata.widget = widget;
  wcont->uidata.packwidget = widget;

  return wcont;
}

void
uiEventSetKeyCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uieventwidget, WCONT_T_KEY, "key-set-cb")) {
    return;
  }

  uievent = uieventwidget->uiint.uievent;

  uievent->keypresscb = uicb;
  uievent->keyreleasecb = uicb;
  /* usually, the key masks are already present */
  gtk_widget_add_events (uiwidgetp->uidata.widget,
      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  g_signal_connect (uiwidgetp->uidata.widget, "key-press-event",
      G_CALLBACK (uiEventKeyHandler), uieventwidget);
  g_signal_connect (uiwidgetp->uidata.widget, "key-release-event",
      G_CALLBACK (uiEventKeyHandler), uieventwidget);
}

void
uiEventSetButtonCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uieventwidget, WCONT_T_KEY, "key-set-button-cb")) {
    return;
  }

  uievent = uieventwidget->uiint.uievent;

  uievent->mbuttonpresscb = uicb;
  uievent->mbuttonreleasecb = uicb;
  gtk_widget_add_events (uiwidgetp->uidata.widget,
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (uiwidgetp->uidata.widget, "button-press-event",
      G_CALLBACK (uiEventButtonHandler), uieventwidget);
  g_signal_connect (uiwidgetp->uidata.widget, "button-release-event",
      G_CALLBACK (uiEventButtonHandler), uieventwidget);
}

void
uiEventSetScrollCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uieventwidget, WCONT_T_KEY, "key-set-scroll-cb")) {
    return;
  }

  uievent = uieventwidget->uiint.uievent;

  uievent->scrollcb = uicb;
  gtk_widget_add_events (uiwidgetp->uidata.widget, GDK_SCROLL_MASK);
  g_signal_connect (uiwidgetp->uidata.widget, "scroll-event",
      G_CALLBACK (uiEventScrollHandler), uieventwidget);
}


bool
uiEventIsKeyPressEvent (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-key-press")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;
  rc = uievent->eventtype == UIEVENT_EV_KEY_PRESS;
  return rc;
}

bool
uiEventIsKeyReleaseEvent (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-key-release")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;
  rc = uievent->eventtype == UIEVENT_EV_KEY_RELEASE;
  return rc;
}

bool
uiEventIsButtonPressEvent (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-button-press")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;
  rc = uievent->eventtype == UIEVENT_EV_MBUTTON_PRESS;
  return rc;
}

bool
uiEventIsButtonDoublePressEvent (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-button-press")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;
  rc = uievent->eventtype == UIEVENT_EV_MBUTTON_DOUBLE_PRESS;
  return rc;
}

bool
uiEventIsButtonReleaseEvent (uiwcont_t *uiwidget)   /* KEEP */
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-button-release")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;
  rc = uievent->eventtype == UIEVENT_EV_MBUTTON_RELEASE;
  return rc;
}

bool
uiEventIsMovementKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-move")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  /* the up and down arrows are spinbox increment controls */
  /* page up and down are spinbox increment controls */
  /* key up/down, page up/down are selection movement controls */
  if (uievent->keyval == GDK_KEY_Up ||
      uievent->keyval == GDK_KEY_KP_Up ||
      uievent->keyval == GDK_KEY_Down ||
      uievent->keyval == GDK_KEY_KP_Down ||
      uievent->keyval == GDK_KEY_Page_Up ||
      uievent->keyval == GDK_KEY_KP_Page_Up ||
      uievent->keyval == GDK_KEY_Page_Down ||
      uievent->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsKey (uiwcont_t *uiwidget, unsigned char keyval)
{
  uievent_t   *uievent;
  bool      rc = false;
  guint     tval1, tval2;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-key")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  tval1 = GDK_KEY_A - 'A' + toupper (keyval);
  tval2 = GDK_KEY_a - 'a' + tolower (keyval);

  if (uievent->keyval == tval1 || uievent->keyval == tval2) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsEnterKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-enter")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_ISO_Enter ||
      uievent->keyval == GDK_KEY_KP_Enter ||
      uievent->keyval == GDK_KEY_Return) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsAudioPlayKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-play")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_AudioPlay) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsAudioPauseKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-pause")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_AudioPause) {
    rc = true;
  }
  if (uievent->keyval == GDK_KEY_AudioStop) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsAudioNextKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-next")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_AudioNext) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsAudioPrevKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-aud-prev")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_AudioPrev) {
    rc = true;
  }

  return rc;
}

/* includes page up */
bool
uiEventIsUpKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-up")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_Up ||
      uievent->keyval == GDK_KEY_KP_Up ||
      uievent->keyval == GDK_KEY_Page_Up ||
      uievent->keyval == GDK_KEY_KP_Page_Up
      ) {
    rc = true;
  }

  return rc;
}

/* includes page down */
bool
uiEventIsDownKey (uiwcont_t *uiwidget)    /* KEEP */
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-down")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_Down ||
      uievent->keyval == GDK_KEY_KP_Down ||
      uievent->keyval == GDK_KEY_Page_Down ||
      uievent->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsPageUpDownKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-page-updown")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (uievent->keyval == GDK_KEY_Page_Up ||
      uievent->keyval == GDK_KEY_KP_Page_Up ||
      uievent->keyval == GDK_KEY_Page_Down ||
      uievent->keyval == GDK_KEY_KP_Page_Down
      ) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsNavKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-nav")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  /* tab and left tab are navigation controls */
  if (uievent->keyval == GDK_KEY_Tab ||
      uievent->keyval == GDK_KEY_KP_Tab ||
      uievent->keyval == GDK_KEY_ISO_Left_Tab
      ) {
    rc = true;
  }

  return rc;
}

bool
uiEventIsPasteCutKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  bool      rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-paste-cut")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (! uievent->controlpressed) {
    return rc;
  }

  if (uievent->keyval == GDK_KEY_V ||
      uievent->keyval == GDK_KEY_v ||
      uievent->keyval == GDK_KEY_X ||
      uievent->keyval == GDK_KEY_x) {
    rc = true;
  }

  if (uievent->shiftpressed) {
    if (uievent->keyval == GDK_KEY_Insert ||
        uievent->keyval == GDK_KEY_KP_Insert ||
        uievent->keyval == GDK_KEY_Delete ||
        uievent->keyval == GDK_KEY_KP_Delete) {
      rc = true;
    }
  }

  return rc;
}

bool
uiEventIsMaskedKey (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-masked")) {
    return false;
  }

  uievent = uiwidget->uiint.uievent;
  return uievent->ismaskedkey;
}

bool
uiEventIsAltPressed (uiwcont_t *uiwidget)     /* KEEP */
{
  uievent_t   *uievent;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-alt")) {
    return false;
  }

  uievent = uiwidget->uiint.uievent;
  return uievent->altpressed;
}

bool
uiEventIsControlPressed (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-control")) {
    return false;
  }

  uievent = uiwidget->uiint.uievent;
  return uievent->controlpressed;
}

bool
uiEventIsShiftPressed (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-is-shift")) {
    return false;
  }

  uievent = uiwidget->uiint.uievent;
  return uievent->shiftpressed;
}

int
uiEventButtonPressed (uiwcont_t *uiwidget)
{
  uievent_t   *uievent;
  int       rc = -1;

  if (! uiwcontValid (uiwidget, WCONT_T_KEY, "key-button-pressed")) {
    return rc;
  }

  uievent = uiwidget->uiint.uievent;

  if (! uievent->buttonpressed && ! uievent->buttonreleased) {
    return rc;
  }

  rc = uievent->button;

  return rc;
}


/* internal routines */

static gboolean
uiEventKeyHandler (GtkWidget *w, GdkEventKey *event, gpointer udata)
{
  uiwcont_t *uiwidget = udata;
  uievent_t *uievent;
  guint     keyval;
  guint     ttype;
  int       rc = UICB_CONT;
  bool      skip = false;
  GdkModifierType state;

  uievent = uiwidget->uiint.uievent;

  gdk_event_get_keyval ((GdkEvent *) event, &keyval);
  uievent->keyval = keyval;

  uievent->eventtype = UIEVENT_EV_NONE;
  ttype = gdk_event_get_event_type ((GdkEvent *) event);
  if (ttype == GDK_KEY_PRESS) {
    uievent->eventtype = UIEVENT_EV_KEY_PRESS;
  }
  if (ttype == GDK_KEY_RELEASE) {
    uievent->eventtype = UIEVENT_EV_KEY_RELEASE;
  }

  if (uievent->eventtype == UIEVENT_EV_NONE) {
    return rc;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uievent->shiftpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uievent->shiftpressed = false;
    skip = true;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uievent->controlpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uievent->controlpressed = false;
    skip = true;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R)) {
    uievent->altpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R)) {
    uievent->altpressed = false;
    skip = true;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Meta_L || keyval == GDK_KEY_Meta_R)) {
    uievent->metapressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Meta_L || keyval == GDK_KEY_Meta_R)) {
    uievent->metapressed = false;
    skip = true;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R)) {
    uievent->superpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R)) {
    uievent->superpressed = false;
    skip = true;
  }

  if (ttype == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Hyper_L || keyval == GDK_KEY_Hyper_R)) {
    uievent->hyperpressed = true;
    skip = true;
  }
  if (ttype == GDK_KEY_RELEASE &&
      (keyval == GDK_KEY_Hyper_L || keyval == GDK_KEY_Hyper_R)) {
    uievent->hyperpressed = false;
    skip = true;
  }

  uievent->ismaskedkey = false;
  /* do not test for shift, it is not handled as a mask */
  gdk_event_get_state ((GdkEvent *) event, &state);
  if (((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) ||
      ((state & GDK_META_MASK) == GDK_META_MASK) ||
      ((state & GDK_SUPER_MASK) == GDK_SUPER_MASK) ||
      ((state & GDK_HYPER_MASK) == GDK_HYPER_MASK)) {
    uievent->ismaskedkey = true;
    skip = true;
  }

  if (skip) {
    /* a press or release of a mask key does not need */
    /* to be processed.  this key handler gets called twice, and processing */
    /* a mask key will cause issues with the callbacks */
    return rc;
  }

  if (ttype == GDK_KEY_PRESS &&
      uievent->keypresscb != NULL) {
    rc = callbackHandler (uievent->keypresscb);
  }
  if (ttype == GDK_KEY_RELEASE &&
      uievent->keyreleasecb != NULL) {
    rc = callbackHandler (uievent->keyreleasecb);
  }

  return rc;
}

static gboolean
uiEventButtonHandler (GtkWidget *w, GdkEventButton *event, gpointer udata)
{
  uiwcont_t *uiwidget = udata;
  uievent_t   *uievent;
  guint     button;
  guint     ttype;
  int       rc = UICB_CONT;
  int       dispidx = -1;
  int       colnum = -1;

  uievent = uiwidget->uiint.uievent;

  gdk_event_get_button ((GdkEvent *) event, &button);
  uievent->button = button;
  /* save the widget address for the check-widget function */
  /* this is the only way to determine the row */
  uievent->savewidget = w;

  uievent->eventtype = UIEVENT_EV_NONE;
  ttype = gdk_event_get_event_type ((GdkEvent *) event);
  if (ttype == GDK_BUTTON_PRESS) {
    uievent->eventtype = UIEVENT_EV_MBUTTON_PRESS;
  }
  if (ttype == GDK_2BUTTON_PRESS) {
    uievent->eventtype = UIEVENT_EV_MBUTTON_DOUBLE_PRESS;
  }
  if (ttype == GDK_BUTTON_RELEASE) {
    uievent->eventtype = UIEVENT_EV_MBUTTON_RELEASE;
  }

  if (uievent->eventtype == UIEVENT_EV_NONE) {
    return rc;
  }

  /* figure out which row and column the mouse pointer is in. */
  /* as there is an eventbox overlaying the entire uivirtlist listing */
  /* it's relatively easy to figure out the row and the column. */
  /* only do this for press events. */
  if (ttype == GDK_BUTTON_PRESS || ttype == GDK_2BUTTON_PRESS) {
    double        x, y;
    GtkWidget     *tw;
    GtkAllocation eba;
    GtkAllocation rowa;

    gdk_event_get_coords ((GdkEvent *) event, &x, &y);
    /* the coordinates of the event box or container */
    gtk_widget_get_allocation (w, &eba);

    /* event-box    */
    /*  vbox        */
    /*   hbox (row) */
    /*    col...    */

    tw = w;
    while (GTK_IS_BIN (tw)) {
      tw = gtk_bin_get_child (GTK_BIN (w));
    }
    if (GTK_IS_CONTAINER (tw)) {
      GList *list, *elem;

      list = gtk_container_get_children (GTK_CONTAINER (tw));
      for (elem = list; elem != NULL; elem = elem->next) {
        tw = elem->data;
        gtk_widget_get_allocation (tw, &rowa);
        /* note that dispidx starts at -1, so the first increment points */
        /* at row 0 */
        if (eba.y + (int) y > rowa.y) {
          ++dispidx;
        } else {
          break;
        }

        /* process the x coordinates (only once) to get the column number */
        /* the tw gtk-widget should be pointing at a row-hbox */
        if (elem == list) {
          GList         *clist, *celem;
          GtkWidget     *tcol;
          GtkAllocation cola;

          clist = gtk_container_get_children (GTK_CONTAINER (tw));
          for (celem = clist; celem != NULL; celem = celem->next) {
            tcol = celem->data;
            gtk_widget_get_allocation (tcol, &cola);
            /* note that colnum starts at -1, so the first increment points */
            /* at column 0 */
            if (eba.x + (int) x > cola.x) {
              ++colnum;
            } else {
              break;
            }
          }
        }
      }
    }
  }

  uievent->buttonpressed = false;
  uievent->buttonreleased = false;

  if ((ttype == GDK_BUTTON_PRESS ||
      ttype == GDK_2BUTTON_PRESS) &&
      uievent->mbuttonpresscb != NULL) {
    uievent->buttonpressed = true;
    rc = callbackHandlerII (uievent->mbuttonpresscb, dispidx, colnum);
  }
  if (ttype == GDK_BUTTON_RELEASE &&
      uievent->mbuttonreleasecb != NULL) {
    uievent->buttonreleased = true;
    rc = callbackHandlerII (uievent->mbuttonreleasecb, dispidx, colnum);
  }

  return rc;
}

static gboolean
uiEventScrollHandler (GtkWidget *w, GdkEventScroll *event, gpointer udata)
{
  uiwcont_t *uiwidget = udata;
  uievent_t *uievent;
  guint     ttype;
  int       rc = UICB_STOP;
  GdkScrollDirection gdir;
  int32_t   dir = UIEVENT_DIR_PREV;

  uievent = uiwidget->uiint.uievent;

  uievent->eventtype = UIEVENT_EV_NONE;
  ttype = gdk_event_get_event_type ((GdkEvent *) event);
  if (ttype == GDK_SCROLL) {
    uievent->eventtype = UIEVENT_EV_SCROLL;
    gdk_event_get_scroll_direction ((GdkEvent *) event, &gdir);
  }

  if (uievent->eventtype == UIEVENT_EV_NONE) {
    return rc;
  }

  switch (gdir) {
    case GDK_SCROLL_UP:     { dir = UIEVENT_DIR_PREV; break; }
    case GDK_SCROLL_DOWN:   { dir = UIEVENT_DIR_NEXT; break; }
    case GDK_SCROLL_LEFT:   { dir = UIEVENT_DIR_LEFT; break; }
    case GDK_SCROLL_RIGHT:  { dir = UIEVENT_DIR_RIGHT; break; }
    case GDK_SCROLL_SMOOTH: { return rc; }
  }

  if (ttype == GDK_SCROLL &&
      uievent->scrollcb != NULL) {
    rc = callbackHandlerI (uievent->scrollcb, dir);
  }

  return rc;
}


