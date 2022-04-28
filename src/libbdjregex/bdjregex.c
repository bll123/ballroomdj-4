#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <glib.h>

#include "bdjregex.h"

typedef struct bdjregex {
  GRegex  *regex;
} bdjregex_t;

bdjregex_t *
regexInit (const char *pattern)
{
  bdjregex_t  *rx;
  GRegex      *regex;
  GError      *gerr = NULL;

  if (pattern == NULL) {
    return NULL;
  }

  regex = g_regex_new (pattern, G_REGEX_OPTIMIZE, 0, &gerr);
  if (regex == NULL) {
    fprintf (stderr, "ERR: failed to compile %s\n", pattern);
    return NULL;
  }
  rx = malloc (sizeof (bdjregex_t));
  rx->regex = regex;
  return rx;
}

void
regexFree (bdjregex_t *rx)
{
  if (rx != NULL) {
    if (rx->regex != NULL) {
      g_regex_unref (rx->regex);
    }
    free (rx);
  }
}

char *
regexEscape (const char *str)
{
  if (str == NULL) {
    return NULL;
  }
  return g_regex_escape_string (str, -1);
}

bool
regexMatch (bdjregex_t *rx, const char *str)
{
  if (rx == NULL || str == NULL) {
    return NULL;
  }
  return g_regex_match (rx->regex, str, 0, NULL);
}


char **
regexGet (bdjregex_t *rx, const char *str)
{
  char **val;

  if (rx == NULL || str == NULL) {
    return NULL;
  }

  val = g_regex_split (rx->regex, str, 0);
  return val;
}

