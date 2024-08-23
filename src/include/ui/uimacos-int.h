/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMACOS_INT_H
#define INC_UIMACOS_INT_H

#include <Cocoa/Cocoa.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct macosmargin {
  NSEdgeInsets  margins;
  NSStackView   *container;
} macosmargin_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIMACOS_INT_H */
