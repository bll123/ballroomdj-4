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
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiinternal.h"

#include "ui/uibox.h"
#include "ui/uitextbox.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uitextbox {
  uiwcont_t    *scw;
  uiwcont_t    *textbox;
  uiwcont_t    *buffer;
} uitextbox_t;

uitextbox_t *
uiTextBoxCreate (int height, const char *hlcolor)
{
  uitextbox_t   *tb;

  tb = mdmalloc (sizeof (uitextbox_t));
  assert (tb != NULL);
  tb->scw = NULL;
  tb->textbox = uiwcontAlloc ();
  tb->buffer = uiwcontAlloc ();

  tb->scw = uiCreateScrolledWindow (height);

  tb->buffer->buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_create_tag (tb->buffer->buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
  if (hlcolor == NULL) {
    gtk_text_buffer_create_tag (tb->buffer->buffer, "highlight", "weight", PANGO_WEIGHT_BOLD, NULL);
  } else {
    gtk_text_buffer_create_tag (tb->buffer->buffer, "highlight", "foreground", hlcolor, NULL);
  }
  tb->textbox->widget = gtk_text_view_new_with_buffer (tb->buffer->buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (tb->textbox->widget), GTK_WRAP_WORD);
  uiWidgetSetAllMargins (tb->textbox, 2);
  uiWidgetSetSizeRequest (tb->textbox, -1, -1);
  uiWidgetAlignVertStart (tb->textbox);
  uiBoxPackInWindow (tb->scw, tb->textbox);

  return tb;
}

void
uiTextBoxFree (uitextbox_t *tb)
{
  if (tb != NULL) {
    uiwcontFree (tb->scw);
    uiwcontFree (tb->textbox);
    uiwcontFree (tb->buffer);
    mdfree (tb);
  }
}

uiwcont_t *
uiTextBoxGetScrolledWindow (uitextbox_t *tb)
{
  return tb->scw;
}

void
uiTextBoxSetReadonly (uitextbox_t *tb)
{
  uiWidgetDisableFocus (tb->textbox);
}

char *
uiTextBoxGetValue (uitextbox_t *tb)
{
  GtkTextIter   siter;
  GtkTextIter   eiter;
  char          *val;

  gtk_text_buffer_get_start_iter (tb->buffer->buffer, &siter);
  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  val = gtk_text_buffer_get_text (tb->buffer->buffer, &siter, &eiter, FALSE);
  mdextalloc (val);
  return val;
}

void
uiTextBoxScrollToEnd (uitextbox_t *tb)
{
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (tb->textbox->widget),
      &iter, 0, false, 0, 0);
}

void
uiTextBoxAppendStr (uitextbox_t *tb, const char *str)
{
  GtkTextIter eiter;

  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer->buffer, &eiter, str, -1);
}

void
uiTextBoxAppendBoldStr (uitextbox_t *tb, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;

  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tb->buffer->buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tb->buffer->buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tb->buffer->buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tb->buffer->buffer, "bold", &siter, &eiter);
  gtk_text_buffer_delete_mark (tb->buffer->buffer, mark);
}

void
uiTextBoxAppendHighlightStr (uitextbox_t *tb, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;
  GtkTextMark *mark;

  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  mark = gtk_text_buffer_create_mark (tb->buffer->buffer, NULL, &eiter, TRUE);
  gtk_text_buffer_insert (tb->buffer->buffer, &eiter, str, -1);
  gtk_text_buffer_get_iter_at_mark (tb->buffer->buffer, &siter, mark);
  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  gtk_text_buffer_apply_tag_by_name (tb->buffer->buffer, "highlight", &siter, &eiter);
  gtk_text_buffer_delete_mark (tb->buffer->buffer, mark);
}

void
uiTextBoxSetValue (uitextbox_t *tb, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;

  gtk_text_buffer_get_start_iter (tb->buffer->buffer, &siter);
  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  gtk_text_buffer_delete (tb->buffer->buffer, &siter, &eiter);
  gtk_text_buffer_get_end_iter (tb->buffer->buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer->buffer, &eiter, str, -1);
}

/* this does not handle any selected text */

void
uiTextBoxDarken (uitextbox_t *tb)
{
  uiWidgetSetClass (tb->textbox, TEXTBOX_DARK_CLASS);
}

void
uiTextBoxHorizExpand (uitextbox_t *tb)
{
  uiWidgetExpandHoriz (tb->scw);
  uiWidgetAlignHorizFill (tb->textbox);
  uiWidgetExpandHoriz (tb->textbox);
}

void
uiTextBoxVertExpand (uitextbox_t *tb)
{
  uiWidgetExpandVert (tb->scw);
  uiWidgetAlignVertFill (tb->textbox);
  uiWidgetExpandVert (tb->textbox);
}

void
uiTextBoxSetHeight (uitextbox_t *tb, int h)
{
  uiWidgetSetSizeRequest (tb->textbox, -1, h);
}

