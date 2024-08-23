/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMACOS_INT_H
#define INC_UIMACOS_INT_H

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uispecific {
  void          *packwidget;
  union {
    void          *widget;
  };
  /* to hold the margin information for the widget */
  NSDictionary  *viewBindings;
  NSDictionary  *metrics;
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIMACOS_INT_H */
