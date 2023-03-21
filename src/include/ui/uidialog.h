/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDIALOG_H
#define INC_UIDIALOG_H

#include "uiwcont.h"

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
void  uiCreateDialog (uiwcont_t *uiwidget, uiwcont_t *window, callback_t *uicb, const char *title, ...);
void  uiDialogShow (uiwcont_t *uiwidgetp);
void  uiDialogAddButtons (uiwcont_t *uidialog, ...);
void  uiDialogPackInDialog (uiwcont_t *uidialog, uiwcont_t *boxp);
void  uiDialogDestroy (uiwcont_t *uidialog);
uiselect_t *uiDialogCreateSelect (uiwcont_t *window, const char *label, const char *startpath, const char *dfltname, const char *mimefiltername, const char *mimetype);

#endif /* INC_UIDIALOG_H */
