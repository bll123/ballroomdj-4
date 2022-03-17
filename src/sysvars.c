#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>

#if _sys_resource
# include <sys/resource.h>
#endif
#if _sys_utsname
# include <sys/utsname.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "sysvars.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "osnetutils.h"
#include "pathutil.h"

#define SV_MAX_SZ   512

static char        sysvars [SV_MAX][SV_MAX_SZ];
static ssize_t     lsysvars [SVL_MAX];

static char *cacertFiles [] = {
  "/etc/ssl/certs/ca-certificates.crt",
  "/opt/local/etc/openssl/cert.pem",
  "/usr/local/etc/openssl/cert.pem",
  "http/curl-ca-bundle.crt",
};
#define CACERT_FILE_COUNT (sizeof (cacertFiles) / sizeof (char *))

static void enable_core_dump (void);

void
sysvarsInit (const char *argv0)
{
  char          tbuf [SV_MAX_SZ+1];
  char          tcwd [SV_MAX_SZ+1];
  char          buff [SV_MAX_SZ+1];
  char          *tptr;
  char          *tsep;
  char          *tokstr;
  char          *p;
#if _lib_uname
  int             rc;
  struct utsname  ubuf;
#endif
#if _lib_GetVersionEx
  OSVERSIONINFOA osvi;
#endif


#if _lib_uname
  rc = uname (&ubuf);
  assert (rc == 0);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, SV_MAX_SZ);
  /* ### FIX : fix osdisp later */
  strlcpy (sysvars [SV_OSDISP], ubuf.sysname, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSBUILD], "", SV_MAX_SZ);
