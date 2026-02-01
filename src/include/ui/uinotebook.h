/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateNotebook (void);
void  uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget, uiwcont_t *uilabel);
void  uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum);
void  uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

