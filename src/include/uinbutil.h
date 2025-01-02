/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINBUTIL_H
#define INC_UINBUTIL_H

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  UI_TAB_MUSICQ,
  UI_TAB_HISTORY,
  UI_TAB_SONGSEL,
  UI_TAB_SONGEDIT,
  UI_TAB_AUDIOID,
};

typedef struct {
  int     tabcount;
  int     *tabids;
} uinbtabid_t;

/* notebook ui helper routines */
uinbtabid_t * uinbutilIDInit (void);
void uinbutilIDFree (uinbtabid_t *nbtabid);
void uinbutilIDAdd (uinbtabid_t *nbtabid, int id);
int  uinbutilIDGet (uinbtabid_t *nbtabid, int idx);
int  uinbutilIDGetPage (uinbtabid_t *nbtabid, int id);
void uinbutilIDStartIterator (uinbtabid_t *nbtabid, int *iteridx);
int  uinbutilIDIterate (uinbtabid_t *nbtabid, int *iteridx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UINBUTIL_H */
