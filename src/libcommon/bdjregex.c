/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <glib.h>
#include <glib/gregex.h>

#include "bdjregex.h"
#include "mdebug.h"

typedef struct bdjregex {
  GRegex  *regex;
} bdjregex_t;

bdjregex_t *
regexInit (const char *pattern)
{
  bdjregex_t  *rx;
  GRegex      *regex;

  if (pattern == NULL) {
    return NULL;
  }

  regex = g_regex_new (pattern, G_REGEX_OPTIMIZE, 0, NULL);
  mdextalloc (regex);
  if (regex == NULL) {
    fprintf (stderr, "ERR: failed to compile %s\n", pattern);
    return NULL;
  }
  rx = mdmalloc (sizeof (bdjregex_t));
  rx->regex = regex;
  return rx;
}

void
regexFree (bdjregex_t *rx)
{
  if (rx != NULL) {
    if (rx->regex != NULL) {
      mdextfree (rx->regex);
      g_regex_unref (rx->regex);
    }
    mdfree (rx);
  }
}

char *
regexEscape (const char *str)
{
  char    *p;

  if (str == NULL) {
    return NULL;
  }
  p = g_regex_escape_string (str, -1);
  mdextalloc (p);
  return p;
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
  mdextalloc (val);
  return val;
}

void
regexGetFree (char **val)
{
  mdextfree (val);
  g_strfreev (val);
}

char *
regexReplace (bdjregex_t *rx, const char *str, const char *repl)
{
  char  *nstr = NULL;
  /* G_REGEX_MATCH_DEFAULT is not defined */
  nstr = g_regex_replace (rx->regex, str, -1, 0, repl, 0, NULL);
  mdextalloc (nstr);
  return nstr;
}

/* this routine re-creates the regex every time */
/* if it is re-used, consider creating a normal regex */
char *
regexReplaceLiteral (const char *str, const char *tgt, const char *repl)
{
  bdjregex_t  *rx;
  char        *nstr = NULL;
  char        *tmptgt;
  size_t      len;

  len = strlen (tgt) + 5;
  tmptgt = mdmalloc (len);
  snprintf (tmptgt, len, "\\Q%s\\E", tgt);
  rx = regexInit (tmptgt);
  /* G_REGEX_MATCH_DEFAULT is not defined */
  nstr = g_regex_replace_literal (rx->regex, str, -1, 0, repl, 0, NULL);
  mdextalloc (nstr);
  mdfree (tmptgt);
  regexFree (rx);
  return nstr;
}

