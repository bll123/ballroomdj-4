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
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scw), 150);

  tb->buffer = gtk_text_buffer_new (NULL);
  tb->textbox = gtk_text_view_new_with_buffer (tb->buffer);
  gtk_widget_set_size_request (tb->textbox, -1, 400);
  gtk_widget_set_halign (tb->textbox, GTK_ALIGN_FILL);
  gtk_widget_set_valign (tb->textbox, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tb->textbox, TRUE);
  gtk_widget_set_vexpand (tb->textbox, TRUE);
  gtk_container_add (GTK_CONTAINER (scw), tb->textbox);

  tb->scw = scw;
  return tb;
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

static void
uiiutilsTextBoxScrollToEnd (uiutilstextbox_t *tb)
{
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (tb->buffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (tb->textbox),
      &iter, 0, false, 0, 0);
}

