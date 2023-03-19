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

#include <gtk/gtk.h>

#include "ui/uiinternal.h"
#include "ui/uisizegrp.h"

void
uiCreateSizeGroupHoriz (uiwcont_t *uiw)
{
  GtkSizeGroup  *sg;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  uiw->sg = sg;
}

void
uiSizeGroupAdd (uiwcont_t *uisg, uiwcont_t *uiwidget)
{
  gtk_size_group_add_widget (uisg->sg, uiwidget->widget);
}

