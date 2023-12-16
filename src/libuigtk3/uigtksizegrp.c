/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uisizegrp.h"

uiwcont_t *
uiCreateSizeGroupHoriz (void)
{
  uiwcont_t     *szgrp;
  GtkSizeGroup  *sg;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  szgrp = uiwcontAlloc ();
  szgrp->wbasetype = WCONT_T_SIZE_GROUP;
  szgrp->wtype = WCONT_T_SIZE_GROUP;
  szgrp->sg = sg;
  return szgrp;
}

void
uiSizeGroupAdd (uiwcont_t *uisg, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uisg, WCONT_T_SIZE_GROUP, "sg-add")) {
    return;
  }

  gtk_size_group_add_widget (uisg->sg, uiwidget->widget);
}

