#include "config.h"

#if __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "osutils.h"

char *
osRegistryGet (char *key, char *name)
{
  return NULL;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  return NULL;
}

#endif
