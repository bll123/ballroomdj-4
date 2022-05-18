#ifndef INC_UILEVEL_H
#define INC_UILEVEL_H

#include "uiutils.h"

typedef struct uilevel uilevel_t;

uilevel_t * uilevelSpinboxCreate (UIWidget *boxp, bool allflag);
void uilevelFree (uilevel_t *uilevel);
int uilevelGetValue (uilevel_t *uilevel);

#endif /* INC_UILEVEL_H */
