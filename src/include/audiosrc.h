#ifndef INC_AUDIOSRC_H
#define INC_AUDIOSRC_H

#include "slist.h"

#define LIBAUDIOSRC_PFX  "libaudiosrc"

typedef struct audiosrc audiosrc_t;

audiosrc_t *audiosrcInit (const char *pkg);
void audiosrcFree (audiosrc_t *audiosrc);
bool audiosrcExists (audiosrc_t *audiosrc, const char *nm);
bool audiosrcRemove (audiosrc_t *audiosrc, const char *nm);
slist_t * audiosrcInterfaceList (void);

const char *audiosrciDesc (void);
bool audiosrciExists (const char *nm);
bool audiosrciRemove (const char *nm);

#endif /* INC_AUDIOSRC_H */
