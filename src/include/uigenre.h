#ifndef INC_UIGENRE_H
#define INC_UIGENRE_H

#include "uiutils.h"

typedef struct uigenre uigenre_t;

uigenre_t * uigenreDropDownCreate (UIWidget *boxp, UIWidget *parentwin, bool allflag);
UIWidget * uigenreGetButton (uigenre_t *uigenre);
void uigenreFree (uigenre_t *uigenre);
int uigenreGetValue (uigenre_t *uigenre);
void uigenreSetValue (uigenre_t *uigenre, int value);
void uigenreDisable (uigenre_t *uigenre);
void uigenreEnable (uigenre_t *uigenre);
void uigenreSizeGroupAdd (uigenre_t *uigenre, UIWidget *sg);

#endif /* INC_UIGENRE_H */
