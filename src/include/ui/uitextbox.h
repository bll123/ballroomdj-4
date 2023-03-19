/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITEXTBOX_H
#define INC_UITEXTBOX_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

typedef struct uitextbox uitextbox_t;

uitextbox_t  *uiTextBoxCreate (int height, const char *hlcolor);
void  uiTextBoxFree (uitextbox_t *tb);
uiwidget_t * uiTextBoxGetScrolledWindow (uitextbox_t *tb);
char  *uiTextBoxGetValue (uitextbox_t *tb);
void  uiTextBoxSetReadonly (uitextbox_t *tb);
void  uiTextBoxScrollToEnd (uitextbox_t *tb);
void  uiTextBoxAppendStr (uitextbox_t *tb, const char *str);
void  uiTextBoxAppendBoldStr (uitextbox_t *tb, const char *str);
void  uiTextBoxAppendHighlightStr (uitextbox_t *tb, const char *str);
void  uiTextBoxSetValue (uitextbox_t *tb, const char *str);
void  uiTextBoxDarken (uitextbox_t *tb);
void  uiTextBoxHorizExpand (uitextbox_t *tb);
void  uiTextBoxVertExpand (uitextbox_t *tb);
void  uiTextBoxSetHeight (uitextbox_t *tb, int h);

#endif /* INC_UITEXTBOX_H */
