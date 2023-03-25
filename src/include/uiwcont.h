/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

typedef struct uiwidget uiwcont_t;

/* general routines that are called by the ui specific code */
uiwcont_t *uiwcontAlloc (void);
void  uiwcontFree (uiwcont_t *);

#endif /* INC_UIGENERAL_H */
