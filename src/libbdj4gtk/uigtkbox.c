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

inline void
uiutilsBoxPackStart (GtkWidget *box, GtkWidget *widget)
{
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
}

inline void
uiutilsBoxPackEnd (GtkWidget *box, GtkWidget *widget)
{
  gtk_box_pack_end (GTK_BOX (box), widget, FALSE, FALSE, 0);
}

