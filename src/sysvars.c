#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#if _sys_resource
# include <sys/resource.h>
#endif
#if _sys_utsname
# include <sys/utsname.h>
#endif
#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
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
#include "osutils.h"
#include "pathutil.h"

typedef struct {
  const char  *desc;
} sysvarsdesc_t;

/* for debugging */
static sysvarsdesc_t sysvarsdesc [SV_MAX] = {
  [SV_BDJ4_BUILD] = { "BDJ4-Build-Number" },
  [SV_BDJ4_BUILDDATE] = { "BDJ4-Build-Date" },
  [SV_BDJ4DATADIR] = { "Dir-Data" },
  [SV_BDJ4DATATOPDIR] = { "Dir-Data-Top" },
  [SV_BDJ4EXECDIR] = { "Dir-Exec" },
  [SV_BDJ4HTTPDIR] = { "Dir-Http" },
  [SV_BDJ4IMGDIR] = { "Dir-Image" },
  [SV_BDJ4LOCALEDIR] = { "Dir-Locale" },
  [SV_BDJ4MAINDIR] = { "Dir-Main" },
  [SV_BDJ4_RELEASELEVEL] = { "BDJ4-Release-Level" },
  [SV_BDJ4TEMPLATEDIR] = { "Dir-Template" },
  [SV_BDJ4TMPDIR] = { "Dir-Temp" },
  [SV_BDJ4_VERSION] = { "BDJ4-Version" },
  [SV_CA_FILE] = { "CA-File" },
  [SV_GETCONF_PATH] = { "Path-getconf" },
  [SV_HOME] = { "Path-Home" },
  [SV_HOSTNAME] = { "Hostname" },
  [SV_LOCALE] = { "Locale" },
  [SV_LOCALE_SHORT] = { "Locale-Short" },
  [SV_LOCALE_SYSTEM] = { "Locale-System" },
  [SV_MOBMQ_HOST] = { "Mobmq-Host" },
  [SV_MOBMQ_URL] = { "Mobmq-URL" },
  [SV_OSBUILD] = { "OS-Build" },
  [SV_OSDISP] = { "OS-Display" },
  [SV_OSNAME] = { "OS-Name" },
  [SV_OSVERS] = { "OS-Version" },
  [SV_PYTHON_DOT_VERSION] = { "Version-Python-Dot" },
  [SV_PYTHON_MUTAGEN] = { "Path-mutagen" },
  [SV_PYTHON_PATH] = { "Path-python" },
  [SV_PYTHON_PIP_PATH] = { "Path-pip" },
  [SV_PYTHON_VERSION] = { "Version-Python" },
  [SV_SHLIB_EXT] = { "Sharedlib-Extension" },
  [SV_WEB_HOST] = { "Host-Web" },
};

static sysvarsdesc_t sysvarsldesc [SVL_MAX] = {
  [SVL_BDJIDX] = { "Profile" },
  [SVL_BASEPORT] = { "Base-Port" },
  [SVL_OSBITS] = { "OS-Bits" },
  [SVL_NUM_PROC] = { "Number-of-Processors" },
  [SVL_LOCALE_SET] = { "Locale-Set" },
};

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
static void checkForFile (char *path, int idx, ...);
static char * svRunProgram (char *prog, char *arg);
static bool svGetLinuxOSInfo (char *fn);

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
#if _lib_RtlGetVersion
  RTL_OSVERSIONINFOEXW osvi;
#endif


#if _lib_uname
  rc = uname (&ubuf);
  assert (rc == 0);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSDISP], ubuf.sysname, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSBUILD], "", SV_MAX_SZ);
