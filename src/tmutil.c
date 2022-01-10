#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "tmutil.h"
#include "bdjstring.h"

void
mssleep (size_t mt)
{
  size_t t = mt * 1000;
  usleep ((useconds_t) t);
}

time_t
mstime (void)
{
  struct timeval    curr;

  gettimeofday (&curr, NULL);

  time_t s = curr.tv_sec;
  time_t u = curr.tv_usec;
  time_t m = u / 1000;
  time_t tot = s * 1000 + m;
  return tot;
}

void
mstimestart (mstime_t *mt)
{
  gettimeofday (&mt->start, NULL);
}

size_t
mstimeend (mstime_t *t)
{
  struct timeval    end;

  gettimeofday (&end, NULL);
  long s = end.tv_sec - t->start.tv_sec;
  long u = end.tv_usec - t->start.tv_usec;
  long m = s * 1000 + u / 1000;
  return (size_t) m;
}

char *
tmutilDstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;

  gettimeofday (&curr, NULL);
  time_t s = curr.tv_sec;
#if 1 || _lib_localtime_r
  struct tm         t;
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%Y-%m-%d", tp);
  return buff;
}

char *
tmutilTstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  char              tbuff [20];

  gettimeofday (&curr, NULL);
  time_t s = curr.tv_sec;
  long u = curr.tv_usec;
  long m = u / 1000;
#if 1 || _lib_localtime_r
  struct tm         t;
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%H:%M:%S", tp);
  snprintf (tbuff, sizeof (tbuff), ".%03ld", m);
  strlcat (buff, tbuff, max);
  return buff;
}
