#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "bdj4intl.h"
#include "conn.h"
#include "pathbld.h"
#include "portability.h"
#include "progstate.h"
#include "uisongsel.h"

uisongselect_t *
uisongselInit (progstate_t *progstate, conn_t *conn)
{
  uisongselect_t    *uisongselect;

  uisongselect = malloc (sizeof (uisongselect_t));
  assert (uisongselect != NULL);
  uisongselect->progstate = progstate;
  uisongselect->conn = conn;

  return uisongselect;
}

void
uisongselFree (uisongselect_t *uisongselect)
{
  if (uisongselect != NULL) {
    free (uisongselect);
  }
}

GtkWidget *
uisongselActivate (uisongselect_t *uisongselect)
{
  GtkWidget *hbox;
  GtkWidget *widget;


  uisongselect->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uisongselect->box != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (uisongselect->box), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uisongselect->box), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uisongselect->box), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  widget = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (widget), "Queue");
  assert (widget != NULL);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  uisongselect->songselTree = gtk_tree_view_new ();
  assert (uisongselect->songselTree != NULL);
  gtk_box_pack_start (GTK_BOX (uisongselect->box),
      GTK_WIDGET (uisongselect->songselTree),
      FALSE, FALSE, 0);
  gtk_widget_set_hexpand (GTK_WIDGET (uisongselect->songselTree), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uisongselect->songselTree), TRUE);

  return uisongselect->box;
}

/* internal routines */

