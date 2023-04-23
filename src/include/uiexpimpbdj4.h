/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIEXPIMPBDJ4_H
#define INC_UIEXPIMPBDJ4_H

#include "nlist.h"
#include "ui.h"

enum {
  UIEXPIMPBDJ4_EXPORT,
  UIEXPIMPBDJ4_IMPORT,
  UIEXPIMPBDJ4_MAX,
};

typedef struct uiexpimpbdj4 uiexpimpbdj4_t;

uiexpimpbdj4_t  *uiexpimpbdj4Init (uiwcont_t *windowp, nlist_t *opts);
void    uiexpimpbdj4Free (uiexpimpbdj4_t *uiexpimpbdj4);
void    uiexpimpbdj4SetResponseCallback (uiexpimpbdj4_t *uiexpimpbdj4, callback_t *uicb);
bool    uiexpimpbdj4Dialog (uiexpimpbdj4_t *uiexpimpbdj4, int expimptype);

#endif /* INC_UIEXPIMPBDJ4_H */
