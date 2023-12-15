/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

typedef struct uiwcont uiwcont_t;

/* general routines that are called by the ui specific code */

/* uiwcont.c */

uiwcont_t *uiwcontAlloc (void);
/* basefree only frees the uiwidget, not any internals */
void uiwcontBaseFree (uiwcont_t *uiwidget);

/* uiwcontfree.c */

/* uiwcontFree() will also call any widget-specific free procedures */
void  uiwcontFree (uiwcont_t *uiwidget);

#endif /* INC_UIGENERAL_H */
