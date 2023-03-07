/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UICLIPBOARD_H
#define INC_UICLIPBOARD_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiClipboardSet (const char *txt);

#endif /* INC_UICLIPBOARD_H */