#endif
#if _lib_RtlGetVersion
  NTSTATUS RtlGetVersion (PRTL_OSVERSIONINFOEXW lpVersionInformation);
  memset (&osvi, 0, sizeof (RTL_OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOEXW);
  RtlGetVersion (&osvi);

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

  strlcpy (sysvars [SV_MOBMQ_HOST], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_MOBMQ_URL], "/marquee4.html", SV_MAX_SZ);

  strlcpy (sysvars [SV_WEB_HOST], "https://ballroomdj4.sourceforge.io", SV_MAX_SZ);

  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopFileExists (cacertFiles [i])) {
      strlcpy (sysvars [SV_CA_FILE], cacertFiles [i], SV_MAX_SZ);
      break;
    }
  }

  /* want to avoid having setlocale() linked in to sysvars. */
  /* so these defaults are all wrong */
  /* the locale is reset by localeinit */
  /* localeinit will also convert the windows names to something normal */
  strlcpy (sysvars [SV_LOCALE_SYSTEM], "en_GB.UTF-8", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE], "en_GB", SV_MAX_SZ);
  lsysvars [SVL_LOCALE_SET] = 0;

  snprintf (buff, sizeof (buff), "%s/locale.txt", sysvars [SV_BDJ4DATADIR]);
  if (fileopFileExists (buff)) {
    FILE    *fh;

    fh = fopen (buff, "r");
    fgets (tbuf, sizeof (tbuf), fh);
    stringTrim (tbuf);
    fclose (fh);
    if (*tbuf) {
      lsysvars [SVL_LOCALE_SET] = 1;
      strlcpy (sysvars [SV_LOCALE], tbuf, SV_MAX_SZ);
    }
  }

  snprintf (buff, sizeof (buff), "%-.2s", tbuf);
  strlcpy (sysvars [SV_LOCALE_SHORT], buff, SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_VERSION], "unknown", SV_MAX_SZ);
  if (fileopFileExists ("VERSION.txt")) {
    char    *data;
    char    *tokptr;
    char    *tokptrb;
    char    *tp;
    char    *vnm;
    char    *p;

    data = filedataReadAll ("VERSION.txt", NULL);
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
  strlcpy (sysvars [SV_PYTHON_PIP_PATH], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_GETCONF_PATH], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_TEMP_A], "", SV_MAX_SZ);

  tptr = strdup (getenv ("PATH"));
  tsep = ":";
  if (isWindows ()) {
    tsep = ";";
  }
  p = strtok_r (tptr, tsep, &tokstr);
  while (p != NULL) {
    strlcpy (tbuf, p, sizeof (tbuf));
    pathNormPath (tbuf, sizeof (tbuf));

    if (*sysvars [SV_PYTHON_PIP_PATH] == '\0') {
      checkForFile (tbuf, SV_PYTHON_PIP_PATH,
          "pip3.exe", "pip.exe", "pip3", "pip", NULL);
    }

    if (*sysvars [SV_PYTHON_PATH] == '\0') {
      checkForFile (tbuf, SV_PYTHON_PATH,
          "python3.exe", "python.exe", "python3", "python", NULL);
    }

    if (*sysvars [SV_GETCONF_PATH] == '\0') {
      checkForFile (tbuf, SV_GETCONF_PATH,
          "getconf", NULL);
    }

    if (*sysvars [SV_TEMP_A] == '\0') {
      checkForFile (tbuf, SV_TEMP_A, "sw_vers", NULL);
    }

    p = strtok_r (NULL, tsep, &tokstr);
  }
  free (tptr);

  if (strcmp (sysvars [SV_OSNAME], "darwin") == 0) {
    char *data;

    strlcpy (sysvars [SV_OSDISP], "MacOS", SV_MAX_SZ);
    data = svRunProgram (sysvars [SV_TEMP_A], "-productVersion");
    stringTrim (data);
    strlcpy (sysvars [SV_OSVERS], data, SV_MAX_SZ);
    if (data != NULL) {
      if (strcmp (data, "13") > 0) {
        strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
        strlcat (sysvars [SV_OSDISP], data, SV_MAX_SZ);
      } else if (strcmp (data, "12") > 0) {
        strlcat (sysvars [SV_OSDISP], " Monterey", SV_MAX_SZ);
      } else if (strcmp (data, "11") > 0) {
        strlcat (sysvars [SV_OSDISP], " Big Sur", SV_MAX_SZ);
      } else if (strcmp (data, "10.15") > 0) {
        strlcat (sysvars [SV_OSDISP], " Catalina", SV_MAX_SZ);
      } else if (strcmp (data, "10.14") > 0) {
        strlcat (sysvars [SV_OSDISP], " Mojave", SV_MAX_SZ);
      } else if (strcmp (data, "10.13") > 0) {
        strlcat (sysvars [SV_OSDISP], " High Sierra", SV_MAX_SZ);
      } else if (strcmp (data, "10.12") > 0) {
        strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
        strlcat (sysvars [SV_OSDISP], data, SV_MAX_SZ);
      }
    }
    free (data);
  }
  if (strcmp (sysvars [SV_OSNAME], "linux") == 0) {
    static char *fna = "/etc/lsb-release";
    static char *fnb = "/etc/os-release";

    if (! svGetLinuxOSInfo (fna)) {
      svGetLinuxOSInfo (fnb);
    }
  }

  /* the launcher is not in the right directory. */
  /* don't bother with this if tmp is not there */
  if (fileopIsDirectory ("tmp") &&
      *sysvars [SV_PYTHON_PATH]) {
    char    *data;
    int     j;

    data = svRunProgram (sysvars [SV_PYTHON_PATH], "--version");

    p = NULL;
    if (data != NULL) {
      p = strstr (data, "3");
    }

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
  // $HOME/Library/Python/<pydotver>/bin/mutagen-inspect
  // msys2: $HOME/.local/bin/mutagen-inspect (use $HOME, not %USERPROFILE%)

  if (isLinux ()) {
    snprintf (buff, sizeof (buff),
        "%s/.local/bin/%s", sysvars [SV_HOME], "mutagen-inspect");
  }
  if (isWindows ()) {
    snprintf (buff, sizeof (buff),
        "%s/AppData/Local/Programs/Python/Python%s/Scripts/%s",
        sysvars [SV_HOME], sysvars [SV_PYTHON_VERSION], "mutagen-inspect-script.py");
  }
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff),
        "%s/Library/Python/%s/bin/%s",
        sysvars [SV_HOME], sysvars [SV_PYTHON_DOT_VERSION], "mutagen-inspect");
  }
  if (fileopFileExists (buff)) {
    strlcpy (sysvars [SV_PYTHON_MUTAGEN], buff, SV_MAX_SZ);
  } else {
    if (isWindows ()) {
      /* for msys2 testing */
      snprintf (buff, sizeof (buff),
          "%s/.local/bin/%s", getenv ("HOME"), "mutagen-inspect");
      if (fileopFileExists (buff)) {
        strlcpy (sysvars [SV_PYTHON_MUTAGEN], buff, SV_MAX_SZ);
      }
    }
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
    tptr = svRunProgram (sysvars [SV_GETCONF_PATH], "_NPROCESSORS_ONLN");
    if (tptr != NULL) {
      lsysvars [SVL_NUM_PROC] = atoi (tptr);
    }
    free (tptr);
  }
  if (lsysvars [SVL_NUM_PROC] > 1) {
    lsysvars [SVL_NUM_PROC] -= 1;  // leave one process free
  }

  if (strcmp (sysvars [SV_BDJ4_RELEASELEVEL], "alpha") == 0) {
    enable_core_dump ();
  }
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
sysvarsSetStr (sysvarkey_t idx, const char *value)
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

