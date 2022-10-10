#ifndef INC_UIREQEXT_H
#define INC_UIREQEXT_H

#include "nlist.h"
#include "ui.h"

typedef struct uireqext uireqext_t;

uireqext_t  *uireqextInit (UIWidget *windowp, nlist_t *opts);
void  uireqextFree (uireqext_t *uireqext);
bool  uireqextDialog (void *udata);

#endif /* INC_UIREQEXT_H */
