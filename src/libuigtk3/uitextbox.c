/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uitextbox.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uitextboxint {
  uiwcont_t     *scw;
  GtkTextBuffer *buffer;
} uitextboxint_t;

uiwcont_t *
uiTextBoxCreate (int height, const char *hlcolor)
{
  uitextboxint_t  *tbint;
  uiwcont_t       *uiwidget;
  void            *widget;

  uiwidget = uiwcontAlloc (WCONT_T_TEXT_BOX, WCONT_T_TEXT_BOX);

  tbint = mdmalloc (sizeof (uitextboxint_t));
  uiwidget->uiint.uitextbox = tbint;

  tbint->buffer = gtk_text_buffer_new (NULL);

  tbint->scw = uiCreateScrolledWindow (height);

  gtk_text_buffer_create_tag (tbint->buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
  if (hlcolor == NULL) {
    gtk_text_buffer_create_tag (tbint->buffer, "highlight", "weight", PANGO_WEIGHT_BOLD, NULL);
  } else {
    gtk_text_buffer_create_tag (tbint->buffer, "highlight", "foreground", hlcolor, NULL);
  }

  widget = gtk_text_view_new_with_buffer (tbint->buffer);
  uiwcontSetWidget (uiwidget, widget, tbint->scw->packwidget);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (uiwidget->widget), GTK_WRAP_WORD);
  uiWindowPackInWindow (tbint->scw, uiwidget);
  uiWidgetSetAllMargins (uiwidget, 2);
  uiWidgetSetSizeRequest (uiwidget, -1, -1);
  uiWidgetAlignVertStart (uiwidget);

  return uiwidget;
}

void
uiTextBoxFree (uiwcont_t *uiwidget)
{
  uitextboxint_t   *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-free")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;
  uiwcontBaseFree (tbint->scw);
  mdfree (tbint);
}

void
uiTextBoxSetReadonly (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-set-ro")) {
    return;
  }

  uiWidgetDisableFocus (uiwidget);
}

char *
uiTextBoxGetValue (uiwcont_t *uiwidget)
{
  GtkTextIter   siter;
  GtkTextIter   eiter;
  char          *val;
  uitextboxint_t   *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-get-value")) {
    return NULL;
  }

  tbint = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_start_iter (tbint->buffer, &siter);
  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  val = gtk_text_buffer_get_text (tbint->buffer, &siter, &eiter, FALSE);
  mdextalloc (val);
  return val;
}

void
uiTextBoxScrollToEnd (uiwcont_t *uiwidget)
{
  GtkTextIter iter;
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-scroll-to-end")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tbint->buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (uiwidget->widget),
      &iter, 0, false, 0, 0);
}

void
uiTextBoxAppendStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter eiter;
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-str")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  gtk_text_buffer_insert (tbint->buffer, &eiter, str, -1);
}

void
uiTextBoxAppendBoldStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-bold-str")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tbint->buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tbint->buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tbint->buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tbint->buffer, "bold", &siter, &eiter);
  gtk_text_buffer_delete_mark (tbint->buffer, mark);
}

void
uiTextBoxAppendHighlightStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-hl-str")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tbint->buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tbint->buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tbint->buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tbint->buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tbint->buffer, "highlight", &siter, &eiter);
  gtk_text_buffer_delete_mark (tbint->buffer, mark);
}

/* this does not handle any selected text */

void
uiTextBoxSetDarkBG (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-darken")) {
    return;
  }

  uiWidgetSetClass (uiwidget, DARKBG_CLASS);
}

void
uiTextBoxHorizExpand (uiwcont_t *uiwidget)
{
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-horiz-expand")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  uiWidgetExpandHoriz (tbint->scw);
  uiWidgetAlignHorizFill (uiwidget);
  uiWidgetExpandHoriz (uiwidget);
}

void
uiTextBoxVertExpand (uiwcont_t *uiwidget)
{
  uitextboxint_t *tbint;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-vert-expand")) {
    return;
  }

  tbint = uiwidget->uiint.uitextbox;

  uiWidgetExpandVert (tbint->scw);
  uiWidgetAlignVertFill (uiwidget);
  uiWidgetExpandVert (uiwidget);
}

void
uiTextBoxSetHeight (uiwcont_t *uiwidget, int h)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-set-height")) {
    return;
  }

  uiWidgetSetSizeRequest (uiwidget, -1, h);
}

void
uiTextBoxSetParagraph (uiwcont_t *uiwidget, int indent, int interpara)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-set-para")) {
    return;
  }

  gtk_text_view_set_indent (GTK_TEXT_VIEW (uiwidget->widget), indent);
  gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (uiwidget->widget),
      interpara);
}
