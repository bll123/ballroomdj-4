/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <ctype.h>

#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"
#include "tmutil.h"

/* windows may not define these, notably timeradd() */

#ifndef timerclear
# define timerclear(tvp)         (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif

#ifndef timercmp
# define timercmp(tvp, uvp, cmp) \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? \
  ((tvp)->tv_usec cmp (uvp)->tv_usec) : \
  ((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif

#ifndef timeradd
# define timeradd(tvp, uvp, vvp) \
  do { \
    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec; \
    (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec; \
    if ((vvp)->tv_usec >= 1000000) { \
      (vvp)->tv_sec++; \
      (vvp)->tv_usec -= 1000000; \
    } \
  } while (0)
#endif

static char radixchar [2] = { "." };
static bool initialized = false;

static struct tm * tmutilLocaltime (const time_t * const s, struct tm *t);
static void tmutilInit (void);

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
  struct tm         t;

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;

  tp = tmutilLocaltime (&s, &t);

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
  struct timeval    ttm;
  struct timeval    atm;
  struct timeval    res;

  gettimeofday (&ttm, NULL);
  s = addTime / 1000;
  atm.tv_sec = s;
  atm.tv_usec = (addTime - (s * 1000)) * 1000;
  timeradd (&ttm, &atm, &res);
  /* avoid race conditions */
  memcpy (&mt->tm, &res, sizeof (struct timeval));
}

void
mstimesettm (mstime_t *mt, time_t setTime)
{
  time_t            s;
  struct timeval    ttm;
  struct timeval    atm;
  struct timeval    res;

  timerclear (&ttm);
  s = setTime / 1000;
  atm.tv_sec = s;
  atm.tv_usec = (setTime - (s * 1000)) * 1000;
  /* avoid race conditions */
  timeradd (&ttm, &atm, &res);
  memcpy (&mt->tm, &res, sizeof (struct timeval));
}

bool
mstimeCheck (mstime_t *mt)
{
  struct timeval  ttm;
  int             rc;

  gettimeofday (&ttm, NULL);
  rc = timercmp (&ttm, &mt->tm, >);
  if (rc) {
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
  struct tm         t;

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  tp = tmutilLocaltime (&s, &t);
  strftime (buff, max, "%F", tp);
  return buff;
}

char *
tmutilDisp (char *buff, size_t sz, int type)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
  struct tm         t;

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  tp = tmutilLocaltime (&s, &t);
  if (type == TM_CLOCK_ISO) {
    ssize_t    pos;

    strftime (buff, sz, "%A %F %R", tp);
    pos = strlen (buff);
    pos -= 5;
    if (pos > 0 && buff [pos] == '0') {
      buff [pos] = ' ';
    }
  }
  if (type == TM_CLOCK_LOCAL) {
#if _lib_GetDateFormatEx
    wchar_t     wtmp [200];
    char        *tmp;
    char        *p = buff;
    char        *end = buff + sz;

    GetDateFormatEx (LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, NULL,
        NULL, wtmp, 200, NULL);
    tmp = osFromWideChar (wtmp);
    if (tmp != NULL) {
      p = stpecpy (p, end, tmp);
      mdfree (tmp);
    }
# if _lib_GetTimeFormatEx
    GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, NULL,
        NULL, wtmp, 200);
    tmp = osFromWideChar (wtmp);
    if (tmp != NULL) {
      p = stpecpy (p, end, " ");
      p = stpecpy (p, end, tmp);
      mdfree (tmp);
    }
# endif
#else
    strftime (buff, sz, "%c", tp);
#endif
  }
  if (type == TM_CLOCK_TIME_12) {
    strftime (buff, sz, "%r", tp);
    if (*buff == '0') {
      *buff = ' ';
    }
  }
  if (type == TM_CLOCK_TIME_24) {
    strftime (buff, sz, "%R", tp);
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
tmutilTstamp (char *buff, size_t sz)
{
  struct timeval    curr;
  struct tm         *tp;
  char              tbuff [20];
  time_t            s, u, m;
  struct tm         t;


  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
  tp = tmutilLocaltime (&s, &t);
  strftime (buff, sz, "%H:%M:%S", tp);
  snprintf (tbuff, sizeof (tbuff), ".%03zd", m);
  stpecpy (buff + strlen (buff), buff + sz, tbuff);
  return buff;
}

char *
tmutilShortTstamp (char *buff, size_t sz)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
  struct tm         t;

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  tp = tmutilLocaltime (&s, &t);
  strftime (buff, sz, "%H%M%S", tp);
  return buff;
}

char *
tmutilToMS (time_t ms, char *buff, size_t sz)
{
  time_t     h, m, s;

  h = ms / 60 / 60 / 1000;
  m = ((ms / 1000) - (h * 60 * 60)) / 60;
  s = (ms / 1000) - (h * 60 * 60) - (m * 60);
  if (h > 0) {
    snprintf (buff, sz, "%2zd:%02zd:%02zd", h, m, s);
  } else {
    snprintf (buff, sz, "%2zd:%02zd", m, s);
  }
  return buff;
}

char *
tmutilToMSD (time_t ms, char *buff, size_t sz, int decimals)
{
  time_t    m, s, d;

  tmutilInit ();

  m = ms / 1000 / 60;
  s = (ms - (m * 1000 * 60)) / 1000;
  d = (ms - (m * 1000 * 60) - (s * 1000));
  if (decimals > 3) {
    decimals = 3;
  }
  /* reduce the number of digits for display */
  d = d / (int) pow (10, (3 - decimals));
  snprintf (buff, sz, "%" PRIu64 ":%02" PRIu64 "%s%0*" PRIu64,
      (uint64_t) m, (uint64_t) s, radixchar, decimals, (uint64_t) d);
  return buff;
}


char *
tmutilToDateHM (time_t ms, char *buff, size_t sz)
{
  struct tm         *tp;
  time_t            s;
  struct tm         t;

  s = ms / 1000;
  tp = tmutilLocaltime (&s, &t);
  /* I'd like to use the locale aware %X, but it returns the 12-hour clock */
  /* for many locales */
  /* there doesn't appear to be a locale aware 24-hour clock display */
  strftime (buff, sz, "%F %R", tp);

  return buff;
}

/* handles H:M:S.00d, M:S.00d, H:M:S,00d, M:S,00d (3 decimals) */
/* also handles 2 or 1 decimal */
/* 4.8.0: fix: hours were not handled. */
long
tmutilStrToMS (const char *str)
{
  char    *tstr;
  char    *tokstr;
  char    *p;
  char    *np;
  double  dval = 0.0;
  double  tval = 0.0;
  long    value;
  int     len;
  double  mult = 1.0;
  double  multb = 1000.0;
  bool    havedecimal = false;

  tmutilInit ();

  tstr = mdstrdup (str);
  if (strstr (tstr, ".") != NULL || strstr (tstr, ",") != NULL) {
    havedecimal = true;
  }
  p = strtok_r (tstr, ":.,", &tokstr);
  np = strtok_r (NULL, ":.,", &tokstr);
  while (p != NULL) {
    tval = atof (p);
    if (havedecimal && np == NULL) {
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
    p = np;
    np = strtok_r (NULL, ":.,", &tokstr);
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

time_t
tmutilStringToUTC (const char *str, const char *fmt)
{
  struct tm   tm;
  time_t      tmval = 0;

  memset (&tm, 0, sizeof (struct tm));
  tm.tm_hour = 12;
  strptime (str, fmt, &tm);

#if _lib_timegm
  tmval = timegm (&tm);
#else
  {
    struct tm   ttm;
    struct tm   *tp;
    time_t      gtmval = 0;
    time_t      ltmval = 0;
    long        diff = 0;

    tmval = time (NULL);
    tp = tmutilLocaltime (&tmval, &ttm);
    tp->tm_isdst = 0;
    ltmval = mktime (tp);
#if _lib_gmtime_s
    gmtime_s (tp, &tmval);
#else
    tp = gmtime (&tmval);
#endif
    gtmval = mktime (tp);
    diff = ltmval;
    diff -= gtmval;

    tmval = mktime (&tm);
    tmval += diff;
  }
#endif

  return tmval;
}

/* internal routines */

static struct tm *
tmutilLocaltime (const time_t * const s, struct tm *t)
{
  struct tm   *tp = t;

#if _lib_localtime_s
  localtime_s (t, s);
#elif _lib_localtime_r
  localtime_r (s, t);
#else
  tp = localtime (s);
#endif
  return tp;
}

static void
tmutilInit (void)
{
  if (! initialized) {
    struct lconv *lconv;

    lconv = localeconv ();
    stpecpy (radixchar, radixchar + sizeof (radixchar), lconv->decimal_point);
    initialized = true;
  }
}
