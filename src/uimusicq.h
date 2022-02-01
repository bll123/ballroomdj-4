#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "progstart.h"

typedef struct {
  progstart_t     *progstart;
  conn_t          *conn;
  GtkWidget       *parentwin;
  GdkPixbuf       *pauseImg;
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
} uimusicq_t;

uimusicq_t  * uimusicqInit (progstart_t *progstart, conn_t *conn);
void        uimusicqFree (uimusicq_t *uimusicq);
GtkWidget   * uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin);
int         uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);

#endif /* INC_UIMUSICQ_H */

