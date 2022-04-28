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

  regex = g_regex_new (pattern, G_REGEX_OPTIMIZE, 0, &gerr);
  if (regex == NULL) {
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
  return g_regex_escape_string (str, -1);
}

bool
regexMatch (bdjregex_t *rx, const char *str)
{
  return g_regex_match (rx->regex, str, 0, NULL);
}


char **
regexGet (bdjregex_t *rx, const char *str)
{
  char **val;
  val = g_regex_split (rx->regex, str, 0);
  return val;
}

