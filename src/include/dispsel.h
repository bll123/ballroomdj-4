/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DISPSEL_H
#define INC_DISPSEL_H

#include "nodiscard.h"
#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  DISP_SEL_NONE           = 0,
  DISP_SEL_LOAD_PLAYER    = (1 << 0),
  DISP_SEL_LOAD_MANAGE    = (1 << 1),
  DISP_SEL_LOAD_MARQUEE   = (1 << 2),
  DISP_SEL_LOAD_ALL       = (1 << 3),
};

typedef enum {
  /* the following are used by the player ui */
  DISP_SEL_HISTORY,
  DISP_SEL_MUSICQ,
  DISP_SEL_REQUEST,
  /* the following are used by the player ui and management ui*/
  DISP_SEL_CURRSONG,
  /* the following are used by the management ui */
  DISP_SEL_SBS_SONGLIST,
  DISP_SEL_SBS_SONGSEL,
  DISP_SEL_MM,
  /* songedit_a through songedit_c must be sequential */
  DISP_SEL_SONGEDIT_A,
  DISP_SEL_SONGEDIT_B,
  DISP_SEL_SONGEDIT_C,
  DISP_SEL_SONGLIST,
  DISP_SEL_SONGSEL,
  DISP_SEL_AUDIOID_LIST,
  DISP_SEL_AUDIOID,
  /* marquee */
  DISP_SEL_MARQUEE,
  DISP_SEL_MAX,
} dispselsel_t;

enum {
  DISP_SEL_SONGEDIT_MAX = 3,
};

typedef struct dispsel dispsel_t;

NODISCARD dispsel_t * dispselAlloc (int loadtype);
void      dispselFree (dispsel_t *dispsel);
slist_t   * dispselGetList (dispsel_t *dispsel, dispselsel_t idx);
void      dispselSave (dispsel_t *dispsel, dispselsel_t idx, slist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_DISPSEL_H */
