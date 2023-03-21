/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINOTEBOOK_H
#define INC_UINOTEBOOK_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

uiwcont_t *uiCreateNotebook (void);
void  uiNotebookTabPositionLeft (uiwcont_t *uiwidget);
void  uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget, uiwcont_t *uilabel);
void  uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget);
void  uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum);
void  uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb);
void  uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show);

#endif /* INC_UINOTEBOOK_H */