#endif
#if _lib_GetVersionEx
  memset (&osvi, 0, sizeof (OSVERSIONINFOA));
  osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
  GetVersionEx (&osvi);

  strlcpy (sysvars [SV_OSNAME], "windows", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSDISP], "Windows ", SV_MAX_SZ);
  snprintf (sysvars [SV_OSVERS], SV_MAX_SZ, "%ld.%ld",
      osvi.dwMajorVersion, osvi.dwMinorVersion);
  snprintf (sysvars [SV_OSBUILD], SV_MAX_SZ, "%ld",
      osvi.dwBuildNumber);
  if (strcmp (sysvars [SV_OSVERS], "5.0") == 0) {
    strlcat (sysvars [SV_OSDISP], "2000", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "5.1") == 0) {
    strlcat (sysvars [SV_OSDISP], "XP", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "5.2") == 0) {
    strlcat (sysvars [SV_OSDISP], "XP Pro", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.0") == 0) {
    strlcat (sysvars [SV_OSDISP], "Vista", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.1") == 0) {
    strlcat (sysvars [SV_OSDISP], "7", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.2") == 0) {
    strlcat (sysvars [SV_OSDISP], "8.0", SV_MAX_SZ);
  }
  else if (strcmp (sysvars [SV_OSVERS], "6.3") == 0) {
    strlcat (sysvars [SV_OSDISP], "8.1", SV_MAX_SZ);
  }
  else {
    strlcat (sysvars [SV_OSDISP], sysvars [SV_OSVERS], SV_MAX_SZ);
  }
  strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
  strlcat (sysvars [SV_OSDISP], sysvars [SV_OSBUILD], SV_MAX_SZ);
#endif
  stringToLower (sysvars [SV_OSNAME]);
  if (sizeof (void *) == 8) {
    lsysvars [SVL_OSBITS] = 64;
  } else {
    lsysvars [SVL_OSBITS] = 32;
  }

  getHostname (sysvars [SV_HOSTNAME], SV_MAX_SZ);

  if (isWindows ()) {
    strlcpy (sysvars [SV_HOME], getenv ("USERPROFILE"), SV_MAX_SZ);
    pathNormPath (sysvars [SV_HOME], SV_MAX_SZ);
  } else {
    strlcpy (sysvars [SV_HOME], getenv ("HOME"), SV_MAX_SZ);
  }

  (void) ! getcwd (tcwd, sizeof (tcwd));

  strlcpy (tbuf, argv0, sizeof (tbuf));
  strlcpy (buff, argv0, sizeof (buff));
  pathNormPath (buff, SV_MAX_SZ);
  if (*buff != '/' &&
      (strlen (buff) > 2 && *(buff + 1) == ':' && *(buff + 2) != '/')) {
    strlcpy (tbuf, tcwd, sizeof (tbuf));
    strlcat (tbuf, buff, sizeof (tbuf));
  }
  /* this gives us the real path to the executable */
  pathRealPath (buff, tbuf, sizeof (buff));
  pathNormPath (buff, sizeof (buff));

  /* strip off the filename */
  p = strrchr (buff, '/');
  *p = '\0';
  strlcpy (sysvars [SV_BDJ4EXECDIR], buff, SV_MAX_SZ);

  /* strip off '/bin' */
  p = strrchr (buff, '/');
  *p = '\0';
  strlcpy (sysvars [SV_BDJ4MAINDIR], buff, SV_MAX_SZ);

  if (fileopIsDirectory ("data")) {
    /* if there is a data directory in the current working directory  */
    /* a change of directories is contra-indicated.                   */

    pathNormPath (tcwd, SV_MAX_SZ);
    strlcpy (sysvars [SV_BDJ4DATATOPDIR], tcwd, SV_MAX_SZ);
  } else {
    if (isMacOS ()) {
      strlcpy (buff, sysvars [SV_HOME], SV_MAX_SZ);
      strlcat (buff, "/Library/Application Support/BDJ4", SV_MAX_SZ);
      strlcpy (sysvars [SV_BDJ4DATATOPDIR], buff, SV_MAX_SZ);
    } else {
      strlcpy (sysvars [SV_BDJ4DATATOPDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
    }
  }

  /* on mac os, the data directory is separated */
  /* full path is also needed so that symlinked bdj4 directories will work */

  strlcpy (sysvars [SV_BDJ4DATADIR], "data", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4IMGDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4IMGDIR], "/img", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4LOCALEDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4LOCALEDIR], "/locale", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4TEMPLATEDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4TEMPLATEDIR], "/templates", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4HTTPDIR], "http", SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4TMPDIR], "tmp", SV_MAX_SZ);

  strlcpy (sysvars [SV_SHLIB_EXT], SHLIB_EXT, SV_MAX_SZ);

  strlcpy (sysvars [SV_MOBMQ_HOST], "ballroomdj.org", SV_MAX_SZ);

  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopFileExists (cacertFiles [i])) {
      strlcpy (sysvars [SV_CA_FILE], cacertFiles [i], SV_MAX_SZ);
      break;
    }
  }

  setlocale (LC_ALL, "");

  /* the locale is reset by localeinit */
  snprintf (tbuf, sizeof (tbuf), "%-.5s", setlocale (LC_ALL, NULL));
  strlcpy (sysvars [SV_LOCALE], tbuf, SV_MAX_SZ);
  snprintf (tbuf, sizeof (tbuf), "%-.2s", sysvars [SV_LOCALE]);
  strlcpy (sysvars [SV_SHORT_LOCALE], tbuf, SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_VERSION], "unknown", SV_MAX_SZ);
  if (fileopFileExists ("VERSION.txt")) {
    char    *data;
    char    *tokptr;
    char    *tokptrb;
    char    *tp;
    char    *vnm;
    char    *p;

    data = filedataReadAll ("VERSION.txt");
    tp = strtok_r (data, "\r\n", &tokptr);
    while (tp != NULL) {
      vnm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (strcmp (vnm, "VERSION") == 0) {
        strlcpy (sysvars [SV_BDJ4_VERSION], p, SV_MAX_SZ);
      }
      if (strcmp (vnm, "BUILD") == 0) {
        strlcpy (sysvars [SV_BDJ4_BUILD], p, SV_MAX_SZ);
      }
      if (strcmp (vnm, "BUILDDATE") == 0) {
        strlcpy (sysvars [SV_BDJ4_BUILDDATE], p, SV_MAX_SZ);
      }
      if (strcmp (vnm, "RELEASELEVEL") == 0) {
        strlcpy (sysvars [SV_BDJ4_RELEASELEVEL], p, SV_MAX_SZ);
      }
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
    free (data);
  }

  strlcpy (sysvars [SV_PYTHON_PATH], "", SV_MAX_SZ);
  tptr = strdup (getenv ("PATH"));
  tsep = ":";
  if (isWindows ()) {
    tsep = ";";
  }
  p = strtok_r (tptr, tsep, &tokstr);
  while (p != NULL) {
    strlcpy (tbuf, p, sizeof (tbuf));
    pathNormPath (tbuf, sizeof (tbuf));

    /* python checks */
    snprintf (buff, sizeof (buff), "%s/%s", tbuf, "python3.exe");
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [SV_PYTHON_PATH], buff, SV_MAX_SZ);
      break;
    }
    snprintf (buff, sizeof (buff), "%s/%s", tbuf, "python.exe");
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [SV_PYTHON_PATH], buff, SV_MAX_SZ);
      break;
    }
    snprintf (buff, sizeof (buff), "%s/%s", tbuf, "python3");
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [SV_PYTHON_PATH], buff, SV_MAX_SZ);
      break;
    }
    snprintf (buff, sizeof (buff), "%s/%s", tbuf, "python");
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [SV_PYTHON_PATH], buff, SV_MAX_SZ);
      break;
    }
    p = strtok_r (NULL, tsep, &tokstr);
  }
  free (tptr);

  /* the launcher is not in the right directory. */
  /* don't bother with this if tmp is not there */
  if (fileopIsDirectory ("tmp") &&
      *sysvars [SV_PYTHON_PATH]) {
    char    *data;
    int     j;

    snprintf (buff, sizeof (buff), "%s --version > %s",
        sysvars [SV_PYTHON_PATH], SV_TMP_FILE);
    system (buff);
    data = filedataReadAll (SV_TMP_FILE);
    p = strstr (data, "3");
    if (p != NULL) {
      strlcpy (buff, p, sizeof (buff));
      p = strstr (buff, ".");
      if (p != NULL) {
        p = strstr (p + 1, ".");
        if (p != NULL) {
          *p = '\0';
          strlcpy (sysvars [SV_PYTHON_DOT_VERSION], buff, SV_MAX_SZ);
          j = 0;
          for (size_t i = 0; i < strlen (buff); ++i) {
            if (buff [i] != '.') {
              sysvars [SV_PYTHON_VERSION][j++] = buff [i];
            }
          }
          sysvars [SV_PYTHON_VERSION][j] = '\0';
        }
      } /* found the first '.' */
    } /* found the '3' starting the python version */
    free (data);
  } /* if python was found */

  // $HOME/.local/bin/mutagen-inspect
  // %USERPROFILE%/AppData/Local/Programs/Python/Python<pyver>/Scripts/mutagen-inspect-script.py
  // $HOME/Library/Python/<pyver>/lib/python/site-packages

  if (isWindows ()) {
    snprintf (buff, sizeof (buff),
        "%s/AppData/Local/Programs/Python/Python%s/Scripts/%s",
        sysvars [SV_HOME], sysvars [SV_PYTHON_VERSION], "mutagen-inspect-script.py");
  }
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff),
        "%s/Library/Python/%s/lib/python/site-packages/%s",
        sysvars [SV_HOME], sysvars [SV_PYTHON_DOT_VERSION], "mutagen-inspect");
  }
  if (isLinux ()) {
    snprintf (buff, sizeof (buff),
        "%s/.local/bin/%s", sysvars [SV_HOME], "mutagen-inspect");
  }
  if (fileopFileExists (buff)) {
    strlcpy (sysvars [SV_PYTHON_MUTAGEN], buff, SV_MAX_SZ);
  }

  lsysvars [SVL_BDJIDX] = 0;
  lsysvars [SVL_BASEPORT] = 35548;

  lsysvars [SVL_NUM_PROC] = 2;
  if (isWindows ()) {
    tptr = getenv ("NUMBER_OF_PROCESSORS");
    if (tptr != NULL) {
      lsysvars [SVL_NUM_PROC] = atoi (tptr);
    }
  } else if (fileopIsDirectory ("tmp")) {
    /* don't bother with this if tmp is not there */
    snprintf (buff, sizeof (buff), "getconf _NPROCESSORS_ONLN> %s", SV_TMP_FILE);
    system (buff);
    tptr = filedataReadAll (SV_TMP_FILE);
    if (tptr != NULL) {
      lsysvars [SVL_NUM_PROC] = atoi (tptr);
    }
    free (tptr);
  }
  lsysvars [SVL_NUM_PROC] -= 1;  // leave one process free

  if (strcmp (sysvars [SV_BDJ4_RELEASELEVEL], "alpha") == 0) {
    enable_core_dump ();
  }

  fileopDelete (SV_TMP_FILE);
}

