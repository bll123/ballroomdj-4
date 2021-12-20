#ifndef INC_TMUTIL_H
#define INC_TMUTIL_H

#include <sys/time.h>

typedef struct {
  struct timeval    start;
} mtime_t;

void      msleep (size_t);
void      mtimestart (mtime_t *);
size_t    mtimeend (mtime_t *);

#endif /* INC_TMUTIL_H */
