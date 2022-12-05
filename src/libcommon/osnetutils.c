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
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "osnetutils.h"

char *
getHostname (char *buff, size_t sz)
{
  *buff = '\0';
  gethostname (buff, sz);

  if (*buff == '\0') {
    /* gethostname() does not appear to work on windows */
    char *hn = getenv ("COMPUTERNAME");
    if (hn != NULL) {
      strlcpy (buff, hn, sz);
      stringAsciiToLower (buff);
    }
  }

  return buff;
}

