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
  VAL_REGEX_MIN_SEC,
  VAL_REGEX_HOUR_MIN,
  VAL_REGEX_MAX,
};

typedef struct {
  char    *regex;
} valregex_t;


static valregex_t valregex [VAL_REGEX_MAX] = {
  [VAL_REGEX_NUMERIC] = { "^ *[0-9]+ *$" },
  [VAL_REGEX_FLOAT]   = { "^ *[0-9]+\\.[0-9]+ *$" },
  [VAL_REGEX_MIN_SEC]   = { "^ *[0-9]+:[0-5][0-9] *$" },
  /* americans are likely to type in am/pm */
  [VAL_REGEX_HOUR_MIN]   = { "^ *([0-9]|[1][0-9]|[2][0-4]):[0-5][0-9](([Aa]|[Pp])[Mm])? *$" },
};

/**
 * Validate a string.
 *
 * @param[in] str The string to validate.
 * @param[in] valflags The validation flags (see validate.h).
 * @return On error, returns a constant string with a %s in it to hold the display name.
 * @return On success, returns a null.
 */
const char *
validate (const char *str, int valflags)
{
  char        *valstr = NULL;
  bdjregex_t  *rx = NULL;

  if ((valflags & VAL_NOT_EMPTY) == VAL_NOT_EMPTY) {
    if (str == NULL || ! *str) {
      /* CONTEXT: validation: a value must be set */
      valstr = _("%s must be set.");
    }
  }
  if ((valflags & VAL_NO_SPACES) == VAL_NO_SPACES) {
    if (str != NULL && strstr (str, " ") != NULL) {
      /* CONTEXT: validation: spaces are not allowed  */
      valstr = _("%s: Spaces are not allowed");
    }
  }
  if ((valflags & VAL_NO_SLASHES) == VAL_NO_SLASHES) {
    if (str != NULL &&
      (strstr (str, "/") != NULL ||
      strstr (str, "\\") != NULL)) {
      /* CONTEXT: validation: slashes ( / and \\ ) are not allowed  */
      valstr = _("%s: Slashes are not allowed");
    }
  }
  if ((valflags & VAL_NUMERIC) == VAL_NUMERIC) {
    rx = regexInit (valregex [VAL_REGEX_NUMERIC].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: spaces are not allowed  */
      valstr = _("%s: Spaces are not allowed.");
    }
    regexFree (rx);
  }
  if ((valflags & VAL_FLOAT) == VAL_FLOAT) {
    rx = regexInit (valregex [VAL_REGEX_FLOAT].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: must be a numeric value */
      valstr = _("%s must be numeric.");
    }
    regexFree (rx);
  }
  if ((valflags & VAL_HOUR_MIN) == VAL_HOUR_MIN) {
    rx = regexInit (valregex [VAL_REGEX_HOUR_MIN].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (hours/minutes) */
      valstr = _("%s: Invalid time.");
    }
    regexFree (rx);
  }
  if ((valflags & VAL_MIN_SEC) == VAL_MIN_SEC) {
    rx = regexInit (valregex [VAL_REGEX_MIN_SEC].regex);
    if (str != NULL && ! regexMatch (rx, str)) {
      /* CONTEXT: validation: invalid time (minutes/seconds) */
      valstr = _("%s: Invalid duration.");
    }
    regexFree (rx);
  }

  return valstr;
}

