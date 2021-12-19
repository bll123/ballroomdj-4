#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <assert.h>
#include <unistd.h>

#include "sysvars.h"

sysvar_t    sysvars [SV_MAX];

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
