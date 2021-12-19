#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "parse.h"

parseinfo_t *
parseInit (void)
{
  parseinfo_t   *pi;

  pi = malloc (sizeof (parseinfo_t));
  pi->strdata = NULL;
  pi->allocCount = 0;
  return pi;
}

void
parseFree (parseinfo_t *pi)
{
  if (pi != NULL) {
    if (pi->strdata != NULL) {
      free (pi->strdata);
    }
    free (pi);
  }
}

char **
parseGetData (parseinfo_t *pi)
{
  return pi->strdata;
}

int
parse (parseinfo_t *pi, char *data)
{
  char        *tokptr;
  char        *str;
  int         dataCounter;

  tokptr = NULL;
  if (pi->allocCount < 60) {
    pi->allocCount = 60;
    pi->strdata = realloc (pi->strdata, sizeof (char *) * pi->allocCount);
    assert (pi->strdata != NULL);
  }

  str = strtok_r (data, "\n", &tokptr);
  dataCounter = 0;
  while (str != NULL) {
    if (*str == '#') {
      str = strtok_r (NULL, "\n", &tokptr);
      continue;
    }

    if ((size_t) dataCounter >= pi->allocCount) {
      pi->allocCount += 10;
      pi->strdata = realloc (pi->strdata, sizeof (char *) * pi->allocCount);
      assert (pi->strdata != NULL);
    }
    if (dataCounter % 2 == 1) {
      /* value data is indented */
      str += 2;
    }
    pi->strdata [dataCounter] = str;
    ++dataCounter;
    str = strtok_r (NULL, "\n", &tokptr);
  }
  return dataCounter;
}
