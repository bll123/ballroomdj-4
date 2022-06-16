#ifndef INC_UINBUTIL_H
#define INC_UINBUTIL_H

#include <stdbool.h>

typedef struct {
  int     tabcount;
  int     *tabids;
} uiutilsnbtabid_t;

/* notebook ui helper routines */
uiutilsnbtabid_t * uiutilsNotebookIDInit (void);
void uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid);
void uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id);
int  uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx);
int  uiutilsNotebookIDGetPage (uiutilsnbtabid_t *nbtabid, int id);
void uiutilsNotebookIDStartIterator (uiutilsnbtabid_t *nbtabid, int *iteridx);
int  uiutilsNotebookIDIterate (uiutilsnbtabid_t *nbtabid, int *iteridx);

#endif /* INC_UINBUTIL_H */
