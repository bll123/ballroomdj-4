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
#include "uiutils.h"

uiutilstextbox_t *
uiutilsTextBoxCreate (void)
{
  GtkWidget         *scw;
  uiutilstextbox_t  *tb;

  tb = malloc (sizeof (uiutilstextbox_t));
  assert (tb != NULL);
  tb->scw = NULL;
  tb->textbox = NULL;
  tb->buffer = NULL;

  scw = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scw), 60);

  tb->buffer = gtk_text_buffer_new (NULL);
  tb->textbox = gtk_text_view_new_with_buffer (tb->buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (tb->textbox), GTK_WRAP_WORD);
  gtk_widget_set_margin_top (tb->textbox, uiutilsBaseMarginSz * 2);
  gtk_widget_set_margin_bottom (tb->textbox, uiutilsBaseMarginSz * 2);
  gtk_widget_set_margin_start (tb->textbox, uiutilsBaseMarginSz * 2);
  gtk_widget_set_margin_end (tb->textbox, uiutilsBaseMarginSz * 2);
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
uiutilsTextBoxFree (uiutilstextbox_t *tb)
{
  if (tb != NULL) {
    free (tb);
  }
}

void
uiutilsTextBoxSetReadonly (uiutilstextbox_t *tb)
{
  gtk_widget_set_can_focus (tb->textbox, FALSE);
}

char *
uiutilsTextBoxGetValue (uiutilstextbox_t *tb)
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
uiutilsTextBoxScrollToEnd (uiutilstextbox_t *tb)
{
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (tb->buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (tb->textbox),
      &iter, 0, false, 0, 0);
}

void
uiutilsTextBoxAppendStr (uiutilstextbox_t *tb, const char *str)
{
  GtkTextIter eiter;

  gtk_text_buffer_get_end_iter (tb->buffer, &eiter);
  gtk_text_buffer_insert (tb->buffer, &eiter, str, -1);
}

void
uiutilsTextBoxSetValue (uiutilstextbox_t *tb, const char *str)
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
uiutilsTextBoxDarken (uiutilstextbox_t *tb)
{
  uiutilsSetCss (tb->textbox,
      "textview text { background-color: shade(@theme_base_color,0.8); } ");
}

void
uiutilsTextBoxVertExpand (uiutilstextbox_t *tb)
{
  gtk_widget_set_vexpand (tb->scw, TRUE);
  gtk_widget_set_valign (tb->textbox, GTK_ALIGN_FILL);
  gtk_widget_set_vexpand (tb->textbox, TRUE);
}

void
uiutilsTextBoxSetHeight (uiutilstextbox_t *tb, int h)
{
  gtk_widget_set_size_request (tb->textbox, -1, h);
}

