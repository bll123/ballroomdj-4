/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDIALOG_H
#define INC_UIDIALOG_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

typedef struct uiselect uiselect_t;

enum {
  RESPONSE_NONE,
  RESPONSE_DELETE_WIN,
  RESPONSE_CLOSE,
  RESPONSE_APPLY,
  RESPONSE_RESET,
};

#include "callback.h"

char  *uiSelectDirDialog (uiselect_t *selectdata);
char  *uiSelectFileDialog (uiselect_t *selectdata);
char  *uiSaveFileDialog (uiselect_t *selectdata);
void  uiCreateDialog (uiwidget_t *uiwidget, uiwidget_t *window, callback_t *uicb, const char *title, ...);
void  uiDialogShow (uiwidget_t *uiwidgetp);
void  uiDialogAddButtons (uiwidget_t *uidialog, ...);
void  uiDialogPackInDialog (uiwidget_t *uidialog, uiwidget_t *boxp);
void  uiDialogDestroy (uiwidget_t *uidialog);
uiselect_t *uiDialogCreateSelect (uiwidget_t *window, const char *label, const char *startpath, const char *dfltname, const char *mimefiltername, const char *mimetype);

#endif /* INC_UIDIALOG_H */
