/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct {
  const char  *uifont;
  const char  *listingfont;
  const char  *accentColor;
  const char  *errorColor;
  const char  *markColor;
  const char  *changedColor;
  const char  *rowselColor;
  const char  *rowhlColor;
  const char  *mqbgColor;
  int         direction;
} uisetup_t;

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
void  uiSetUICSS (uisetup_t *uisetup);
void  uiAddColorClass (const char *classnm, const char *color);
void  uiAddBGColorClass (const char *classnm, const char *color);
void  uiAddProgressbarClass (const char *classnm, const char *color);
void  uiInitUILog (void);

/* ui interface specific */
void  uiwcontUIInit (uiwcont_t *uiwidget);
void  uiwcontUIWidgetInit (uiwcont_t *uiwidget);
void  uiwcontUIFree (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

