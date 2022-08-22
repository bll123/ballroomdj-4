#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "datafile.h"
#include "songutil.h"

char *
songFullFileName (char *sfname)
{
  char      *tname;
  size_t    len;

  if (sfname == NULL) {
    return NULL;
  }

  tname = malloc (MAXPATHLEN);
  assert (tname != NULL);

  len = strlen (sfname);
  if ((len > 0 && sfname [0] == '/') ||
      (len > 2 && sfname [1] == ':' && sfname [2] == '/')) {
    strlcpy (tname, sfname, MAXPATHLEN);
  } else {
    snprintf (tname, MAXPATHLEN, "%s/%s",
        bdjoptGetStr (OPT_M_DIR_MUSIC), sfname);
  }
  return tname;
}

void
songConvAdjustFlags (datafileconv_t *conv)
{

  if (conv->valuetype == VALUE_STR) {
    int   num;
    char  *str;

    str = conv->str;

    num = SONG_ADJUST_NONE;
    while (*str) {
      if (*str == SONG_ADJUST_STR_NORM) {
        num |= SONG_ADJUST_NORM;
      }
      if (*str == SONG_ADJUST_STR_TRIM) {
        num |= SONG_ADJUST_TRIM;
      }
      if (*str == SONG_ADJUST_STR_SPEED) {
        num |= SONG_ADJUST_SPEED;
      }
      ++str;
    }

    conv->valuetype = VALUE_NUM;
    if (conv->allocated) {
      free (conv->str);
    }
    conv->num = num;
    conv->allocated = false;
  } else if (conv->valuetype == VALUE_NUM) {
    int   num;
    char  tbuff [40];
    char  *str;

    conv->valuetype = VALUE_STR;
    num = conv->num;

    str = tbuff;
    if ((num & SONG_ADJUST_SPEED) == SONG_ADJUST_SPEED) {
      *str++ = SONG_ADJUST_STR_SPEED;
    }
    if ((num & SONG_ADJUST_TRIM) == SONG_ADJUST_TRIM) {
      *str++ = SONG_ADJUST_STR_TRIM;
    }
    if ((num & SONG_ADJUST_NORM) == SONG_ADJUST_NORM) {
      *str++ = SONG_ADJUST_STR_NORM;
    }
    *str = '\0';

    conv->str = strdup (tbuff);
    conv->allocated = true;
  }
}

