/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
};

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
  bdjregex_t  *rx = NULL;
  bool        rc = true;

  *buff = '\0';

  if ((valflags & VAL_NOT_EMPTY) == VAL_NOT_EMPTY) {
    if (str == NULL || ! *str) {
      /* CONTEXT: validation: a value must be set */
      snprintf (buff, sz, _("%s: Must be set."), label);
      rc = false;
    }
  }
  if ((valflags & VAL_NO_SPACES) == VAL_NO_SPACES) {
    if (str != NULL && strstr (str, " ") != NULL) {
      /* CONTEXT: validation: spaces are not allowed  */
      snprintf (buff, sz, _("%s: Spaces are not allowed."), label);
      rc = false;
    }
  }
  if ((valflags & VAL_NO_SLASHES) == VAL_NO_SLASHES) {
    if (str != NULL &&
      (strstr (str, "/") != NULL ||
      strstr (str, "\\") != NULL)) {
      /* CONTEXT: validation: slashes ( / and \\ ) are not allowed  */
      snprintf (buff, sz, _("%s: Slashes are not allowed."), label);
      rc = false;
    }
  }
  if ((valflags & VAL_NO_WINCHARS) == VAL_NO_WINCHARS) {
    rx = regexInit (valregex [VAL_REGEX_WINCHARS].regex);
    if (str != NULL && regexMatch (rx, str)) {
      /* CONTEXT: validation: characters not allowed*/
      snprintf (buff, sz, _("%s: *'\":|<>^ characters are not allowed."), label);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_NUMERIC) == VAL_NUMERIC) {
    rx = regexInit (valregex [VAL_REGEX_NUMERIC].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: must be numeric */
      snprintf (buff, sz, _("%s: Must be numeric."), label);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_FLOAT) == VAL_FLOAT) {
    rx = regexInit (valregex [VAL_REGEX_FLOAT].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: must be a numeric value */
      snprintf (buff, sz, _("%s: Must be numeric."), label);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_HOUR_MIN) == VAL_HOUR_MIN) {
    rx = regexInit (valregex [VAL_REGEX_HM].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (hours/minutes) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_MIN_SEC) == VAL_MIN_SEC) {
    rx = regexInit (valregex [VAL_REGEX_MS].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (minutes/seconds) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_HMS) == VAL_HMS) {
    rx = regexInit (valregex [VAL_REGEX_HMS].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (hours/minutes/seconds) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
    regexFree (rx);
  }
  if ((valflags & VAL_HMS_PRECISE) == VAL_HMS_PRECISE) {
    rx = regexInit (valregex [VAL_REGEX_HMS_PRECISE].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (hour/min/sec.sec) */
      snprintf (buff, sz, _("%s: Invalid time (%s)."), label, str);
      rc = false;
    }
    regexFree (rx);
  }

  return rc;
}

