/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "mdebug.h"
#include "tmutil.h"

static char radixchar [2] = { "." };
static bool initialized = false;

void
mssleep (time_t mt)
{
/* windows seems to have a very large amount of overhead when calling */
/* nanosleep() or Sleep(). */
/* macos seems to have a minor amount of overhead when calling nanosleep() */

/* on windows, nanosleep is within the libwinpthread msys2 library which */
/* is not wanted for bdj4se &etc. So use the Windows API Sleep() function */
#if _lib_Sleep
  Sleep ((DWORD) mt);
#endif
#if ! _lib_Sleep && _lib_nanosleep
//  int               rc;
  struct timespec   ts;
  struct timespec   rem;

  ts.tv_sec = mt / 1000;
  ts.tv_nsec = (mt - (ts.tv_sec * 1000)) * 1000 * 1000;
  while (ts.tv_sec > 0 || ts.tv_nsec > 0) {
    /* rc = */ nanosleep (&ts, &rem);
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    /* remainder is only valid when EINTR is returned */
    /* most of the time, an interrupt is caused by a control-c while testing */
    /* just let an interrupt stop the sleep */
//    if (rc < 0 && errno == EINTR) {
//      ts.tv_sec = rem.tv_sec;
//      ts.tv_nsec = rem.tv_nsec;
//    }
  }
#endif
}

time_t
mstime (void)
{
  struct timeval    curr;
  time_t            s, u, m, tot;

  gettimeofday (&curr, NULL);

  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
  tot = s * 1000 + m;
  return tot;
}

time_t
mstimestartofday (void)
{
  struct timeval    curr;
  time_t            s, m, h, tot;
  struct tm         *tp;
#if _lib_localtime_r
  struct tm         t;
#endif

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;

#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif

  m = tp->tm_min * 60;
  h = tp->tm_hour * 3600;
  tot = (s - h - m) * 1000;
  return tot;
}

void
mstimestart (mstime_t *mt)
{
  gettimeofday (&mt->tm, NULL);
}

time_t
mstimeend (mstime_t *t)
{
  struct timeval    end;
  time_t            s, u, m;

  gettimeofday (&end, NULL);
  s = end.tv_sec - t->tm.tv_sec;
  u = end.tv_usec - t->tm.tv_usec;
  m = s * 1000 + u / 1000;
  return m;
}

void
mstimeset (mstime_t *mt, time_t addTime)
{
  time_t            s;

  gettimeofday (&mt->tm, NULL);
  s = addTime / 1000;
  mt->tm.tv_sec += s;
  mt->tm.tv_usec += (addTime - (s * 1000)) * 1000;
}

void
mstimesettm (mstime_t *mt, time_t setTime)
{
  time_t            s;

  mt->tm.tv_sec = 0;
  mt->tm.tv_usec = 0;
  s = setTime / 1000;
  mt->tm.tv_sec += s;
  mt->tm.tv_usec += (setTime - (s * 1000)) * 1000;
}

bool
mstimeCheck (mstime_t *mt)
{
  struct timeval    ttm;

  gettimeofday (&ttm, NULL);
  if (ttm.tv_sec > mt->tm.tv_sec) {
    return true;
  }
  if (ttm.tv_sec == mt->tm.tv_sec &&
      ttm.tv_usec >= mt->tm.tv_usec) {
    return true;
  }
  return false;
}

char *
tmutilDstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
#if _lib_localtime_r
  struct tm         t;
#endif

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%Y-%m-%d", tp);
  return buff;
}

char *
tmutilDisp (char *buff, size_t max, int type)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
#if _lib_localtime_r
  struct tm         t;
#endif

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  if (type == TM_CLOCK_ISO) {
    ssize_t    pos;

    strftime (buff, max, "%A %Y-%m-%d %H:%M", tp);
    pos = strlen (buff);
    pos -= 5;
    if (pos > 0 && buff [pos] == '0') {
      buff [pos] = ' ';
    }
  }
  if (type == TM_CLOCK_LOCAL) {
    strftime (buff, max, "%c", tp);
  }
  if (type == TM_CLOCK_TIME_12) {
    strftime (buff, max, "%I:%M %p", tp);
    if (*buff == '0') {
      *buff = ' ';
    }
  }
  if (type == TM_CLOCK_TIME_24) {
    strftime (buff, max, "%H:%M", tp);
    if (*buff == '0') {
      *buff = ' ';
    }
  }
  if (type == TM_CLOCK_OFF) {
    *buff = '\0';
  }
  return buff;
}

