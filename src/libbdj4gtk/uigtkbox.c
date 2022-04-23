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

GtkWidget *
uiutilsCreateVertBox (void)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  return box;
}

GtkWidget *
uiutilsCreateHorizBox (void)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  return box;
}

void
uiutilsBoxSetMargins (GtkWidget *box, int margin)
{
  gtk_widget_set_margin_top (box, margin);
  gtk_widget_set_margin_bottom (box, margin);
  gtk_widget_set_margin_start (box, margin);
  gtk_widget_set_margin_end (box, margin);
}
