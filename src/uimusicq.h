#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "musicq.h"
#include "progstart.h"

typedef struct {
  /* music queue tab */
  GtkWidget       *box;
  GtkWidget       *playlistSelectButton;
  GtkWidget       *playlistSelectWin;
  GtkWidget       *playlistSelect;
  bool            playlistSelectOpen;
  GtkWidget       *danceSelectButton;
  GtkWidget       *danceSelectWin;
  GtkWidget       *danceSelect;
  bool            danceSelectOpen;
  /* tree views */
  GtkWidget       *musicqTree;
} uimusicqui_t;

typedef struct {
  int             musicqManageIdx;
  progstart_t     *progstart;
  conn_t          *conn;
  GtkWidget       *parentwin;
  GdkPixbuf       *pauseImg;
  uimusicqui_t    ui [MUSICQ_MAX];
  /* temporary stuff used for music queue update processing */
  nlist_t         *uniqueList;
  nlist_t         *dispList;
  nlist_t         *workList;
} uimusicq_t;

uimusicq_t  * uimusicqInit (progstart_t *progstart, conn_t *conn);
void        uimusicqFree (uimusicq_t *uimusicq);
GtkWidget   * uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci);
int         uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);
void        uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);

#endif /* INC_UIMUSICQ_H */

