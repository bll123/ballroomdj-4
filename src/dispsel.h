#ifndef INC_DISPSEL_H
#define INC_DISPSEL_H

#include "datafile.h"
#include "slist.h"

typedef enum {
  DISP_SEL_MM,
  DISP_SEL_MUSICQ,
  DISP_SEL_REQUEST,
  DISP_SEL_SONGEDIT_A,
  DISP_SEL_SONGEDIT_B,
  DISP_SEL_SONGLIST,
//  DISP_SEL_SONGSEL,
  DISP_SEL_MAX,
} dispselsel_t;

typedef struct {
  char          *name [DISP_SEL_MAX];
  datafile_t    *df [DISP_SEL_MAX];
  slist_t       *dispsel [DISP_SEL_MAX];
} dispsel_t;

dispsel_t * dispselAlloc (void);
void      dispselFree (dispsel_t *dispsel);
slist_t   * dispselGetList (dispsel_t *dispsel, dispselsel_t idx);
void      dispselSave (dispsel_t *dispsel, dispselsel_t idx, slist_t *list);

#endif /* INC_DISPSEL_H */