char *
tmutilTstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  char              tbuff [20];
  time_t            s, u, m;
#if _lib_localtime_r
  struct tm         t;
#endif


  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%H:%M:%S", tp);
  snprintf (tbuff, sizeof (tbuff), ".%03zd", m);
  strlcat (buff, tbuff, max);
  return buff;
}

char *
tmutilShortTstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
#if _lib_localtime_r
  struct tm         t;
#endif

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%H%M%S", tp);
  return buff;
}

char *
tmutilToMS (time_t ms, char *buff, size_t max)
{
  time_t     h, m, s;

  h = ms / 60 / 60 / 1000;
  m = ((ms / 1000) - (h * 60 * 60)) / 60;
  s = (ms / 1000) - (h * 60 * 60) - (m * 60);
  if (h > 0) {
    snprintf (buff, max, "%2zd:%02zd:%02zd", h, m, s);
  } else {
    snprintf (buff, max, "%2zd:%02zd", m, s);
  }
  return buff;
}

char *
tmutilToMSD (time_t ms, char *buff, size_t max, int decimals)
{
  time_t    m, s, d;

  if (! initialized) {
    struct lconv *lconv;

    lconv = localeconv ();
    strlcpy (radixchar, lconv->decimal_point, sizeof (radixchar));
    initialized = true;
  }

  m = ms / 1000 / 60;
  s = (ms - (m * 1000 * 60)) / 1000;
  d = (ms - (m * 1000 * 60) - (s * 1000));
  if (decimals > 3) {
    decimals = 3;
  }
  /* reduce the number of digits for display */
  d = d / (int) pow (10, (3 - decimals));
  snprintf (buff, max, "%"PRIu64":%02"PRIu64"%s%0*"PRIu64,
      (uint64_t) m, (uint64_t) s, radixchar, decimals, (uint64_t) d);
  return buff;
}


char *
tmutilToDateHM (time_t ms, char *buff, size_t max)
{
  struct tm         *tp;
  time_t            s;
#if _lib_localtime_r
  struct tm         t;
#endif

  s = ms / 1000;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%Y-%m-%d %H:%M", tp);
  return buff;
}

/* handles H:M:S.00d, M:S.00d, H:M:S,00d, M:S,00d (3 decimals) */
/* also handles 2 or 1 decimal */
long
tmutilStrToMS (const char *str)
{
  char    *tstr;
  char    *tokstr;
  char    *p;
  double  dval = 0.0;
  double  tval = 0.0;
  long    value;
  int     count;
  int     len;
  double  mult = 1.0;
  double  multb = 1000.0;

  tstr = mdstrdup (str);
  p = strtok_r (tstr, ":.,", &tokstr);
  count = 0;
  while (p != NULL) {
    tval = atof (p);
    if (count == 2) {
      len = strlen (p);
      if (len < 3) {
        /* have to handle conversions of .1, .01, .001 */
        tval *= pow (10, 3 - len);
      }
      multb = 1.0;
    } else {
      dval *= mult;
    }
    dval += tval * multb;
    mult = 60.0;
    p = strtok_r (NULL, ":.,", &tokstr);
    ++count;
  }
  value = (long) dval;

  mdfree (tstr);
  return value;
}

long
tmutilStrToHM (const char *str)
{
  char    *tstr;
  char    *tokstr;
  char    *p;
  long    value;
  long    hour;
  bool    isam = false;
  bool    ispm = false;

  tstr = mdstrdup (str);
  p = strtok_r (tstr, ":.", &tokstr);
  value = atoi (p);
  hour = value;
  value *= 60;
  p = strtok_r (NULL, ":.", &tokstr);
  if (p != NULL) {
    /* americans might say: 12am with no colon */
    value += atoi (p);
  } else {
    p = tstr;
  }

  /* handle americanisms */
  while (*p) {
    if (*p == 'a' || *p == 'A') {
      isam = true;
      break;
    }
    if (*p == 'p' || *p == 'P') {
      ispm = true;
      break;
    }
    ++p;
  }

  /* 12am must be changed to 24:00, as 0:00 is the not-set state */
  if (isam && value == 720) {
    value *= 2;
  }
  if (ispm && hour != 12) {
    /* add 12 hours */
    value += 720;
  }

  mdfree (tstr);
  value *= 1000;
  return value;
}

