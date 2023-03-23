/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "configui.h"
#include "fileop.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"
#include "templateutil.h"

void
confuiPopulateOptions (confuigui_t *gui)
{
  const char  *sval;
  ssize_t     nval;
  nlistidx_t  selidx;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  char        tbuff [MAXPATHLEN];
  long        debug = 0;
  bool        accentcolorchanged = false;
  bool        localechanged = false;
  bool        themechanged = false;
  bool        scalechanged = false;

  logProcBegin (LOG_PROC, "confuiPopulateOptions");
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    int     musicq = 0;
    bool    isqueueitem = false;

    sval = "fail";
    nval = -1;
    dval = -1.0;

    basetype = gui->uiitem [i].basetype;
    outtype = gui->uiitem [i].outtype;

    switch (basetype) {
      case CONFUI_NONE: {
        break;
      }
      case CONFUI_ENTRY: {
        sval = uiEntryGetValue (gui->uiitem [i].entry);
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiSpinboxTextGetValue (gui->uiitem [i].spinbox);
        if (outtype == CONFUI_OUT_STR) {
          if (gui->uiitem [i].sbkeylist != NULL) {
            sval = nlistGetStr (gui->uiitem [i].sbkeylist, nval);
          } else {
            sval = nlistGetStr (gui->uiitem [i].displist, nval);
          }
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiSpinboxGetValue (gui->uiitem [i].uiwidgetp);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiSpinboxGetValue (gui->uiitem [i].uiwidgetp);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiSpinboxTimeGetValue (gui->uiitem [i].spinbox);
        if (i == CONFUI_SPINBOX_Q_STOP_AT_TIME) {
          /* convert to hh:mm */
          nval *= 60;
        }
        break;
      }
      case CONFUI_COLOR: {
        uiColorButtonGetColor (gui->uiitem [i].uiwidgetp,
            tbuff, sizeof (tbuff));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = uiFontButtonGetFont (gui->uiitem [i].uiwidgetp);
        break;
      }
      case CONFUI_SWITCH: {
        nval = uiSwitchGetValue (gui->uiitem [i].uiswitch);
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = uiToggleButtonIsActive (gui->uiitem [i].uiwidgetp);
        break;
      }
      case CONFUI_COMBOBOX: {
        sval = slistGetDataByIdx (gui->uiitem [i].displist,
            gui->uiitem [i].listidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    if (i == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uispinbox_t  *spinbox;

      spinbox = gui->uiitem [i].spinbox;
      if (! uiSpinboxIsChanged (spinbox)) {
        continue;
      }
    }

    if (gui->uiitem [i].bdjoptIdx >= OPT_Q_ACTIVE) {
      musicq = gui->uiitem [CONFUI_SPINBOX_MUSIC_QUEUE].listidx;
      isqueueitem = true;
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
        if (i == CONFUI_SPINBOX_LOCALE) {
          if (strcmp (sysvarsGetStr (SV_LOCALE), sval) != 0) {
            localechanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_ACCENT_COLOR) {
          if (strcmp (bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sval) != 0) {
            accentcolorchanged = true;
          }
        }
        if (i == CONFUI_SPINBOX_UI_THEME) {
          if (strcmp (bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sval) != 0) {
            themechanged = true;
          }
        }
        if (isqueueitem) {
          bdjoptSetStrPerQueue (gui->uiitem [i].bdjoptIdx, sval, musicq);
        } else {
          bdjoptSetStr (gui->uiitem [i].bdjoptIdx, sval);
        }
        break;
      }
      case CONFUI_OUT_NUM: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, nval, musicq);
        } else {
          if (i == CONFUI_WIDGET_UI_SCALE) {
            if (nval != bdjoptGetNum (gui->uiitem [i].bdjoptIdx)) {
              scalechanged = true;
            }
          }
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_DOUBLE: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, dval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, dval);
        }
        break;
      }
      case CONFUI_OUT_BOOL: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, nval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_CB: {
        nlistSetNum (gui->filterDisplaySel,
            nlistGetNum (gui->filterLookup, i), nval);
        break;
      }
      case CONFUI_OUT_DEBUG: {
        if (nval) {
          long  idx;
          long  bitval;

          idx = i - CONFUI_WIDGET_DEBUG_1;
          bitval = 1 << idx;
          debug |= bitval;
        }
        break;
      }
    } /* out type */

    if (i == CONFUI_SPINBOX_LOCALE &&
        localechanged) {
      sysvarsSetStr (SV_LOCALE, sval);
      snprintf (tbuff, sizeof (tbuff), "%.2s", sval);
      sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "locale", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fileopDelete (tbuff);

      /* if the set locale does not match the system or default locale */
      /* save it in the locale file */
      if (strcmp (sval, sysvarsGetStr (SV_LOCALE_SYSTEM)) != 0 &&
          strcmp (sval, "en_GB") != 0) {
        FILE    *fh;

        fh = fileopOpen (tbuff, "w");
        fprintf (fh, "%s\n", sval);
        fclose (fh);
      }
    }

    if (i == CONFUI_WIDGET_UI_SCALE && scalechanged) {
      FILE    *fh;

      pathbldMakePath (tbuff, sizeof (tbuff),
          "scale", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fileopDelete (tbuff);

      fh = fileopOpen (tbuff, "w");
      fprintf (fh, "%d\n", (int) nval);
      fclose (fh);
    }

    if (i == CONFUI_ENTRY_CHOOSE_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (gui->uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME && themechanged) {
      FILE    *fh;

      sval = bdjoptGetStr (gui->uiitem [i].bdjoptIdx);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fh = fileopOpen (tbuff, "w");
      if (sval != NULL) {
        fprintf (fh, "%s\n", sval);
      }
      fclose (fh);
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR &&
        accentcolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      sval = bdjoptGetStr (gui->uiitem [i].bdjoptIdx);
      templateHttpCopy (sval, "bdj4remote.html");
    }
  } /* for each item */

  selidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  confuiDispSaveTable (gui, selidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd (LOG_PROC, "confuiPopulateOptions", "");
}

