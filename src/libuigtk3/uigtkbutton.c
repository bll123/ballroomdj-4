/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
  uiwcont_t   *button;
  mstime_t    repeatTimer;
  int         repeatMS;
  callback_t  *cb;
  callback_t  *presscb;
  callback_t  *releasecb;
  bool        repeatOn;
  bool        repeating;
} uibutton_t;

uibutton_t *
uiCreateButton (callback_t *uicb,
    char *title, char *imagenm)
{
  uibutton_t  *uibutton;
  GtkWidget   *widget;

  uibutton = mdmalloc (sizeof (uibutton_t));

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
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }
  if (uicb != NULL) {
    g_signal_connect (widget, "clicked",
        G_CALLBACK (uiButtonSignalHandler), uibutton);
  }
  uibutton->cb = uicb;
  uibutton->presscb = callbackInit (uiButtonPressCallback,
      uibutton, "button-repeat-press");
  uibutton->releasecb = callbackInit (uiButtonReleaseCallback,
      uibutton, "button-repeat-release");
  uibutton->repeating = false;
  uibutton->repeatOn = false;
  uibutton->repeatMS = 250;

  uibutton->button = uiwcontAlloc ();
  uibutton->button->wtype = WCONT_T_BUTTON;
  uibutton->button->widget = widget;

  return uibutton;
}

void
uiButtonFree (uibutton_t *uibutton)
{
  if (uibutton != NULL) {
    uiwcontFree (uibutton->button);
    callbackFree (uibutton->presscb);
    callbackFree (uibutton->releasecb);
    mdfree (uibutton);
  }
}

uiwcont_t *
uiButtonGetWidgetContainer (uibutton_t *uibutton)
{
  if (uibutton == NULL) {
    return NULL;
  }
  return uibutton->button;
}

void
uiButtonSetImagePosRight (uibutton_t *uibutton)
{
  gtk_button_set_image_position (GTK_BUTTON (uibutton->button->widget),
      GTK_POS_RIGHT);
}

void
uiButtonSetImage (uibutton_t *uibutton, const char *imagenm,
    const char *tooltip)
{
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  gtk_button_set_label (GTK_BUTTON (uibutton->button->widget), "");
  /* relative path */
  pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (uibutton->button->widget), image);
  /* macos needs this */
  gtk_button_set_always_show_image (
      GTK_BUTTON (uibutton->button->widget), TRUE);
  if (tooltip != NULL) {
    gtk_widget_set_tooltip_text (uibutton->button->widget, tooltip);
  }
}

void
uiButtonSetImageIcon (uibutton_t *uibutton, const char *nm)
{
  GtkWidget *image;

  if (uibutton == NULL ||
      uibutton->button == NULL ||
      uibutton->button->widget == NULL) {
    return;
  }

  image = gtk_image_new_from_icon_name (nm, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (uibutton->button->widget), image);
  gtk_button_set_always_show_image (
      GTK_BUTTON (uibutton->button->widget), TRUE);
}

void
uiButtonAlignLeft (uibutton_t *uibutton)
{
  GtkWidget *widget;

  /* a regular button is: */
  /*  button / label  */
  /* a button w/image is: */
  /*  button / alignment / box / label, image (or image, label) */
  /* widget is either a label or an alignment */
  widget = gtk_bin_get_child (GTK_BIN (uibutton->button->widget));
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
uiButtonSetReliefNone (uibutton_t *uibutton)
{
  gtk_button_set_relief (GTK_BUTTON (uibutton->button->widget), GTK_RELIEF_NONE);
}

void
uiButtonSetFlat (uibutton_t *uibutton)
{
  uiWidgetSetClass (uibutton->button, FLATBUTTON_CLASS);
}

void
uiButtonSetText (uibutton_t *uibutton, const char *txt)
{
  gtk_button_set_label (GTK_BUTTON (uibutton->button->widget), txt);
}

void
uiButtonSetRepeat (uibutton_t *uibutton, int repeatms)
{
  uibutton->repeatMS = repeatms;
  uibutton->repeatOn = true;
  g_signal_connect (uibutton->button->widget, "pressed",
      G_CALLBACK (uiButtonRepeatSignalHandler), uibutton->presscb);
  g_signal_connect (uibutton->button->widget, "released",
      G_CALLBACK (uiButtonRepeatSignalHandler), uibutton->releasecb);
}

bool
uiButtonCheckRepeat (uibutton_t *uibutton)
{
  bool    rc = false;

  if (uibutton == NULL) {
    return rc;
  }

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

void
uiButtonSetState (uibutton_t *uibutton, int state)
{
  if (uibutton == NULL) {
    return;
  }
  uiWidgetSetState (uibutton->button, state);
}

/* internal routines */

static inline void
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

static inline void
uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata)
{
  callback_t *uicb = udata;

  callbackHandler (uicb);
}

static bool
uiButtonPressCallback (void *udata)
{
  uibutton_t  *uibutton = udata;
  callback_t  *uicb = uibutton->cb;

  if (uibutton == NULL) {
    return UICB_CONT;
  }

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
  uibutton_t  *uibutton = udata;

  if (uibutton == NULL) {
    return UICB_CONT;
  }

  uibutton->repeating = false;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-release");
  return UICB_CONT;
}
