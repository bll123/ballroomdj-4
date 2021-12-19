#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

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
  strlcpy (sysvars [SV_OSNAME].value, ubuf.sysname, MAXPATHLEN);
  strlcpy (sysvars [SV_OSVERS].value, ubuf.version, MAXPATHLEN);
  gethostname (tbuf, MAXPATHLEN);
  strlcpy (sysvars [SV_HOSTNAME].value, tbuf, MAXPATHLEN);
}
