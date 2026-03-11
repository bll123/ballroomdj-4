/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

#include "bdj4intl.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "validate.h"

enum {
  VAL_REGEX_NUMERIC,
  VAL_REGEX_FLOAT,
  VAL_REGEX_MS,
  VAL_REGEX_HMS,
  VAL_REGEX_HM,
  VAL_REGEX_HMS_PRECISE,
  VAL_REGEX_WINCHARS,
  VAL_REGEX_BASE_URI,
  VAL_REGEX_FULL_URI,
  VAL_REGEX_MAX,
};

typedef struct {
  char    *regex;
} valregex_t;

/* 06時54分19秒 japanese and chinese */
/* 30 36 e6 99 82 35 33 e5  88 86 30 31 e7 a7 92 */

static valregex_t valregex [VAL_REGEX_MAX] = {
  [VAL_REGEX_NUMERIC] = { "^ *[0-9]+ *$" },
  [VAL_REGEX_FLOAT]   = { "^ *[0-9]*[,.]?[0-9]+ *$" },
  /* in the time fields, allow the dot character for other locales */
  [VAL_REGEX_MS]   = { "^ *[0-9]+[:.][0-5][0-9] *$" },
  [VAL_REGEX_HMS]   = { "^ *(([0-9]+:)?[0-5])?[0-9][:.][0-5][0-9] *$" },
  /* americans are likely to type in am/pm */
  /* hour-min is use for a clock value */
  [VAL_REGEX_HM]   = { "^ *(0?[0-9]|[1][0-9]|[2][0-4])([:.][0-5][0-9])?( *([Aa]|[Pp])(\\.?[Mm]\\.?)?)? *$" },
  [VAL_REGEX_HMS_PRECISE] = { "^ *(([0-9]+:)?[0-5])?[0-9][:.][0-5][0-9]([.,][0-9]*)? *$" },
  [VAL_REGEX_WINCHARS]   = { "[*'\":|<>^]" },
  [VAL_REGEX_BASE_URI]   = { "^([\\w-]+\\.)+[\\w-]+$" },
  [VAL_REGEX_FULL_URI]   = { "^[a-zA-Z][a-zA-Z0-9]*://([\\w-]+\\.)+[\\w-]+(:[0-9]+)?[\\w ;,./?%&=-]*$" },
};

static_assert (sizeof (valregex) / sizeof (valregex_t) == VAL_REGEX_MAX,
    "missing val-regex entry");

typedef struct {
  volatile atomic_flag  initialized;
  bdjregex_t            *rx [VAL_REGEX_MAX];
} validateinit_t;

static validateinit_t gvalinit = { ATOMIC_FLAG_INIT,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } };

static void validateInit (void);
static void validateCleanup (void);

/**
 * Validate a string.
 *
 * @param[in] buff Output buffer for response.
 * @param[in] sz Size of output buffer.
 * @param[in] str The string to validate.
 * @param[in] valflags The validation flags (see validate.h).
 * @return On error, returns a constant string with a %s in it to hold the display name.
 * @return On success, returns true
 */
bool
validate (char *buff, size_t sz, const char *label, const char *str, int valflags)
{
  bool        rc = true;

  validateInit ();

  *buff = '\0';

  if (rc && (valflags & VAL_NOT_EMPTY) == VAL_NOT_EMPTY) {
    if (str == NULL || ! *str) {
      /* CONTEXT: validation: a value must be set */
      snprintf (buff, sz, _("%s: Must be set."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_NO_SPACES) == VAL_NO_SPACES) {
    if (str != NULL && strstr (str, " ") != NULL) {
      /* CONTEXT: validation: spaces are not allowed  */
      snprintf (buff, sz, _("%s: Spaces are not allowed."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_NO_SLASHES) == VAL_NO_SLASHES) {
    if (str != NULL &&
      (strstr (str, "/") != NULL ||
      strstr (str, "\\") != NULL)) {
      /* CONTEXT: validation: slashes ( / and \\ ) are not allowed  */
      snprintf (buff, sz, _("%s: Slashes are not allowed."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_NO_WINCHARS) == VAL_NO_WINCHARS) {
    if (str != NULL && regexMatch (gvalinit.rx [VAL_REGEX_WINCHARS], str)) {
      /* CONTEXT: validation: characters not allowed*/
      snprintf (buff, sz, _("%s: *'\":|<>^ characters are not allowed."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_NUMERIC) == VAL_NUMERIC) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_NUMERIC], str)) {
      /* CONTEXT: validation: must be numeric */
      snprintf (buff, sz, _("%s: Must be numeric."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_FLOAT) == VAL_FLOAT) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_FLOAT], str)) {
      /* CONTEXT: validation: must be a numeric value */
      snprintf (buff, sz, _("%s: Must be numeric."), label);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_HOUR_MIN) == VAL_HOUR_MIN) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_HM], str)) {
      /* CONTEXT: validation: invalid time (hours/minutes) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_MIN_SEC) == VAL_MIN_SEC) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_MS], str)) {
      /* CONTEXT: validation: invalid time (minutes/seconds) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_HMS) == VAL_HMS) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_HMS], str)) {
      /* CONTEXT: validation: invalid time (hours/minutes/seconds) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_HMS_PRECISE) == VAL_HMS_PRECISE) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_HMS_PRECISE], str)) {
      /* CONTEXT: validation: invalid time (hour/min/sec.sec) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_BASE_URI) == VAL_BASE_URI) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_BASE_URI], str)) {
      /* CONTEXT: validation: invalid URL */
      snprintf (buff, sz, _("%s: Invalid URL (%s)."), label, str);
      rc = false;
    }
  }
  if (rc && (valflags & VAL_FULL_URI) == VAL_FULL_URI) {
    if (str != NULL && ! regexMatch (gvalinit.rx [VAL_REGEX_FULL_URI], str)) {
      /* CONTEXT: validation: invalid URL */
      snprintf (buff, sz, _("%s: Invalid URL (%s)."), label, str);
      rc = false;
    }
  }

  return rc;
}

static void
validateInit (void)
{
  if (atomic_flag_test_and_set (&gvalinit.initialized)) {
    return;
  }

  for (int i = 0; i < VAL_REGEX_MAX; ++i) {
    gvalinit.rx [i] = regexInit (valregex [i].regex);
  }

  atexit (validateCleanup);
}

static void
validateCleanup (void)
{
  if (! atomic_flag_test_and_set (&gvalinit.initialized)) {
    atomic_flag_clear (&gvalinit.initialized);
    return;
  }

  for (int i = 0; i < VAL_REGEX_MAX; ++i) {
    regexFree (gvalinit.rx [i]);
  }

  atomic_flag_clear (&gvalinit.initialized);
}
