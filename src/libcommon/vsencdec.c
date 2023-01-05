/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include "bdjstring.h"
#include "mdebug.h"
#include "vsencdec.h"
#include "osrandom.h"

enum {
  VSEC_ENCRYPT,
  VSEC_DECRYPT,
};

enum {
  VSEC_SALT_SIZE = 4,
};

void
vsencdec (const char *str, char *buff, size_t sz)
{
  int         encdecflag;
  int         salt [VSEC_SALT_SIZE];
  int         count = 0;
  int         pc;
  int         nc;
  size_t      len;
  size_t      tlen;
  int         saltidx;
  const char  *p = NULL;
  char        *data = NULL;

  len = strlen (str);
  if (strncmp (str, VSEC_E_PFX, strlen (VSEC_E_PFX)) == 0) {
    encdecflag = VSEC_DECRYPT;
  } else {
    encdecflag = VSEC_ENCRYPT;
  }

  if (encdecflag == VSEC_ENCRYPT) {
    for (int i = 0; i < VSEC_SALT_SIZE; ++i) {
      salt [i] = (char) (dRandom () * 128.0);
    }
    p = str;
  }
  if (encdecflag == VSEC_DECRYPT) {
    data = (char *) g_base64_decode (
        (const gchar *) str + strlen (VSEC_E_PFX), &len);
    p = data;
    len -= VSEC_SALT_SIZE;
    for (int i = 0; i < VSEC_SALT_SIZE; ++i) {
      salt [i] = data [len + i];
    }
    buff [count++] = VSEC_DECRYPT;
  }

  count = 0;
  saltidx = 0;
  pc = '.';
  tlen = len;
  while (tlen--) {
    nc = pc ^ (int) *p ^ salt [saltidx];
    if (encdecflag == VSEC_ENCRYPT) {
      pc = *p;
    } else {
      pc = nc;
    }
    buff [count++] = nc;
    ++saltidx;
    if (saltidx >= VSEC_SALT_SIZE) { saltidx = 0; }
    ++p;
  }

  if (encdecflag == VSEC_ENCRYPT) {
    for (int i = 0; i < VSEC_SALT_SIZE; ++i) {
      buff [count++] = salt [i];
    }
    data = g_base64_encode ((const guchar *) buff, len + VSEC_SALT_SIZE);
    strlcpy (buff, VSEC_E_PFX, sz);
    strlcat (buff, data, sz);
    mdfree (data);
  }
  if (encdecflag == VSEC_DECRYPT) {
    buff [count++] = '\0';
    mdfree (data);
  }
}