/* for debugging */
const char *
sysvarsDesc (sysvarkey_t idx)
{
  if (idx >= SV_MAX) {
    return "";
  }
  return sysvarsdesc [idx].desc;
}

const char *
sysvarslDesc (sysvarlkey_t idx)
{
  if (idx >= SVL_MAX) {
    return "";
  }
  return sysvarsldesc [idx].desc;
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

static void
checkForFile (char *path, int idx, ...)
{
  va_list   valist;
  char      buff [MAXPATHLEN];
  char      *fn;
  bool      found = false;

  va_start (valist, idx);

  while (! found && (fn = va_arg (valist, char *)) != NULL) {
    snprintf (buff, sizeof (buff), "%s/%s", path, fn);
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [idx], buff, SV_MAX_SZ);
      found = true;
    }
  }

  va_end (valist);
}

static char *
svRunProgram (char *prog, char *arg)
{
  char    *targv [3];
  char    *data;

  targv [0] = prog;
  targv [1] = arg;
  targv [2] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, SV_TMP_FILE);
  data = filedataReadAll (SV_TMP_FILE, NULL);
  fileopDelete (SV_TMP_FILE);
  return data;
}

static bool
svGetLinuxOSInfo (char *fn)
{
  static char *desctag = "PRETTY_NAME=";
  static char *reltag = "DISTRIB_RELEASE=";
  static char *verstag = "VERSION_ID=";
  FILE        *fh;
  char        tbuf [MAXPATHLEN];
  char        buff [MAXPATHLEN];
  bool        rc = false;

  fh = fopen (fn, "r");
  if (fh != NULL) {
    while (fgets (tbuf, sizeof (tbuf), fh) != NULL) {
      if (strncmp (tbuf, desctag, strlen (desctag)) == 0) {
        strlcpy (buff, tbuf + strlen (desctag) + 1, sizeof (buff));
        stringTrim (buff);
        stringTrimChar (buff, '"');
        strlcpy (sysvars [SV_OSDISP], buff, SV_MAX_SZ);
        rc = true;
      }
      if (strncmp (tbuf, reltag, strlen (reltag)) == 0) {
        strlcpy (buff, tbuf + strlen (reltag), sizeof (buff));
        stringTrim (buff);
        strlcpy (sysvars [SV_OSVERS], buff, SV_MAX_SZ);
        rc = true;
      }
      if (strncmp (tbuf, verstag, strlen (verstag)) == 0) {
        strlcpy (buff, tbuf + strlen (verstag) + 1, sizeof (buff));
        stringTrim (buff);
        stringTrimChar (buff, '"');
        strlcpy (sysvars [SV_OSVERS], buff, SV_MAX_SZ);
        rc = true;
      }
    }
    fclose (fh);
  }

  return rc;
}
