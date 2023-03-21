/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

typedef union uiwidget uiwcont_t;

/* general routines that are called by the ui specific code */
uiwcont_t *uiwcontAlloc (void);
void  uiwcontFree (uiwcont_t *);

/* the follwing routines will be removed at a later date */
void uiwcontInit (uiwcont_t *uiwidget);
bool uiwcontIsSet (uiwcont_t *uiwidget);
void uiwcontCopy (uiwcont_t *target, uiwcont_t *source);

#endif /* INC_UIGENERAL_H */
