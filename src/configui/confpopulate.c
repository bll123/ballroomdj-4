/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "configui.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "sysvars.h"
#include "templateutil.h"
#include "vsencdec.h"
#include "ui.h"

void
confuiPopulateOptions (confuigui_t *gui)
{
  const char  *sval;
  int64_t     nval;
  nlistidx_t  selidx;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  int32_t     debug = 0;

  logProcBegin ();

  for (int widx = 0; widx < CONFUI_ITEM_MAX; ++widx) {
    int     musicq = 0;
    bool    isqueueitem = false;
    char    tbuff [BDJ4_PATH_MAX];

    sval = "fail";
    nval = -1;
    dval = -1.0;

    gui->uiitem [widx].changed = false;

    basetype = gui->uiitem [widx].basetype;
    outtype = gui->uiitem [widx].outtype;

    switch (basetype) {
      case CONFUI_NONE: {
        break;
      }
      case CONFUI_ENTRY_ENCRYPT:
      case CONFUI_ENTRY: {
        sval = uiEntryGetValue (gui->uiitem [widx].uiwidgetp);
        if (basetype == CONFUI_ENTRY_ENCRYPT) {
          vsencdec (sval, tbuff, sizeof (tbuff));
          sval = tbuff;
        }
        if (widx == CONFUI_ENTRY_CHOOSE_STARTUP ||
            widx == CONFUI_ENTRY_CHOOSE_SHUTDOWN) {
          const char  *tmain;
          size_t      len;

          tmain = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
          len = strlen (tmain);

          if (strncmp (tmain, sval, len) == 0) {
            sval = sval + len + 1;
          }
        }
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiSpinboxTextGetValue (gui->uiitem [widx].uiwidgetp);
        if (outtype == CONFUI_OUT_STR) {
          if (gui->uiitem [widx].sbkeylist != NULL) {
            sval = nlistGetStr (gui->uiitem [widx].sbkeylist, nval);
          } else {
            sval = nlistGetStr (gui->uiitem [widx].displist, nval);
          }
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiSpinboxGetValue (gui->uiitem [widx].uiwidgetp);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiSpinboxGetValue (gui->uiitem [widx].uiwidgetp);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiSpinboxTimeGetValue (gui->uiitem [widx].uiwidgetp);

        /* do some additional validation, as after a failed validation, */
        /* the spinbox value can be junk */
        if (nval < 5000 || nval > 7200000) {
          nval = 0;
        }

        if (widx == CONFUI_SPINBOX_Q_STOP_AT_TIME) {
          /* convert to hh:mm */
          nval *= 60;
        }
        break;
      }
      case CONFUI_COLOR: {
        uiColorButtonGetColor (gui->uiitem [widx].uiwidgetp,
            tbuff, sizeof (tbuff));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = uiFontButtonGetFont (gui->uiitem [widx].uiwidgetp);
        break;
      }
      case CONFUI_SWITCH: {
        nval = uiSwitchGetValue (gui->uiitem [widx].uiwidgetp);
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = uiToggleButtonIsActive (gui->uiitem [widx].uiwidgetp);
        break;
      }
      case CONFUI_DD: {
        /* the listidx is not valid */
        outtype = CONFUI_OUT_NONE;
        break;
      }
    }

    if (widx == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uiwcont_t  *spinbox;

      spinbox = gui->uiitem [widx].uiwidgetp;
      if (! uiSpinboxIsChanged (spinbox)) {
        continue;
      }
    }

    if (gui->uiitem [widx].bdjoptIdx >= OPT_Q_ACTIVE) {
      musicq = gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].listidx;
      isqueueitem = true;
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
        if (widx == CONFUI_WIDGET_UI_ACCENT_COLOR) {
          if (strcmp (bdjoptGetStr (gui->uiitem [widx].bdjoptIdx), sval) != 0) {
            gui->uiitem [widx].changed = true;
          }
        }

        if (isqueueitem) {
          bdjoptSetStrPerQueue (gui->uiitem [widx].bdjoptIdx, sval, musicq);
        } else {
          bdjoptSetStr (gui->uiitem [widx].bdjoptIdx, sval);
          if (widx == CONFUI_SPINBOX_PLI) {
            bdjoptSetStr (OPT_M_PLAYER_INTFC_NM,
                nlistGetStr (gui->uiitem [widx].displist, nval));
          }
        }
        break;
      }
      case CONFUI_OUT_NUM: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [widx].bdjoptIdx, nval, musicq);
        } else {
          if (widx == CONFUI_WIDGET_UI_SCALE) {
            if (nval != bdjoptGetNum (gui->uiitem [widx].bdjoptIdx)) {
              gui->uiitem [widx].changed = true;
            }
          }
          bdjoptSetNum (gui->uiitem [widx].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_DOUBLE: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [widx].bdjoptIdx, dval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [widx].bdjoptIdx, dval);
        }
        break;
      }
      case CONFUI_OUT_BOOL: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [widx].bdjoptIdx, nval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [widx].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_CB: {
        if (widx > CONFUI_WIDGET_FILTER_START &&
            widx < CONFUI_WIDGET_FILTER_MAX) {
          nlistSetNum (gui->filterDisplaySel,
              nlistGetNum (gui->filterLookup, widx), nval);
        }
        if (widx > CONFUI_WIDGET_QUICKEDIT_START &&
            widx < CONFUI_WIDGET_QUICKEDIT_MAX) {
          nlistSetNum (gui->quickeditDisplaySel,
              nlistGetNum (gui->quickeditLookup, widx), nval);
        }
        break;
      }
      case CONFUI_OUT_DEBUG: {
        if (nval) {
          debug |= gui->uiitem [widx].debuglvl;
        }
        break;
      }
    } /* out type */

    if (widx == CONFUI_WIDGET_UI_SCALE && gui->uiitem [widx].changed) {
      FILE    *fh;

      pathbldMakePath (tbuff, sizeof (tbuff),
          "scale", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fileopDelete (tbuff);

      fh = fileopOpen (tbuff, "w");
      fprintf (fh, "%" PRId64 "\n", nval);
      mdextfclose (fh);
      fclose (fh);
    }

    if (widx == CONFUI_ENTRY_CHOOSE_MUSIC_DIR) {
      stpecpy (tbuff, tbuff + sizeof (tbuff), bdjoptGetStr (gui->uiitem [widx].bdjoptIdx));
      pathNormalizePath (tbuff, sizeof (tbuff));
      bdjoptSetStr (gui->uiitem [widx].bdjoptIdx, tbuff);
    }

#if 0
    if (widx == CONFUI_SPINBOX_UI_THEME && gui->uiitem [widx].changed) {
      FILE    *fh;

      sval = bdjoptGetStr (gui->uiitem [widx].bdjoptIdx);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fh = fileopOpen (tbuff, "w");
      if (sval != NULL) {
        fprintf (fh, "%s\n", sval);
      }
      mdextfclose (fh);
      fclose (fh);
    }
#endif

    if (widx == CONFUI_WIDGET_UI_ACCENT_COLOR && gui->uiitem [widx].changed) {
      templateImageCopy (sval);
    }

    if (widx == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      uiwcont_t  *spinbox;

      /* only copy if the spinbox changed */

      spinbox = gui->uiitem [widx].uiwidgetp;
      if (uiSpinboxIsChanged (spinbox)) {
        sval = bdjoptGetStr (gui->uiitem [widx].bdjoptIdx);
        templateHttpCopy (sval, "bdj4remote.html");
      }
    }
  } /* for each item */

  selidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].uiwidgetp);
  confuiDispSaveTable (gui, selidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd ("");
}
