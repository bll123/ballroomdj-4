/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUI_H
#define INC_UIUI_H

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  UIUTILS_BASE_MARGIN_SZ = 2,
};

extern int uiBaseMarginSz;
extern int uiTextDirection;

const char * uiBackend (void);
void  uiUIInitialize (int direction);
void  uiUIProcessEvents (void);
void  uiUIProcessWaitEvents (void);     // a small delay
void  uiCleanup (void);
void  uiSetUICSS (const char *uifont, const char *listingfont, const char *accentcolor, const char *errorcolor, const char *markcolor, const char *rowselcolor, const char *rowhlcolor);
void  uiAddColorClass (const char *classnm, const char *color);
void  uiAddBGColorClass (const char *classnm, const char *color);
void  uiAddProgressbarClass (const char *classnm, const char *color);
void  uiInitUILog (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIUI_H */
