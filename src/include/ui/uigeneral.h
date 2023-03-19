/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* general routines that are called by the ui specific code */
uiwidget_t *uiwidgetAlloc (void);
void  uiwidgetFree (uiwidget_t *);
/* these routines will be removed at a later date */
void uiwidgetInit (uiwidget_t *uiwidget);
bool uiwidgetIsSet (uiwidget_t *uiwidget);
void uiwidgetCopy (uiwidget_t *target, uiwidget_t *source);

#endif /* INC_UIGENERAL_H */
