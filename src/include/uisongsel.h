#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "level.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "rating.h"
#include "songfilter.h"
#include "sortopt.h"
#include "status.h"
#include "uilevel.h"
#include "uirating.h"
#include "uiutils.h"

typedef struct uisongselgtk uisongselgtk_t;

typedef struct {
  const char        *tag;
  conn_t            *conn;
  ssize_t           idxStart;
  ssize_t           oldIdxStart;
  ilistidx_t        danceIdx;
  songfilter_t      *songfilter;
  rating_t          *ratings;
  level_t           *levels;
  status_t          *status;
  nlist_t           *options;
  sortopt_t         *sortopt;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  dispselsel_t      dispselType;
  double            dfilterCount;
  UIWidget          *windowp;
  /* filter data */
  UIWidget          filterDialog;
  UICallback        filtercb;
  songfilterpb_t    dfltpbflag;
  uidropdown_t      sortbysel;
  uidropdown_t      filterdancesel;
  uidropdown_t      filtergenresel;
  uientry_t         *searchentry;
  uirating_t        *uirating;
  uilevel_t         *uilevel;
  uispinbox_t       *filterstatussel;
  uispinbox_t       *filterfavoritesel;
  time_t            filterApplied;
  uiswitch_t        *playstatusswitch;
  /* song selection tab */
  uidropdown_t dancesel;
  /* widget data */
  uisongselgtk_t    *uiWidgetData;
} uisongsel_t;

/* uisongsel.c */
uisongsel_t * uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *opts,
    songfilterpb_t pbflag, dispselsel_t dispselType);
void  uisongselInitializeSongFilter (uisongsel_t *uisongsel,
    songfilter_t *songfilter);
void  uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb);
void  uisongselFree (uisongsel_t *uisongsel);
void  uisongselMainLoop (uisongsel_t *uisongsel);
int   uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
void  uisongselDanceSelect (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);
void  uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx);
/* song filter */
void  uisongselApplySongFilter (uisongsel_t *uisongsel);
void  uisongselInitFilterDisplay (uisongsel_t *uisongsel);
char  * uisongselRatingGet (void *udata, int idx);
char  * uisongselLevelGet (void *udata, int idx);
char  * uisongselStatusGet (void *udata, int idx);
char  * uisongselFavoriteGet (void *udata, int idx);
void  uisongselSortBySelect (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselCreateSortByList (uisongsel_t *uisongsel);
void  uisongselGenreSelect (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselCreateGenreList (uisongsel_t *uisongsel);
void  uisongselFilterDanceProcess (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselSetSelectionCallback (uisongsel_t *uisongsel, UICallback *uicbpath, UICallback *uicbdbidx);

/* uisongselfilter.c */
bool uisongselFilterDialog (void *udata);
void uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

/* uisongselgtk.c */
void  uisongselUIInit (uisongsel_t *uisongsel);
void  uisongselUIFree (uisongsel_t *uisongsel);
UIWidget  * uisongselBuildUI (uisongsel_t *uisongsel, UIWidget *parentwin);
bool  uisongselQueueProcessPlayCallback (void *udata);
void  uisongselClearData (uisongsel_t *uisongsel);
void  uisongselPopulateData (uisongsel_t *uisongsel);
void  uisongselSetFavoriteForeground (uisongsel_t *uisongsel, char *color);
bool  uisongselQueueProcessSelectCallback (void *udata);
void  uisongselSetDefaultSelection (uisongsel_t *uisongsel);
void  uisongselSetSelection (uisongsel_t *uisongsel, const char *pathstr);
bool  uisongselNextSelection (void *udata);
bool  uisongselPreviousSelection (void *udata);
bool  uisongselFirstSelection (void *udata);

#endif /* INC_UISONGSEL_H */

