/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* general routines that are called by the ui specific code */
uiwcont_t *uiwcontAlloc (void);
void  uiwcontFree (uiwcont_t *);
/* these routines will be removed at a later date */
void uiwcontInit (uiwcont_t *uiwidget);
bool uiwcontIsSet (uiwcont_t *uiwidget);
void uiwcontCopy (uiwcont_t *target, uiwcont_t *source);

#endif /* INC_UIGENERAL_H */
