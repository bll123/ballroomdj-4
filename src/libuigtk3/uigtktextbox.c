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

#include "log.h"
#include "ui.h"
#include "uiutils.h"

uitextbox_t *
uiTextBoxCreate (int height)
{
  GtkWidget         *scw;
  uitextbox_t  *tb;

  tb = malloc (sizeof (uitextbox_t));
  assert (tb != NULL);
  tb->scw = NULL;
  tb->textbox = NULL;
  tb->buffer = NULL;

  scw = uiCreateScrolledWindow (height);

  tb->buffer = gtk_text_buffer_new (NULL);
  tb->textbox = gtk_text_view_new_with_buffer (tb->buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (tb->textbox), GTK_WRAP_WORD);
  uiWidgetSetAllMargins (tb->textbox, uiBaseMarginSz * 2);
  gtk_widget_set_size_request (tb->textbox, -1, -1);
  gtk_widget_set_halign (tb->textbox, GTK_ALIGN_FILL);
  gtk_widget_set_valign (tb->textbox, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tb->textbox, FALSE);
  gtk_widget_set_vexpand (tb->textbox, FALSE);
  gtk_container_add (GTK_CONTAINER (scw), tb->textbox);

  tb->scw = scw;
  return tb;
}

void
uiTextBoxFree (uitextbox_t *tb)
{
  if (tb != NULL) {
    free (tb);
  }
}

void
uiTextBoxSetReadonly (uitextbox_t *tb)
{
  gtk_widget_set_can_focus (tb->textbox, FALSE);
}

char *
uiTextBoxGetValue (uitextbox_t *tb)
{
  GtkTextIter   siter;
  GtkTextIter   eiter;
  char          *val;

  gtk_text_buffer_get_start_iter (tb->buffer, &siter);
  gtk_text_buffer_get_end_iter (tb->buffer, &eiter);
  val = gtk_text_buffer_get_text (tb->buffer, &siter, &eiter, FALSE);
  return val;
}

void
uiTextBoxScrollToEnd (uitextbox_t *tb)
{
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (tb->buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (tb->textbox),
      &iter, 0, false, 0, 0);
}

void
uiTextBoxAppendStr (uitextbox_t *tb, const char *str)
{
  GtkTextIter eiter;

  gtk_text_buffer_get_end_iter (tb->buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer, &eiter, str, -1);
}

void
uiTextBoxSetValue (uitextbox_t *tb, const char *str)
{
  GtkTextIter siter;
  GtkTextIter eiter;

  gtk_text_buffer_get_start_iter (tb->buffer, &siter);
  gtk_text_buffer_get_end_iter (tb->buffer, &eiter);
  gtk_text_buffer_delete (tb->buffer, &siter, &eiter);
  gtk_text_buffer_get_end_iter (tb->buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer, &eiter, str, -1);
}

/* this does not handle any selected text */
void
uiTextBoxDarken (uitextbox_t *tb)
{
  uiSetCss (tb->textbox,
      "textview text { background-color: shade(@theme_base_color,0.8); } ");
}

void
uiTextBoxVertExpand (uitextbox_t *tb)
{
  gtk_widget_set_vexpand (tb->scw, TRUE);
  gtk_widget_set_valign (tb->textbox, GTK_ALIGN_FILL);
  gtk_widget_set_vexpand (tb->textbox, TRUE);
}

void
uiTextBoxSetHeight (uitextbox_t *tb, int h)
{
  gtk_widget_set_size_request (tb->textbox, -1, h);
}

