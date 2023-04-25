#ifndef INC_EXPIMPBDJ4_H
#define INC_EXPIMPBDJ4_H

#include "musicdb.h"

enum {
  EIBDJ4_EXPORT,
  EIBDJ4_IMPORT,
};

typedef struct eibdj4 eibdj4_t;

eibdj4_t *eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag);
void eibdj4Free (eibdj4_t *eibdj4);
void eibdj4GetCount (eibdj4_t *eibdj4, int *count, int *tot);
bool eibdj4Process (eibdj4_t *eibdj4);

#endif /* INC_EXPIMPBDJ4_H */
