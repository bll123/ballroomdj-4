/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWCONT_H
#define INC_UIWCONT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uiwcont uiwcont_t;

/* general routines that are called by the ui specific code */

/* uiwcont.c */

uiwcont_t *uiwcontAlloc (int basetype, int type);
/* basefree only frees the uiwidget, not any internals */
void uiwcontBaseFree (uiwcont_t *uiwidget);
void uiwcontSetWidget (uiwcont_t *uiwidget, void *widget, void *packwidget);
const char * uiwcontDesc (int wtype);

/* uiwcontfree.c */

/* uiwcontFree() will also call any widget-specific free procedures */
void  uiwcontFree (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIWCONT_H */
