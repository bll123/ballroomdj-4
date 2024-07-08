/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITEXTBOX_H
#define INC_UITEXTBOX_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "uiwcont.h"

uiwcont_t *uiTextBoxCreate (int height, const char *hlcolor);
void  uiTextBoxFree (uiwcont_t *uiwidget);
char  *uiTextBoxGetValue (uiwcont_t *uiwidget);
void  uiTextBoxSetReadonly (uiwcont_t *uiwidget);
void  uiTextBoxScrollToEnd (uiwcont_t *uiwidget);
void  uiTextBoxAppendStr (uiwcont_t *uiwidget, const char *str);
void  uiTextBoxAppendBoldStr (uiwcont_t *uiwidget, const char *str);
void  uiTextBoxAppendHighlightStr (uiwcont_t *uiwidget, const char *str);
void  uiTextBoxSetDarkBG (uiwcont_t *uiwidget);
void  uiTextBoxHorizExpand (uiwcont_t *uiwidget);
void  uiTextBoxVertExpand (uiwcont_t *uiwidget);
void  uiTextBoxSetHeight (uiwcont_t *uiwidget, int h);
void  uiTextBoxSetParagraph (uiwcont_t *uiwidget, int indent, int interpara);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UITEXTBOX_H */
