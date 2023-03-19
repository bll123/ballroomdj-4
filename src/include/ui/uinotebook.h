/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINOTEBOOK_H
#define INC_UINOTEBOOK_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void  uiCreateNotebook (uiwidget_t *uiwidget);
void  uiNotebookTabPositionLeft (uiwidget_t *uiwidget);
void  uiNotebookAppendPage (uiwidget_t *uinotebook, uiwidget_t *uiwidget, uiwidget_t *uilabel);
void  uiNotebookSetActionWidget (uiwidget_t *uinotebook, uiwidget_t *uiwidget);
void  uiNotebookSetPage (uiwidget_t *uinotebook, int pagenum);
void  uiNotebookSetCallback (uiwidget_t *uinotebook, callback_t *uicb);
void  uiNotebookHideShowPage (uiwidget_t *uinotebook, int pagenum, bool show);

#endif /* INC_UINOTEBOOK_H */
