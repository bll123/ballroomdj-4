#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "ui.h"

void
uiCreateSizeGroupHoriz (UIWidget *uiw)
{
  GtkSizeGroup  *sg;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  uiw->sg = sg;
}

void
uiSizeGroupAdd (UIWidget *uisg, UIWidget *uiwidget)
{
  gtk_size_group_add_widget (uisg->sg, uiwidget->widget);
}

