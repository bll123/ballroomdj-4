/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#if ! __has_include (<winsock2.h>)
# include <sys/types.h>
#endif
#include <unistd.h>

#if __has_include (<winsock2.h>)
# include <winsock2.h>
#endif

#include "bdjstring.h"
#include "osenv.h"
#include "osnetutils.h"

/* this function will fail if the computer name has non-ascii characters */
char *
getHostname (char *buff, size_t sz)
{
  *buff = '\0';
  gethostname (buff, sz);

  if (*buff == '\0') {
    /* gethostname() does not appear to work on windows */
    osGetEnv ("COMPUTERNAME", buff, sz);
    stringAsciiToLower (buff);
  }

  return buff;
}
