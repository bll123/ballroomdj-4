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
#include "tagdef.h"
#include "ui.h"

void
confuiBuildUIiTunes (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIiTunes");
  uiCreateVertBox (&vbox);

  /* filter display */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: itunes settings */
      _("iTunes"), CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);

  confuiMakeItemSwitch (gui, &vbox, &sg,
      /* CONTEXT: configuration: enable itunes support */
      _("Enable iTunes Support"),
      CONFUI_SWITCH_ENABLE_ITUNES, OPT_G_ITUNESSUPPORT,
      bdjoptGetNum (OPT_G_ITUNESSUPPORT), NULL);

  logProcEnd (LOG_PROC, "confuiBuildUIiTunes", "");
}

