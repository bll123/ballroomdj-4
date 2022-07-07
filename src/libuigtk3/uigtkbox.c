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

#include "ui.h"

void
uiCreateVertBox (UIWidget *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  uiwidget->widget = box;
}

void
uiCreateHorizBox (UIWidget *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  uiwidget->widget = box;
}

inline void
uiBoxPackInWindow (UIWidget *uiwindow, UIWidget *uibox)
{
  gtk_container_add (GTK_CONTAINER (uiwindow->widget), uibox->widget);
}

inline void
uiBoxPackStart (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

inline void
uiBoxPackStartExpand (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

inline void
uiBoxPackEnd (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

inline void
uiBoxPackEndExpand (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

/* these routines will be removed at a later date */

inline void
uiBoxPackInWindowUW (UIWidget *uiwindow, GtkWidget *widget)
{
  gtk_container_add (GTK_CONTAINER (uiwindow->widget), widget);
}

void
uiBoxPackInWindowWU (GtkWidget *window, UIWidget *uibox)
{
  gtk_container_add (GTK_CONTAINER (window), uibox->widget);
}

inline void
uiBoxPackStartUW (UIWidget *uibox, GtkWidget *widget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), widget, FALSE, FALSE, 0);
}

inline void
uiBoxPackStartExpandUW (UIWidget *uibox, GtkWidget *widget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), widget, TRUE, TRUE, 0);
}

inline void
uiBoxPackEndUW (UIWidget *uibox, GtkWidget *widget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), widget, FALSE, FALSE, 0);
}

/* these routines will be removed at a later date */

inline void
uiBoxPackStartWU (GtkWidget *box, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (box), uiwidget->widget, FALSE, FALSE, 0);
}

inline void
uiBoxPackStartExpandWU (GtkWidget *box, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (box), uiwidget->widget, TRUE, TRUE, 0);
}

inline void
uiBoxPackEndWU (GtkWidget *box, UIWidget *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (box), uiwidget->widget, FALSE, FALSE, 0);
}

/* these routines will be removed at a later date */

GtkWidget *
uiCreateVertBoxWW (void)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  return box;
}

GtkWidget *
uiCreateHorizBoxWW (void)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  return box;
}

void
uiBoxPackInWindowWW (GtkWidget *window, GtkWidget *box)
{
  gtk_container_add (GTK_CONTAINER (window), box);
}

inline void
uiBoxPackStartWW (GtkWidget *box, GtkWidget *widget)
{
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
}

inline void
uiBoxPackStartExpandWW (GtkWidget *box, GtkWidget *widget)
{
  gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);
}

inline void
uiBoxPackEndWW (GtkWidget *box, GtkWidget *widget)
{
  gtk_box_pack_end (GTK_BOX (box), widget, FALSE, FALSE, 0);
}

