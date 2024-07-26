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

#include "bdj4.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

static void uiButtonSignalHandler (GtkButton *b, gpointer udata);
static void uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata);
static bool uiButtonPressCallback (void *udata);
static bool uiButtonReleaseCallback (void *udata);

typedef struct uibutton {
  GtkWidget   *image;
  mstime_t    repeatTimer;
  int         repeatMS;
  callback_t  *cb;
  callback_t  *presscb;
  callback_t  *releasecb;
  bool        repeatOn;
  bool        repeating;
} uibutton_t;

uiwcont_t *
uiCreateButton (callback_t *uicb, char *title, char *imagenm)
{
  uiwcont_t   *uiwidget;
  uibutton_t  *uibutton;
  GtkWidget   *widget;

  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BUTTON;
  uiwidget->wtype = WCONT_T_BUTTON;

  widget = gtk_button_new ();
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  if (imagenm != NULL) {
    GtkWidget   *image;
    char        tbuff [MAXPATHLEN];

    gtk_button_set_label (GTK_BUTTON (widget), "");
    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    image = gtk_image_new_from_file (tbuff);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_tooltip_text (widget, title);
    gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
    uibutton->image = image;
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }
  if (uicb != NULL) {
    g_signal_connect (widget, "clicked",
        G_CALLBACK (uiButtonSignalHandler), uibutton);
  }

  uibutton->cb = uicb;
  uibutton->presscb = callbackInit (uiButtonPressCallback,
      uiwidget, "button-repeat-press");
  uibutton->releasecb = callbackInit (uiButtonReleaseCallback,
      uiwidget, "button-repeat-release");
  uibutton->repeating = false;
  uibutton->repeatOn = false;
  uibutton->repeatMS = 250;

  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;
  uiwidget->uiint.uibutton = uibutton;

  return uiwidget;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  uibutton_t *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-free")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;

  callbackFree (uibutton->presscb);
  callbackFree (uibutton->releasecb);
  mdfree (uibutton);
}

void
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-pos-r")) {
    return;
  }

  gtk_button_set_image_position (GTK_BUTTON (uiwidget->uidata.widget),
      GTK_POS_RIGHT);
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  uibutton_t    *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-margin-top")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton->image != NULL) {
    gtk_widget_set_margin_top (uibutton->image, uiBaseMarginSz * margin);
  }
}


void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
  GtkWidget *image;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-icon")) {
    return;
  }

  image = gtk_image_new_from_icon_name (nm, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (uiwidget->uidata.widget), image);
  gtk_button_set_always_show_image (
      GTK_BUTTON (uiwidget->uidata.widget), TRUE);
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  GtkWidget *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-align-left")) {
    return;
  }

  /* a regular button is: */
  /*    button / label  */
  /* a button w/image is: */
  /*    button / alignment / box / label, image (or image, label) */
  /* widget is either a label or an alignment */
  widget = gtk_bin_get_child (GTK_BIN (uiwidget->uidata.widget));
  if (GTK_IS_LABEL (widget)) {
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  } else {
    /* this does not seem to work... <sad face> */
    /* there's no way to get the hbox in the alignment to fill the space either */
    gtk_widget_set_halign (widget, GTK_ALIGN_START);
    gtk_widget_set_hexpand (widget, TRUE);
  }
}

void
uiButtonSetReliefNone (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-relief-none")) {
    return;
  }

  gtk_button_set_relief (GTK_BUTTON (uiwidget->uidata.widget), GTK_RELIEF_NONE);
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-flat")) {
    return;
  }

  uiWidgetAddClass (uiwidget, FLATBUTTON_CLASS);
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-text")) {
    return;
  }

  gtk_button_set_label (GTK_BUTTON (uiwidget->uidata.widget), txt);
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  uibutton_t    *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;

  uibutton->repeatMS = repeatms;
  uibutton->repeatOn = true;
  g_signal_connect (uiwidget->uidata.widget, "pressed",
      G_CALLBACK (uiButtonRepeatSignalHandler), uibutton->presscb);
  g_signal_connect (uiwidget->uidata.widget, "released",
      G_CALLBACK (uiButtonRepeatSignalHandler), uibutton->releasecb);
}

bool
uiButtonCheckRepeat (uiwcont_t *uiwidget)
{
  uibutton_t  *uibutton;
  bool        rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return rc;
  }

  uibutton = uiwidget->uiint.uibutton;

  if (uibutton->repeating) {
    if (mstimeCheck (&uibutton->repeatTimer)) {
      if (uibutton->cb != NULL) {
        uibutton->repeating = false;
        callbackHandler (uibutton->cb);
        mstimeset (&uibutton->repeatTimer, uibutton->repeatMS);
        uibutton->repeating = true;
      }
    }
    rc = true;
  }

  return rc;
}

/* internal routines */

static void
uiButtonSignalHandler (GtkButton *b, gpointer udata)
{
  uibutton_t  *uibutton = udata;

  if (uibutton == NULL) {
    return;
  }
  if (uibutton->repeatOn) {
    return;
  }
  if (uibutton->cb == NULL) {
    return;
  }

  callbackHandler (uibutton->cb);
}

static void
uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata)
{
  callback_t *uicb = udata;

  callbackHandler (uicb);
}

static bool
uiButtonPressCallback (void *udata)
{
  uiwcont_t   *uiwidget = udata;
  uibutton_t  *uibutton;
  callback_t  *uicb;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-state")) {
    return UICB_CONT;
  }

  uibutton = uiwidget->uiint.uibutton;
  uicb = uibutton->cb;

  uibutton->repeating = false;
  uicb = uibutton->cb;
  if (uicb == NULL) {
    return UICB_CONT;
  }
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-press");
  if (uicb != NULL) {
    callbackHandler (uicb);
  }
  /* the first time the button is pressed, the repeat timer should have */
  /* a longer initial delay. */
  mstimeset (&uibutton->repeatTimer, uibutton->repeatMS * 3);
  uibutton->repeating = true;
  return UICB_CONT;
}

static bool
uiButtonReleaseCallback (void *udata)
{
  uiwcont_t   *uiwidget = udata;
  uibutton_t  *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-state")) {
    return UICB_CONT;
  }

  uibutton = uiwidget->uiint.uibutton;

  uibutton->repeating = false;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-release");
  return UICB_CONT;
}
