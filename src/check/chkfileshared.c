/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include "fileshared.h"

#define FN  "tmp/fileshared.txt"
#define DATAB "def456\n"

int
main (int argc, char *argv [])
{
  fileshared_t  *sfh;

  sfh = fileSharedOpen (FN, FILE_OPEN_APPEND);
  sleep (1);
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  sleep (1);
  fileSharedClose (sfh);
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
