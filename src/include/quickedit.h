/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_QUICKEDIT_H
#define INC_QUICKEDIT_H

#include "datafile.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  QUICKEDIT_DISP_SPEED,
  QUICKEDIT_DISP_VOLUME,
  QUICKEDIT_DISP_DANCELEVEL,
  QUICKEDIT_DISP_DANCERATING,
  QUICKEDIT_DISP_FAVORITE,
  QUICKEDIT_DISP_MAX,
};

typedef struct quickedit quickedit_t;

quickedit_t * quickeditAlloc (void);
void        quickeditFree (quickedit_t *qe);
nlist_t     * quickeditGetList (quickedit_t *qe);
void        quickeditSave (quickedit_t *qe, nlist_t *dispsel);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_QUICKEDIT_H */
