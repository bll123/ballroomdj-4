#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "progstate.h"

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  /* song selection tab */
  GtkWidget       *box;
  GtkWidget       *queueButton;
  GtkWidget       *songselTree;
} uisongselect_t;

uisongselect_t  * uisongselInit (progstate_t *progstate, conn_t *conn);
void            uisongselFree (uisongselect_t *uisongselect);
GtkWidget       * uisongselActivate (uisongselect_t *uisongselect);

#endif /* INC_UISONGSEL_H */

