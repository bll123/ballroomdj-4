/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBUTTON_H
#define INC_UIBUTTON_H

#include "callback.h"
#include "uiwcont.h"

typedef struct uibutton uibutton_t;

uibutton_t *uiCreateButton (callback_t *uicb, char *title, char *imagenm);
void uiButtonFree (uibutton_t *uibutton);
uiwcont_t *uiButtonGetWidgetContainer (uibutton_t *uibutton);
void uiButtonSetImagePosRight (uibutton_t *uibutton);
void uiButtonSetImage (uibutton_t *uibutton, const char *imagenm, const char *tooltip);
void uiButtonSetImageIcon (uibutton_t *uibutton, const char *nm);
void uiButtonAlignLeft (uibutton_t *uibutton);
void uiButtonSetText (uibutton_t *uibutton, const char *txt);
void uiButtonSetReliefNone (uibutton_t *uibutton);
void uiButtonSetFlat (uibutton_t *uibutton);
void uiButtonSetRepeat (uibutton_t *uibutton, int repeatms);
void uiButtonCheckRepeat (uibutton_t *uibutton);
void uiButtonSetState (uibutton_t *uibutton, int state);

#endif /* INC_UIBUTTON_H */
