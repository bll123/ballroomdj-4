#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#if _hdr_bsd_string
# include <bsd/string.h>
#endif

#include "sysvars.h"

char sysvars [SV_MAX][MAXPATHLEN];

void
sysvarsInit (void)
{
  int                 rc;
  struct utsname      ubuf;
  char                tbuf [MAXPATHLEN+1];

  rc = uname (&ubuf);
  assert (rc == 0);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, MAXPATHLEN);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, MAXPATHLEN);
  gethostname (tbuf, MAXPATHLEN);
  strlcpy (sysvars [SV_HOSTNAME], tbuf, MAXPATHLEN);
}

int
isMacOS (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Darwin"));
}

int
isWindows (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Windows"));
}

int
isLinux (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Linux"));
}
