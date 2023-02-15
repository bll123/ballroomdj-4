/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "mdebug.h"
#include "ati.h"

void
atiiInit (const char *atipkg)
{
  return;
}

char *
atiiReadTags (const char *ffn)
{
  return NULL;
}

void
atiiParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite)
{
  return;
}

int
atiiWriteTags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags)
{
  return;
}
