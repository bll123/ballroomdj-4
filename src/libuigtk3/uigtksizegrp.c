#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "log.h"
#include "ui.h"
#include "uiutils.h"

inline void
uiCreateSizeGroupHoriz (UIWidget *uiw)
{
  GtkSizeGroup  *sg;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  uiw->sg = sg;
}

inline void
uiSizeGroupAdd (UIWidget *uiw, GtkWidget *widget)
{
  gtk_size_group_add_widget (uiw->sg, widget);
}
