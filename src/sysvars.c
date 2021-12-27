#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#if _hdr_unistd
# include <unistd.h>
#endif
#if _sys_utsname
# include <sys/utsname.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "sysvars.h"
#include "bdjstring.h"
#include "portability.h"

char        sysvars [SV_MAX][MAXPATHLEN];
long        lsysvars [SVL_MAX];

void
sysvarsInit (void)
{
  char                tbuf [MAXPATHLEN+1];
#if _lib_uname
  int                 rc;
  struct utsname      ubuf;

  rc = uname (&ubuf);
  assert (rc == 0);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, MAXPATHLEN);
  /* ### fix osdisp this later */
  strlcpy (sysvars [SV_OSDISP], ubuf.sysname, MAXPATHLEN);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, MAXPATHLEN);
#endif
#if _lib_GetVersionEx
  OSVERSIONINFOA osvi;

  memset (&osvi, 0, sizeof (OSVERSIONINFOA));
  osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
  GetVersionEx (&osvi);

  strlcpy (sysvars [SV_OSNAME], "Windows", MAXPATHLEN);
  strlcpy (sysvars [SV_OSDISP], "Windows ", MAXPATHLEN);
  snprintf (sysvars [SV_OSVERS], MAXPATHLEN, "%ld.%ld",
      osvi.dwMajorVersion, osvi.dwMinorVersion);
  if (strcmp (sysvars [SV_OSVERS], "5.0") == 0) {
    strlcat (sysvars [SV_OSDISP], "2000", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "5.1") == 0) {
    strlcat (sysvars [SV_OSDISP], "XP", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "5.2") == 0) {
    strlcat (sysvars [SV_OSDISP], "XP Pro", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.0") == 0) {
    strlcat (sysvars [SV_OSDISP], "Vista", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.1") == 0) {
    strlcat (sysvars [SV_OSDISP], "7", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.2") == 0) {
    strlcat (sysvars [SV_OSDISP], "8.0", MAXPATHLEN);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.3") == 0) {
    strlcat (sysvars [SV_OSDISP], "8.1", MAXPATHLEN);
  }
  else {
    strlcat (sysvars [SV_OSDISP], sysvars [SV_OSVERS], MAXPATHLEN);
  }
#endif
  gethostname (tbuf, MAXPATHLEN);
  strlcpy (sysvars [SV_HOSTNAME], tbuf, MAXPATHLEN);

  lsysvars [SVL_BDJIDX] = 0;
}

void
sysvarSetLong (sysvarlkey_t idx, long value)
{
  lsysvars [idx] = value;
}

/* boolean */
inline int
isMacOS (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Darwin") == 0);
}

/* boolean */
inline int
isWindows (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Windows") == 0);
}

/* boolean */
inline int
isLinux (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Linux") == 0);
}

