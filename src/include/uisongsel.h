#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "level.h"
#include "musicdb.h"
#include "nlist.h"
#include "rating.h"
#include "songfilter.h"
#include "sortopt.h"
#include "status.h"
#include "uiutils.h"

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
  GtkWidget         *window;
  /* filter data */
  GtkWidget         *filterDialog;
  GtkWidget         *statusPlayable;
  songfilterpb_t    dfltpbflag;
  uidropdown_t      sortbysel;
  uidropdown_t      filterdancesel;
  uidropdown_t      filtergenresel;
  uientry_t         searchentry;
  uispinbox_t       filterratingsel;
  uispinbox_t       filterlevelsel;
  uispinbox_t       filterstatussel;
  uispinbox_t       filterfavoritesel;
  time_t            filterApplied;
  /* song selection tab */
  uidropdown_t dancesel;
  /* widget data */
  void              *uiWidgetData;
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

/* uisongselfilter.c */
bool uisongselFilterDialog (void *udata);
void uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

/* uisongselgtk.c */
void      uisongselUIInit (uisongsel_t *uisongsel);
void      uisongselUIFree (uisongsel_t *uisongsel);
UIWidget  * uisongselBuildUI (uisongsel_t *uisongsel, GtkWidget *parentwin);
void      uisongselClearData (uisongsel_t *uisongsel);
void      uisongselPopulateData (uisongsel_t *uisongsel);
void      uisongselSetFavoriteForeground (uisongsel_t *uisongsel, char *color);
bool      uisongselQueueProcessSelectCallback (void *udata);

#endif /* INC_UISONGSEL_H */

