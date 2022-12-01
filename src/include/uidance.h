#ifndef INC_UIDANCE_H
#define INC_UIDANCE_H

#include "ui.h"

enum {
  UIDANCE_PACK_START,
  UIDANCE_PACK_END,
  UIDANCE_NONE,
  UIDANCE_ALL_DANCES,
  UIDANCE_EMPTY_DANCE,
};

typedef struct uidance uidance_t;

uidance_t * uidanceDropDownCreate (UIWidget *boxp, UIWidget *parentwin, int flags, const char *label, int where, int count);
UIWidget * uidanceGetButton (uidance_t *uidance);
void uidanceFree (uidance_t *uidance);
int uidanceGetValue (uidance_t *uidance);
void uidanceSetValue (uidance_t *uidance, int value);
void uidanceDisable (uidance_t *uidance);
void uidanceEnable (uidance_t *uidance);
void uidanceSizeGroupAdd (uidance_t *uidance, UIWidget *sg);
void uidanceSetCallback (uidance_t *uidance, UICallback *cb);

#endif /* INC_UIDANCE_H */
