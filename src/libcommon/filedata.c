/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "filedata.h"
#include "fileop.h"
#include "mdebug.h"

char *
filedataReadAll (const char *fname, size_t *rlen)
{
  FILE        *fh;
  ssize_t     len;
  char        *data = NULL;

  len = fileopSize (fname);
  if (len <= 0) {
    if (rlen != NULL) {
      *rlen = 0;
    }
    return NULL;
  }
  fh = fileopOpen (fname, "r");
  if (fh != NULL) {
    data = mdmalloc (len + 1);
    assert (data != NULL);
    len = fread (data, 1, len, fh);
    data [len] = '\0';
    fclose (fh);
    if (rlen != NULL) {
      *rlen = len;
    }
  }

  return data;
}

char *
filedataReplace (char *data, size_t *dlen, const char *target, const char *repl)
{
  char    *p;
  char    *lastp;
  size_t  nlenalloc;
  size_t  nlen;
  size_t  nlenpos;
  size_t  tlen;
  size_t  targetlen;
  size_t  repllen;
  char    *ndata;

  if (data == NULL) {
    return NULL;
  }

  targetlen = strlen (target);
  repllen = strlen (repl);
  nlen = *dlen;
  nlenalloc = nlen;
  ndata = mdmalloc (nlenalloc + 1);
  assert (ndata != NULL);
  lastp = data;

  nlenpos = 0;
  p = strstr (data, target);
  while (p != NULL) {
    tlen = nlenalloc + (repllen - targetlen);
    if (tlen > nlenalloc) {
      ndata = mdrealloc (ndata, tlen + 1);
      assert (ndata != NULL);
      nlenalloc = tlen;
    }
    nlen += (repllen - targetlen);
    memcpy (ndata + nlenpos, lastp, p - lastp);
    nlenpos += p - lastp;
    memcpy (ndata + nlenpos, repl, repllen);
    nlenpos += repllen;

    lastp = p + targetlen;
    ++p;
    p = strstr (p, target);
  }
  memcpy (ndata + nlenpos, lastp, *dlen - (lastp - data));
  ndata [nlen] = '\0';
  *dlen = nlen;

  return ndata;
}
