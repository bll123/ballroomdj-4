#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "configui.h"
#include "log.h"
#include "ui.h"

void
confuiBuildUIOrganization (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIOrganization");
  uiCreateVertBox (&vbox);

  /* organization */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the audio file organization path */
  confuiMakeItemCombobox (gui, &vbox, &sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  /* CONTEXT: configuration: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (gui, &vbox, &sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  /* CONTEXT: configuration: checkbox: is automatic organization enabled */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Auto Organise"),
      CONFUI_SWITCH_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE), NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIOrganization", "");
}

