#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#if _hdr_bsd_string
# include <bsd/string.h>
#endif

#include "datafile.h"
#include "tmutil.h"
#include "list.h"
#include "sysvars.h"

typedef enum {
  PARSE_SIMPLE,
  PARSE_KEYVALUE
} parsetype_t;

static size_t parse (parseinfo_t *pi, char *data, parsetype_t parsetype);

parseinfo_t *
parseInit (void)
{
  parseinfo_t   *pi;

  pi = malloc (sizeof (parseinfo_t));
  assert (pi != NULL);
  pi->strdata = NULL;
  pi->allocCount = 0;
  pi->count = 0;
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

size_t
parseSimple (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_SIMPLE);
}

size_t
parseKeyValue (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_KEYVALUE);
}

int
saveDataFile (list_t *list, char *fname)
{
/* TODO */
  makeBackups (fname, 2);
  return 0;
}

void
makeBackups (char *fname, int count)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];

  for (int i = count; i >= 1; --i) {
    snprintf (nfn, MAXPATHLEN, "%s.bak.%d", fname, i);
    snprintf (ofn, MAXPATHLEN, "%s.bak.%d", fname, i - 1);
    if (i - 1 == 0) {
      strlcpy (ofn, fname, MAXPATHLEN);
    }
    if (fileExists (ofn)) {
      if ((i - 1) != 0) {
        fileMove (ofn, nfn);
      } else {
        fileCopy (ofn, nfn);
      }
    } else {
    }
  }
  return;
}

int
fileExists (char *fname)
{
  struct stat   statbuf;

  int rc = stat (fname, &statbuf);
  return (rc == 0);
}

int
fileCopy (char *fname, char *nfn)
{
  char      cmd [MAXPATHLEN];

  if (isWindows ()) {
    snprintf (cmd, MAXPATHLEN, "copy /f \"%s\" \"%s\"\n", fname, nfn);
  } else {
    snprintf (cmd, MAXPATHLEN, "cp -f '%s' '%s'\n", fname, nfn);
  }
  int rc = system (cmd);
  return rc;
}

int
fileMove (char *fname, char *nfn)
{
  char      cmd [MAXPATHLEN];

  if (isWindows ()) {
    snprintf (cmd, MAXPATHLEN, "move /f \"%s\" \"%s\"\n", fname, nfn);
  } else {
    snprintf (cmd, MAXPATHLEN, "mv -f '%s' '%s'\n", fname, nfn);
  }
  int rc = system (cmd);
  return rc;
}

/* internal routines */

static size_t
parse (parseinfo_t *pi, char *data, parsetype_t parsetype)
{
  char        *tokptr;
  char        *str;
  size_t      dataCounter;

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
    if (parsetype == PARSE_KEYVALUE && dataCounter % 2 == 1) {
      /* value data is indented */
      str += 2;
    }
    pi->strdata [dataCounter] = str;
    ++dataCounter;
    str = strtok_r (NULL, "\n", &tokptr);
  }
  pi->count = dataCounter;
  return dataCounter;
}
