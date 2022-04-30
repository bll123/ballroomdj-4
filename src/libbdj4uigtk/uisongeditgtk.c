#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "conn.h"
#include "log.h"
#include "slist.h"
#include "tagdef.h"
#include "uisongedit.h"
#include "uiutils.h"

typedef struct {
  GtkWidget           *parentwin;
  GtkWidget           *vbox;
} uisongeditgtk_t;

static void uisongeditAddDisplay (uisongedit_t *songedit, GtkWidget *col, int dispsel);
static void uisongeditAddItem (GtkWidget * hbox, GtkSizeGroup *sg, int tagkey);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  uisongeditgtk_t  *uiw;

  uiw = malloc (sizeof (uisongeditgtk_t));
  uiw->parentwin = NULL;
  uiw->vbox = NULL;
  uisongedit->uiWidgetData = uiw;
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  if (uisongedit->uiWidgetData != NULL) {
    free (uisongedit->uiWidgetData);
    uisongedit->uiWidgetData = NULL;
  }
}

GtkWidget *
uisongeditActivate (uisongedit_t *uisongedit, GtkWidget *parentwin)
{
  uisongeditgtk_t    *uiw;
  GtkWidget         *hbox;
  GtkWidget         *lcol;
  GtkWidget         *rcol;

  logProcBegin (LOG_PROC, "uisongeditActivate");

  uiw = uisongedit->uiWidgetData;
  uiw->parentwin = parentwin;

  uiw->vbox = uiutilsCreateVertBox ();
  gtk_widget_set_hexpand (uiw->vbox, TRUE);

  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_halign (hbox, GTK_ALIGN_FILL);
  uiutilsBoxPackStart (uiw->vbox, hbox);

  lcol = uiutilsCreateVertBox ();
  gtk_widget_set_halign (lcol, GTK_ALIGN_START);
  uiutilsBoxPackStart (hbox, lcol);

  uisongeditAddDisplay (uisongedit, lcol, DISP_SEL_SONGEDIT_A);

  rcol = uiutilsCreateVertBox ();
  gtk_widget_set_halign (rcol, GTK_ALIGN_START);
  gtk_widget_set_hexpand (rcol, TRUE);
  uiutilsBoxPackStart (hbox, rcol);

  uisongeditAddDisplay (uisongedit, rcol, DISP_SEL_SONGEDIT_B);

  logProcEnd (LOG_PROC, "uisongeditActivate", "");
  return uiw->vbox;
}

/* internal routines */

static void
uisongeditAddDisplay (uisongedit_t *uisongedit, GtkWidget *col, int dispsel)
{
  slist_t       *sellist;
  GtkWidget     *hbox;
  char          *keystr;
  slistidx_t    dsiteridx;
  GtkSizeGroup  *sg;

  sellist = dispselGetList (uisongedit->dispsel, dispsel);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  slistStartIterator (sellist, &dsiteridx);
  while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
    int   tagkey;

    tagkey = slistGetNum (sellist, keystr);

    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (col, hbox);
    uisongeditAddItem (hbox, sg, tagkey);
  }
}

static void
uisongeditAddItem (GtkWidget * hbox, GtkSizeGroup *sg, int tagkey)
{
  GtkWidget     *widget;

  if (tagkey >= TAG_KEY_MAX) {
    return;
  }
  if (! tagdefs [tagkey].isEditable) {
    return;
  }

  widget = uiutilsCreateColonLabel (tagdefs [tagkey].displayname);
  uiutilsBoxPackStart (hbox, widget);
  gtk_size_group_add_widget (sg, widget);

  switch (tagkey) {
    /* entry box */
    case TAG_ALBUM:
    case TAG_ALBUMARTIST:
    case TAG_ARTIST:
    case TAG_COMPOSER:
    case TAG_CONDUCTOR:
    case TAG_KEYWORD:
    case TAG_MQDISPLAY:
    case TAG_NOTES:
    case TAG_TAGS:
    case TAG_TITLE: {
      break;
    }
    /* dropdown */
    case TAG_DANCE:
    case TAG_GENRE: {
      break;
    }
    /* spinbox-text */
    case TAG_DANCELEVEL:
    case TAG_DANCERATING:
    case TAG_STATUS: {
      break;
    }
    /* numeric entry box */
    case TAG_BPM: {
      break;
    }
    /* numeric spin box */
    case TAG_DISCNUMBER:
    case TAG_TRACKNUMBER: {
      break;
    }
    /* time spin box */
    case TAG_SONGEND:
    case TAG_SONGSTART: {
      break;
    }
    /* favorite spin box */
    case TAG_FAVORITE: {
      break;
    }
    /* scale */
    case TAG_SPEEDADJUSTMENT:
    case TAG_VOLUMEADJUSTPERC: {
      break;
    }
    default: {
      /* unknown */
      break;
    }
  }
}