inline char *
sysvarsGetStr (sysvarkey_t idx)
{
  if (idx >= SV_MAX) {
    return NULL;
  }

  return sysvars [idx];
}

inline ssize_t
sysvarsGetNum (sysvarlkey_t idx)
{
  if (idx >= SVL_MAX) {
    return -1;
  }

  return lsysvars [idx];
}


inline void
sysvarsSetStr (sysvarkey_t idx, char *value)
{
  if (idx >= SV_MAX) {
    return;
  }

  strlcpy (sysvars [idx], value, SV_MAX_SZ);
}

inline void
sysvarsSetNum (sysvarlkey_t idx, ssize_t value)
{
  if (idx >= SVL_MAX) {
    return;
  }

  lsysvars [idx] = value;
}

inline bool
isMacOS (void)
{
  return (strcmp (sysvars[SV_OSNAME], "darwin") == 0);
}

inline bool
isWindows (void)
{
  return (strcmp (sysvars[SV_OSNAME], "windows") == 0);
}

inline bool
isLinux (void)
{
  return (strcmp (sysvars[SV_OSNAME], "linux") == 0);
}

/* internal routines */

static void
enable_core_dump (void)
{
#if _lib_setrlimit
  struct rlimit corelim;

  corelim.rlim_cur = RLIM_INFINITY;
  corelim.rlim_max = RLIM_INFINITY;

  setrlimit (RLIMIT_CORE, &corelim);
#endif
  return;
}

