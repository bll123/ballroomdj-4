#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#if _sys_utsname
# include <sys/utsname.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "sysvars.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "portability.h"
#include "pathutil.h"

char        sysvars [SV_MAX][MAXPATHLEN];
ssize_t     lsysvars [SVL_MAX];

static char *cacertFiles [] = {
  "/etc/ssl/certs/ca-certificates.crt",
  "/opt/local/etc/openssl/cert.pem",
  "/usr/local/etc/openssl/cert.pem",
  "http/curl-ca-bundle.crt",
};
#define CACERT_FILE_COUNT (sizeof (cacertFiles) / sizeof (char *))

void
sysvarsInit (const char *argv0)
{
  char          tbuf [MAXPATHLEN+1];
  char          tcwd [MAXPATHLEN+1];
  char          buff [MAXPATHLEN+1];
  int           rc;
  char          *p;

#if _lib_uname
  struct utsname      ubuf;

  rc = uname (&ubuf);
  assert (rc == 0);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, MAXPATHLEN);
  /* ### fix osdisp this later */
  strlcpy (sysvars [SV_OSDISP], ubuf.sysname, MAXPATHLEN);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, MAXPATHLEN);
  strlcpy (sysvars [SV_OSBUILD], "", MAXPATHLEN);
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
  snprintf (sysvars [SV_OSBUILD], MAXPATHLEN, "%ld",
      osvi.dwBuildNumber);
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
  strlcat (sysvars [SV_OSDISP], " ", MAXPATHLEN);
  strlcat (sysvars [SV_OSDISP], sysvars [SV_OSBUILD], MAXPATHLEN);
#endif
  gethostname (tbuf, MAXPATHLEN);
  strlcpy (sysvars [SV_HOSTNAME], tbuf, MAXPATHLEN);
  if (isWindows ()) {
    /* gethostname() does not appear to work on windows */
    char *hn = strdup (getenv ("COMPUTERNAME"));
    assert (hn != NULL);
    strtolower (hn);
    strlcpy (sysvars [SV_HOSTNAME], hn, MAXPATHLEN);
    free (hn);
  }

  (void) ! getcwd (tcwd, MAXPATHLEN);

  strlcpy (tbuf, argv0, MAXPATHLEN);
  strlcpy (buff, argv0, MAXPATHLEN);
  pathNormPath (buff, MAXPATHLEN);
  if (*buff != '/' &&
      (strlen (buff) > 2 && *(buff + 1) == ':' && *(buff + 2) != '/')) {
    strlcpy (tbuf, tcwd, MAXPATHLEN);
    strlcat (tbuf, buff, MAXPATHLEN);
  }
  /* this gives us the real path to the executable */
  pathRealPath (tbuf, buff);
  pathNormPath (buff, MAXPATHLEN);

  /* strip off the filename */
  p = strrchr (buff, '/');
  *p = '\0';
  strlcpy (sysvars [SV_BDJ4EXECDIR], buff, MAXPATHLEN);

  /* strip off '/bin' */
  p = strrchr (buff, '/');
  *p = '\0';
  strlcpy (sysvars [SV_BDJ4MAINDIR], buff, MAXPATHLEN);

  if (fileopExists ("data")) {
    /* if there is a data directory in the current working directory  */
    /* a change of directories is contra-indicated.                   */

    pathNormPath (tcwd, MAXPATHLEN);
    strlcpy (sysvars [SV_BDJ4DIR], tcwd, MAXPATHLEN);

    strlcpy (sysvars [SV_BDJ4DATADIR], tcwd, MAXPATHLEN);
    strlcat (sysvars [SV_BDJ4DATADIR], "/data", MAXPATHLEN);
  } else {
    strlcpy (sysvars [SV_BDJ4DIR], sysvars [SV_BDJ4MAINDIR], MAXPATHLEN);
    strlcpy (sysvars [SV_BDJ4DATADIR], sysvars [SV_BDJ4MAINDIR], MAXPATHLEN);
    strlcat (sysvars [SV_BDJ4DATADIR], "/data", MAXPATHLEN);
  }

  strlcpy (sysvars [SV_BDJ4HTTPDIR], sysvars [SV_BDJ4MAINDIR], MAXPATHLEN);
  strlcat (sysvars [SV_BDJ4HTTPDIR], "/http", MAXPATHLEN);

  strlcpy (sysvars [SV_SHLIB_EXT], SHLIB_EXT, MAXPATHLEN);

  strlcpy (sysvars [SV_MOBMQ_HOST], "ballroomdj.org", MAXPATHLEN);

  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopExists (cacertFiles [i])) {
      strlcpy (sysvars [SV_CA_FILE], cacertFiles [i], MAXPATHLEN);
      break;
    }
  }

  strlcpy (sysvars [SV_BDJ4_VERSION], "unknown", MAXPATHLEN);
  if (fileopExists ("VERSION.txt")) {
    char    *data;
    char    *tokptr;
    char    *tokptrb;
    char    *v;
    char    *b;

    data = filedataReadAll ("VERSION.txt");
    v = strtok_r (data, "\r\n", &tokptr);
    p = strtok_r (v, "=", &tokptrb);
    p = strtok_r (NULL, "=", &tokptrb);
    strlcpy (sysvars [SV_BDJ4_VERSION], p, MAXPATHLEN);
    b = strtok_r (NULL, "\r\n", &tokptr);
    p = strtok_r (b, "=", &tokptrb);
    p = strtok_r (NULL, "=", &tokptrb);
    strlcpy (sysvars [SV_BDJ4_BUILD], p, MAXPATHLEN);
    free (data);
  }

  lsysvars [SVL_BDJIDX] = 0;
  lsysvars [SVL_BASEPORT] = 35548;
}

void
sysvarSetNum (sysvarlkey_t idx, ssize_t value)
{
  lsysvars [idx] = value;
}

inline bool
isMacOS (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Darwin") == 0);
}

inline bool
isWindows (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Windows") == 0);
}

inline bool
isLinux (void)
{
  return (strcmp (sysvars[SV_OSNAME], "Linux") == 0);
}

