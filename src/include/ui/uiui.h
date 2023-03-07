/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUI_H
#define INC_UIUI_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

enum {
  UIUTILS_BASE_MARGIN_SZ = 2,
};

extern int uiBaseMarginSz;

void  uiUIInitialize (void);
void  uiUIProcessEvents (void);
void  uiUIProcessWaitEvents (void);     // a small delay
void  uiCleanup (void);
void  uiSetUICSS (const char *uifont, const char *accentcolor, const char *errorcolor);
void  uiAddColorClass (const char *classnm, const char *color);
void  uiAddBGColorClass (const char *classnm, const char *color);
void  uiAddProgressbarClass (const char *classnm, const char *color);
void  uiInitUILog (void);

#endif /* INC_UIUI_H */
