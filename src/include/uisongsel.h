#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
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
  conn_t            *conn;
  ssize_t           idxStart;
  ssize_t           oldIdxStart;
  ilistidx_t        danceIdx;
  songfilter_t      *songfilter;
  rating_t          *ratings;
  level_t           *levels;
  status_t          *status;
  nlist_t           *options;
  datafile_t        *filterDisplayDf;
  nlist_t           *filterDisplaySel;
  sortopt_t         *sortopt;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  dispselsel_t      dispselType;
  double            dfilterCount;
  /* filter data */
  songfilterpb_t    dfltpbflag;
  uiutilsdropdown_t sortbysel;
  uiutilsdropdown_t filterdancesel;
  uiutilsdropdown_t filtergenresel;
  uiutilsentry_t    searchentry;
  uiutilsspinbox_t  filterratingsel;
  uiutilsspinbox_t  filterlevelsel;
  uiutilsspinbox_t  filterstatussel;
  uiutilsspinbox_t  filterfavoritesel;
  /* song selection tab */
  uiutilsdropdown_t dancesel;
  /* widget data */
  void              *uiWidgetData;
} uisongsel_t;

/* uisongsel.c */
uisongsel_t * uisongselInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel, nlist_t *opts,
    songfilterpb_t pbflag, dispselsel_t dispselType);
void  uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb);
void  uisongselFree (uisongsel_t *uisongsel);
void  uisongselMainLoop (uisongsel_t *uisongsel);
int   uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
void  uisongselFilterDanceProcess (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselDanceSelect (uisongsel_t *uisongsel, ssize_t idx);
void  uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);
void  uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx);
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


/* uisongselgtk.c */
void      uisongselUIInit (uisongsel_t *uisongsel);
void      uisongselUIFree (uisongsel_t *uisongsel);
GtkWidget * uisongselBuildUI (uisongsel_t *uisongsel, GtkWidget *parentwin);
void      uisongselClearData (uisongsel_t *uisongsel);
void      uisongselPopulateData (uisongsel_t *uisongsel);
void      uisongselSetFavoriteForeground (uisongsel_t *uisongsel, char *color);
void      uisongselQueueProcessSelectHandler (UIWidget *uiwidget, void *udata);

#endif /* INC_UISONGSEL_H */

