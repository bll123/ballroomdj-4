/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUI_H
#define INC_UIUI_H

#include "uiwcont.h"

enum {
  UIUTILS_BASE_MARGIN_SZ = 2,
};

extern int uiBaseMarginSz;

const char * uiBackend (void);
void  uiUIInitialize (int direction);
void  uiUIProcessEvents (void);
void  uiUIProcessWaitEvents (void);     // a small delay
void  uiCleanup (void);
void  uiSetUICSS (const char *uifont, const char *accentcolor, const char *errorcolor);
void  uiAddColorClass (const char *classnm, const char *color);
void  uiAddBGColorClass (const char *classnm, const char *color);
void  uiAddProgressbarClass (const char *classnm, const char *color);
void  uiInitUILog (void);

#endif /* INC_UIUI_H */
