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
#include "uisongfilter.h"
#include "uiutils.h"

typedef struct uisongsel uisongsel_t;
typedef struct uisongselgtk uisongselgtk_t;

enum {
  UISONGSEL_PEER_MAX = 2,
};

typedef struct uisongsel {
  const char        *tag;
  conn_t            *conn;
  dbidx_t           idxStart;
  ilistidx_t        danceIdx;
  nlist_t           *options;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  dispselsel_t      dispselType;
  double            dfilterCount;
  UIWidget          *windowp;
  UICallback        *queuecb;
  UICallback        *editcb;
  dbidx_t           lastdbidx;
  /* peers */
  int               peercount;
  uisongsel_t       *peers [UISONGSEL_PEER_MAX];
  bool              ispeercall;
  /* filter data */
  uisongfilter_t    *uisongfilter;
  UICallback        sfapplycb;
  UICallback        sfdanceselcb;
  songfilter_t      *songfilter;
  /* song selection tab */
  uidropdown_t      *dancesel;
  /* widget data */
  uisongselgtk_t    *uiWidgetData;
  /* song editor */
  UICallback        *newselcb;
} uisongsel_t;

/* uisongsel.c */
uisongsel_t * uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *opts,
    uisongfilter_t *uisf, dispselsel_t dispselType);
void  uisongselSetPeer (uisongsel_t *uisongsel, uisongsel_t *peer);
void  uisongselInitializeSongFilter (uisongsel_t *uisongsel, songfilter_t *songfilter);
void  uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb);
void  uisongselFree (uisongsel_t *uisongsel);
void  uisongselMainLoop (uisongsel_t *uisongsel);
int   uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
void  uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);
void  uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx);
void  uisongselSetSelectionCallback (uisongsel_t *uisongsel, UICallback *uicbdbidx);
void  uisongselSetPeerFlag (uisongsel_t *uisongsel, bool val);
void  uisongselSetQueueCallback (uisongsel_t *uisongsel, UICallback *uicbdbidx);
/* song filter */
void  uisongselDanceSelectHandler (uisongsel_t *uisongsel, ssize_t idx);
bool  uisongselApplySongFilter (void *uisongsel);
void  uisongselSetEditCallback (uisongsel_t *uisongsel, UICallback *uicb);


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
void  uisongselSetSelection (uisongsel_t *uisongsel, long idx);
bool  uisongselNextSelection (void *udata);
bool  uisongselPreviousSelection (void *udata);
bool  uisongselFirstSelection (void *udata);
long  uisongselGetSelectLocation (uisongsel_t *uisongsel);

#endif /* INC_UISONGSEL_H */

