#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "ilist.h"
#include "nlist.h"
#include "samesong.h"
#include "songfilter.h"
#include "uidance.h"
#include "uisongfilter.h"
#include "ui.h"

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
  samesong_t        *samesong;
  dispselsel_t      dispselType;
  double            dfilterCount;
  UIWidget          *windowp;
  UICallback        *queuecb;
  UICallback        *playcb;
  UICallback        *editcb;
  UICallback        *songsavecb;
  dbidx_t           lastdbidx;
  nlist_t           *songlistdbidxlist;
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
  uidance_t         *uidance;
  /* widget data */
  uisongselgtk_t    *uiWidgetData;
  /* song editor */
  UICallback        *newselcb;
} uisongsel_t;

/* uisongsel.c */
uisongsel_t * uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, samesong_t *samesong, nlist_t *opts,
    uisongfilter_t *uisf, dispselsel_t dispselType);
void  uisongselSetPeer (uisongsel_t *uisongsel, uisongsel_t *peer);
void  uisongselInitializeSongFilter (uisongsel_t *uisongsel, songfilter_t *songfilter);
void  uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb);
void  uisongselSetSamesong (uisongsel_t *uisongsel, samesong_t *samesong);
void  uisongselFree (uisongsel_t *uisongsel);
void  uisongselMainLoop (uisongsel_t *uisongsel);
void  uisongselSetSelectionCallback (uisongsel_t *uisongsel, UICallback *uicb);
void  uisongselSetQueueCallback (uisongsel_t *uisongsel, UICallback *uicb);
void  uisongselSetPlayCallback (uisongsel_t *uisongsel, UICallback *uicb);
void  uisongselSetSongSaveCallback (uisongsel_t *uisongsel, UICallback *uicb);
/* song filter */
void  uisongselSetEditCallback (uisongsel_t *uisongsel, UICallback *uicb);
void  uisongselProcessMusicQueueData (uisongsel_t *uisongsel, mp_musicqupdate_t *musicqupdate);

/* uisongselgtk.c */
void  uisongselUIInit (uisongsel_t *uisongsel);
void  uisongselUIFree (uisongsel_t *uisongsel);
UIWidget  * uisongselBuildUI (uisongsel_t *uisongsel, UIWidget *parentwin);
bool  uisongselPlayCallback (void *udata);
void  uisongselClearData (uisongsel_t *uisongsel);
void  uisongselPopulateData (uisongsel_t *uisongsel);
void  uisongselSetFavoriteForeground (uisongsel_t *uisongsel, char *color);
bool  uisongselSelectCallback (void *udata);
void  uisongselSetDefaultSelection (uisongsel_t *uisongsel);
void  uisongselSetSelection (uisongsel_t *uisongsel, long idx);
void  uisongselSetSelectionOffset (uisongsel_t *uisongsel, long idx);
bool  uisongselNextSelection (void *udata);
bool  uisongselPreviousSelection (void *udata);
bool  uisongselFirstSelection (void *udata);
long  uisongselGetSelectLocation (uisongsel_t *uisongsel);
bool  uisongselApplySongFilter (void *udata);
void  uisongselDanceSelectHandler (uisongsel_t *uisongsel, ilistidx_t idx);
bool  uisongselDanceSelectCallback (void *udata, long danceIdx);
void  uisongselSaveSelections (uisongsel_t *uisongsel);
void  uisongselRestoreSelections (uisongsel_t *uisongsel);
void  uisongselSetPlayButtonState (uisongsel_t *uisongsel, int active);
nlist_t *uisongselGetSelectedList (uisongsel_t *uisongsel);
void uisongselClearAllSelections (uisongsel_t *uisongsel);

/* uisongselcommon.c */
void  uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);
void  uisongselPlayProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);
void  uisongselSetPeerFlag (uisongsel_t *uisongsel, bool val);
void  uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx);

#endif /* INC_UISONGSEL_H */

