/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPBAR_H
#define INC_UIPBAR_H

#include "uiwcont.h"

uiwcont_t *uiCreateProgressBar (void);
void uiProgressBarSet (uiwcont_t *uipb, double val);

#endif /* INC_UIPBAR_H */
