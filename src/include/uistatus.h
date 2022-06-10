#ifndef INC_UISTATUS_H
#define INC_UISTATUS_H

#include "uiutils.h"

typedef struct uistatus uistatus_t;

uistatus_t * uistatusSpinboxCreate (UIWidget *boxp, bool allflag);
void uistatusFree (uistatus_t *uistatus);
int uistatusGetValue (uistatus_t *uistatus);
void uistatusSetValue (uistatus_t *uistatus, int value);
void uistatusDisable (uistatus_t *uistatus);
void uistatusEnable (uistatus_t *uistatus);

#endif /* INC_UISTATUS_H */
