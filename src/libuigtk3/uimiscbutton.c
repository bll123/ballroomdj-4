/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uimiscbutton.h"

uiwcont_t *
uiCreateFontButton (const char *fontname)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *fb;

  if (fontname != NULL && *fontname) {
    fb = gtk_font_button_new_with_font (fontname);
  } else {
    fb = gtk_font_button_new ();
  }
  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_FONT_BUTTON);
  uiwcontSetWidget (uiwidget, fb, NULL);
  return uiwidget;
}

const char *
uiFontButtonGetFont (uiwcont_t *uiwidget)
{
  const char *sval;

  if (! uiwcontValid (uiwidget, WCONT_T_FONT_BUTTON, "font-button-get")) {
    return NULL;
  }

  sval = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (uiwidget->uidata.widget));
  return sval;
}

uiwcont_t *
uiCreateColorButton (const char *color)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *cb;
  GdkRGBA     rgba;

  if (color != NULL && *color) {
    gdk_rgba_parse (&rgba, color);
    cb = gtk_color_button_new_with_rgba (&rgba);
  } else {
    cb = gtk_color_button_new ();
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_COLOR_BUTTON);
  uiwcontSetWidget (uiwidget, cb, NULL);
  return uiwidget;
}

void
uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz)
{
  GdkRGBA     gcolor;

  if (! uiwcontValid (uiwidget, WCONT_T_COLOR_BUTTON, "col-button-get")) {
    return;
  }

  gtk_color_chooser_get_rgba (
      GTK_COLOR_CHOOSER (uiwidget->uidata.widget), &gcolor);
  snprintf (tbuff, sz, "#%02x%02x%02x",
      (int) round (gcolor.red * 255.0),
      (int) round (gcolor.green * 255.0),
      (int) round (gcolor.blue * 255.0));
}

void
uiColorButtonSetColor (uiwcont_t *uiwidget, const char *color)
{
  GdkRGBA     rgba;

  if (! uiwcontValid (uiwidget, WCONT_T_COLOR_BUTTON, "col-button-get")) {
    return;
  }

  gdk_rgba_parse (&rgba, color);
  gtk_color_chooser_set_rgba (
      GTK_COLOR_CHOOSER (uiwidget->uidata.widget), &rgba);
}
