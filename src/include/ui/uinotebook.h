/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINOTEBOOK_H
#define INC_UINOTEBOOK_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void  uiCreateNotebook (UIWidget *uiwidget);
void  uiNotebookTabPositionLeft (UIWidget *uiwidget);
void  uiNotebookAppendPage (UIWidget *uinotebook, UIWidget *uiwidget, UIWidget *uilabel);
void  uiNotebookSetActionWidget (UIWidget *uinotebook, UIWidget *uiwidget);
void  uiNotebookSetPage (UIWidget *uinotebook, int pagenum);
void  uiNotebookSetCallback (UIWidget *uinotebook, callback_t *uicb);
void  uiNotebookHideShowPage (UIWidget *uinotebook, int pagenum, bool show);

#endif /* INC_UINOTEBOOK_H */
