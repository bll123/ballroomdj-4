/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DISPSEL_H
#define INC_DISPSEL_H

#include "datafile.h"
#include "slist.h"

enum {
  DISP_SEL_LOAD_PLAYER,
  DISP_SEL_LOAD_MANAGE,
  DISP_SEL_LOAD_ALL,
};

typedef enum {
  /* the following are used by the player ui */
  DISP_SEL_HISTORY,
  DISP_SEL_MUSICQ,
  DISP_SEL_REQUEST,
  DISP_SEL_MAX_PLAYER,
  /* the following are used by the management ui */
  DISP_SEL_EZSONGLIST,
  DISP_SEL_EZSONGSEL,
  DISP_SEL_MM,
  /* songedit_a through songedit_c must be sequential */
  DISP_SEL_SONGEDIT_A,
  DISP_SEL_SONGEDIT_B,
  DISP_SEL_SONGEDIT_C,
  DISP_SEL_SONGLIST,
  DISP_SEL_SONGSEL,
  DISP_SEL_AUDIOID_DISP,
  DISP_SEL_AUDIOID,
  DISP_SEL_MAX,
} dispselsel_t;

enum {
  DISP_SEL_SONGEDIT_MAX = 3,
};

typedef struct dispsel dispsel_t;

dispsel_t * dispselAlloc (int loadtype);
void      dispselFree (dispsel_t *dispsel);
slist_t   * dispselGetList (dispsel_t *dispsel, dispselsel_t idx);
void      dispselSave (dispsel_t *dispsel, dispselsel_t idx, slist_t *list);

#endif /* INC_DISPSEL_H */
