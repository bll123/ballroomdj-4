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
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uitextbox.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uitextbox {
  uiwcont_t    *scw;
  uiwcont_t    *buffer;
} uitextbox_t;

uiwcont_t *
uiTextBoxCreate (int height, const char *hlcolor)
{
  uitextbox_t   *tb;
  uiwcont_t     *uiwidget;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_TEXT_BOX;
  uiwidget->wtype = WCONT_T_TEXT_BOX;

  tb = mdmalloc (sizeof (uitextbox_t));
  uiwidget->uiint.uitextbox = tb;

  tb->buffer = uiwcontAlloc ();
  tb->buffer->wbasetype = WCONT_T_TEXT_BUFFER;
  tb->buffer->wtype = WCONT_T_TEXT_BUFFER;
  tb->buffer->uidata.buffer = gtk_text_buffer_new (NULL);

  tb->scw = uiCreateScrolledWindow (height);

  gtk_text_buffer_create_tag (tb->buffer->uidata.buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
  if (hlcolor == NULL) {
    gtk_text_buffer_create_tag (tb->buffer->uidata.buffer, "highlight", "weight", PANGO_WEIGHT_BOLD, NULL);
  } else {
    gtk_text_buffer_create_tag (tb->buffer->uidata.buffer, "highlight", "foreground", hlcolor, NULL);
  }

  uiwidget->uidata.widget = gtk_text_view_new_with_buffer (tb->buffer->uidata.buffer);
  uiwidget->uidata.packwidget = tb->scw->uidata.packwidget;
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (uiwidget->uidata.widget), GTK_WRAP_WORD);
  uiWindowPackInWindow (tb->scw, uiwidget);
  uiWidgetSetAllMargins (uiwidget, 2);
  uiWidgetSetSizeRequest (uiwidget, -1, -1);
  uiWidgetAlignVertStart (uiwidget);

  return uiwidget;
}

void
uiTextBoxFree (uiwcont_t *uiwidget)
{
  uitextbox_t   *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-free")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;
  uiwcontBaseFree (tb->scw);
  uiwcontBaseFree (tb->buffer);
  mdfree (tb);
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
  uitextbox_t   *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-get-value")) {
    return NULL;
  }

  tb = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_start_iter (tb->buffer->uidata.buffer, &siter);
  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  val = gtk_text_buffer_get_text (tb->buffer->uidata.buffer, &siter, &eiter, FALSE);
  mdextalloc (val);
  return val;
}

void
uiTextBoxScrollToEnd (uiwcont_t *uiwidget)
{
  GtkTextIter iter;
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-scroll-to-end")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (uiwidget->uidata.widget),
      &iter, 0, false, 0, 0);
}

void
uiTextBoxAppendStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter eiter;
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-str")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer->uidata.buffer, &eiter, str, -1);
}

void
uiTextBoxAppendBoldStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-bold-str")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tb->buffer->uidata.buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tb->buffer->uidata.buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tb->buffer->uidata.buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tb->buffer->uidata.buffer, "bold", &siter, &eiter);
  gtk_text_buffer_delete_mark (tb->buffer->uidata.buffer, mark);
}

void
uiTextBoxAppendHighlightStr (uiwcont_t *uiwidget, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-append-hl-str")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tb->buffer->uidata.buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tb->buffer->uidata.buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tb->buffer->uidata.buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tb->buffer->uidata.buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tb->buffer->uidata.buffer, "highlight", &siter, &eiter);
  gtk_text_buffer_delete_mark (tb->buffer->uidata.buffer, mark);
}


/* this does not handle any selected text */

void
uiTextBoxSetDarkBG (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-darken")) {
    return;
  }

  uiWidgetAddClass (uiwidget, DARKBG_CLASS);
}

void
uiTextBoxHorizExpand (uiwcont_t *uiwidget)
{
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-horiz-expand")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  uiWidgetExpandHoriz (tb->scw);
  uiWidgetAlignHorizFill (uiwidget);
  uiWidgetExpandHoriz (uiwidget);
}

void
uiTextBoxVertExpand (uiwcont_t *uiwidget)
{
  uitextbox_t *tb;

  if (! uiwcontValid (uiwidget, WCONT_T_TEXT_BOX, "textbox-vert-expand")) {
    return;
  }

  tb = uiwidget->uiint.uitextbox;

  uiWidgetExpandVert (tb->scw);
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

  gtk_text_view_set_indent (GTK_TEXT_VIEW (uiwidget->uidata.widget), indent);
  gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (uiwidget->uidata.widget),
      interpara);
}
