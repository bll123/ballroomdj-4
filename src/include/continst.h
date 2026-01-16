/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */

#include "conn.h"
#include "controller.h"
#include "uiplayer.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct continst continst_t;

continst_t * contInstanceInit (conn_t *conn, uiplayer_t *uiplayer);
void  contInstanceFree (continst_t *ci);
int   contInstanceSetup (continst_t *ci);
void  contInstanceSetUIPlayer (continst_t *ci, uiplayer_t *uiplayer);
void  contInstanceSetURICallback (continst_t *ci, callback_t *cburi);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
