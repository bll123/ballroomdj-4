/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINOTEBOOK_H
#define INC_UINOTEBOOK_H

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateNotebook (void);
void  uiNotebookTabPositionLeft (uiwcont_t *uiwidget);
void  uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget, uiwcont_t *uilabel);
void  uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget);
void  uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum);
void  uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb);
void  uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UINOTEBOOK_H */
