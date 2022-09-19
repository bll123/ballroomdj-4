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
#include "osprocess.h"
#include "osutils.h"
#include "pathutil.h"

typedef struct {
  const char  *desc;
} sysvarsdesc_t;

/* for debugging */
static sysvarsdesc_t sysvarsdesc [SV_MAX] = {
  [SV_BDJ4_BUILD] = { "BDJ4_BUILD" },
  [SV_BDJ4_BUILDDATE] = { "BDJ4_BUILDDATE" },
  [SV_BDJ4DATADIR] = { "BDJ4DATADIR" },
  [SV_BDJ4DATATOPDIR] = { "BDJ4DATATOPDIR" },
  [SV_BDJ4EXECDIR] = { "BDJ4EXECDIR" },
  [SV_BDJ4HTTPDIR] = { "BDJ4HTTPDIR" },
  [SV_BDJ4IMGDIR] = { "BDJ4IMGDIR" },
  [SV_BDJ4INSTDIR] = { "BDJ4INSTDIR" },
  [SV_BDJ4LOCALEDIR] = { "BDJ4LOCALEDIR" },
  [SV_BDJ4MAINDIR] = { "BDJ4MAINDIR" },
  [SV_BDJ4_RELEASELEVEL] = { "BDJ4_RELEASELEVEL" },
  [SV_BDJ4TEMPLATEDIR] = { "BDJ4TEMPLATEDIR" },
  [SV_BDJ4TMPDIR] = { "BDJ4TMPDIR" },
  [SV_BDJ4_VERSION] = { "BDJ4_VERSION" },
  [SV_CA_FILE] = { "CA_FILE" },
  [SV_CONFIG_DIR] = { "CONFIG_DIR" },
  [SV_FONT_DEFAULT] = { "FONT_DEFAULT" },
  [SV_HOME] = { "HOME" },
  [SV_HOST_FORUM] = { "HOST_FORUM" },
  [SV_HOST_MOBMQ] = { "HOST_MOBMQ" },
  [SV_HOSTNAME] = { "HOSTNAME" },
  [SV_HOST_SUPPORTMSG] = { "HOST_SUPPORTMSG" },
  [SV_HOST_TICKET] = { "HOST_TICKET" },
  [SV_HOST_WEB] = { "HOST_WEB" },
  [SV_HOST_WIKI] = { "HOST_WIKI" },
  [SV_LOCALE] = { "LOCALE" },
  [SV_LOCALE_RADIX] = { "LOCALE_RADIX" },
  [SV_LOCALE_SHORT] = { "LOCALE_SHORT" },
  [SV_LOCALE_SYSTEM] = { "LOCALE_SYSTEM" },
  [SV_OSBUILD] = { "OSBUILD" },
  [SV_OSDISP] = { "OSDISP" },
  [SV_OS_EXEC_EXT] = { "OS_EXEC_EXT" },
  [SV_OSNAME] = { "OSNAME" },
  [SV_OSVERS] = { "OSVERS" },
  [SV_PATH_GSETTINGS] = { "PATH_GSETTINGS" },
  [SV_PATH_PYTHON] = { "PATH_PYTHON" },
  [SV_PATH_PYTHON_PIP] = { "PATH_PYTHON_PIP" },
  [SV_PATH_VLC] = { "PATH_VLC" },
  [SV_PATH_XDGUSERDIR] = { "PATH_XDGUSERDIR" },
  [SV_PYTHON_DOT_VERSION] = { "PYTHON_DOT_VERSION" },
  [SV_PYTHON_MUTAGEN] = { "PYTHON_MUTAGEN" },
  [SV_PYTHON_VERSION] = { "PYTHON_VERSION" },
  [SV_SHLIB_EXT] = { "SHLIB_EXT" },
  [SV_THEME_DEFAULT] = { "THEME_DEFAULT" },
  [SV_URI_FORUM] = { "URI_FORUM" },
  [SV_URI_MOBMQ_POST] = { "URI_MOBMQ_POST" },
  [SV_URI_MOBMQ] = { "URI_MOBMQ" },
  [SV_URI_REGISTER] = { "URI_REGISTER" },
  [SV_URI_SUPPORTMSG] = { "URI_SUPPORTMSG" },
  [SV_URI_TICKET] = { "URI_TICKET" },
  [SV_URI_WIKI] = { "URI_WIKI" },
  [SV_USER_AGENT] = { "USER_AGENT" },
  [SV_USER_MUNGE] = { "USER_MUNGE" },
  [SV_USER] = { "USER" },
  [SV_WEB_VERSION_FILE] = { "WEB_VERSION_FILE" },
};

static sysvarsdesc_t sysvarsldesc [SVL_MAX] = {
  [SVL_BDJIDX] = { "Profile" },
  [SVL_BASEPORT] = { "Base-Port" },
  [SVL_OSBITS] = { "OS-Bits" },
  [SVL_NUM_PROC] = { "Number-of-Processors" },
};

enum {
  SV_MAX_SZ = 512,
};

static char       sysvars [SV_MAX][SV_MAX_SZ];
static ssize_t    lsysvars [SVL_MAX];

static char *cacertFiles [] = {
  "/etc/ssl/certs/ca-certificates.crt",
  "/opt/local/etc/openssl/cert.pem",
  "http/curl-ca-bundle.crt",
  "templates/curl-ca-bundle.crt",
  "plocal/etc/ssl/cert.pem",
};
enum {
  CACERT_FILE_COUNT = (sizeof (cacertFiles) / sizeof (char *))
};

static void enable_core_dump (void);
static void checkForFile (char *path, int idx, ...);
static bool svGetLinuxOSInfo (char *fn);
static void svGetLinuxDefaultTheme (void);
static void svGetSystemFont (void);

void
sysvarsInit (const char *argv0)
{
  char          tbuff [SV_MAX_SZ+1];
  char          tcwd [SV_MAX_SZ+1];
  char          buff [SV_MAX_SZ+1];
  char          *tptr;
  char          *p;
  size_t        dlen;
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
  strlcpy (sysvars [SV_OS_EXEC_EXT], "", SV_MAX_SZ);
#endif
#if _lib_RtlGetVersion
  NTSTATUS RtlGetVersion (PRTL_OSVERSIONINFOEXW lpVersionInformation);
  memset (&osvi, 0, sizeof (RTL_OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOEXW);
  RtlGetVersion (&osvi);

  strlcpy (sysvars [SV_OS_EXEC_EXT], ".exe", SV_MAX_SZ);
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
  stringAsciiToLower (sysvars [SV_OSNAME]);
  if (sizeof (void *) == 8) {
    lsysvars [SVL_OSBITS] = 64;
  } else {
    lsysvars [SVL_OSBITS] = 32;
  }

  getHostname (sysvars [SV_HOSTNAME], SV_MAX_SZ);

  if (isWindows ()) {
    strlcpy (sysvars [SV_HOME], getenv ("USERPROFILE"), SV_MAX_SZ);
    pathNormPath (sysvars [SV_HOME], SV_MAX_SZ);
    strlcpy (sysvars [SV_USER], getenv ("USERNAME"), SV_MAX_SZ);
  } else {
    strlcpy (sysvars [SV_HOME], getenv ("HOME"), SV_MAX_SZ);
    strlcpy (sysvars [SV_USER], getenv ("USER"), SV_MAX_SZ);
  }
  dlen = strlen (sysvars [SV_USER]) + 1;
  tptr = filedataReplace (sysvars [SV_USER], &dlen, " ", "-");
  strlcpy (sysvars [SV_USER_MUNGE], tptr, SV_MAX_SZ);
  free (tptr);

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/AppData/Roaming/%s", sysvars [SV_HOME], BDJ4_NAME);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s/.config/%s", sysvars [SV_HOME], BDJ4_NAME);
  }
  strlcpy (sysvars [SV_CONFIG_DIR], tbuff, SV_MAX_SZ);

  (void) ! getcwd (tcwd, sizeof (tcwd));

  strlcpy (tbuff, argv0, sizeof (tbuff));
  strlcpy (buff, argv0, sizeof (buff));
  pathNormPath (buff, SV_MAX_SZ);
  if (*buff != '/' &&
      (strlen (buff) > 2 && *(buff + 1) == ':' && *(buff + 2) != '/')) {
    strlcpy (tbuff, tcwd, sizeof (tbuff));
    strlcat (tbuff, buff, sizeof (tbuff));
  }
  /* this gives us the real path to the executable */
  pathRealPath (buff, tbuff, sizeof (buff));
  pathNormPath (buff, sizeof (buff));

  /* strip off the filename */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4EXECDIR] = '\0';
  if (p != NULL) {
    *p = '\0';
    strlcpy (sysvars [SV_BDJ4EXECDIR], buff, SV_MAX_SZ);
  }

  /* strip off '/bin' */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4MAINDIR] = '\0';
  if (p != NULL) {
    *p = '\0';
    strlcpy (sysvars [SV_BDJ4MAINDIR], buff, SV_MAX_SZ);
  }

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

  strlcpy (sysvars [SV_BDJ4INSTDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4INSTDIR], "/install", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4LOCALEDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4LOCALEDIR], "/locale", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4TEMPLATEDIR], sysvars [SV_BDJ4MAINDIR], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4TEMPLATEDIR], "/templates", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4HTTPDIR], "http", SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4TMPDIR], "tmp", SV_MAX_SZ);

  strlcpy (sysvars [SV_SHLIB_EXT], SHLIB_EXT, SV_MAX_SZ);

  strlcpy (sysvars [SV_HOST_MOBMQ], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_MOBMQ], "/marquee4.html", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_MOBMQ_POST], "/marquee4.php", SV_MAX_SZ);

  strlcpy (sysvars [SV_HOST_WEB], "https://ballroomdj4.sourceforge.io", SV_MAX_SZ);
  strlcpy (sysvars [SV_WEB_VERSION_FILE], "bdj4version.txt", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_WIKI], "https://sourceforge.net", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_WIKI], "/p/ballroomdj4/wiki/Home", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_FORUM], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_FORUM], "/forum/index.php", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_TICKET], "https://sourceforge.net", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_TICKET], "/p/ballroomdj4/tickets/", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_SUPPORTMSG], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_SUPPORTMSG], "/bdj4support.php", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_REGISTER], "/bdj4register.php", SV_MAX_SZ);

  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopFileExists (cacertFiles [i])) {
      strlcpy (sysvars [SV_CA_FILE], cacertFiles [i], SV_MAX_SZ);
      break;
    }
    if (*cacertFiles [i] != '/') {
      snprintf (tbuff, sizeof (tbuff), "%s/%s",
        sysvars [SV_BDJ4MAINDIR], cacertFiles [i]);
      if (fileopFileExists (tbuff)) {
        strlcpy (sysvars [SV_CA_FILE], tbuff, SV_MAX_SZ);
        break;
      }
    }
  }

  /* want to avoid having setlocale() linked in to sysvars. */
  /* so these defaults are all wrong */
  /* the locale is reset by localeinit */
  /* localeinit will also convert the windows names to something normal */
  strlcpy (sysvars [SV_LOCALE_SYSTEM], "en_GB.UTF-8", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE], "en_GB", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE_RADIX], ".", SV_MAX_SZ);

  snprintf (buff, sizeof (buff), "%s/locale.txt", sysvars [SV_BDJ4DATADIR]);
  lsysvars [SVL_LOCALE_SET] = 0;
  if (fileopFileExists (buff)) {
    FILE    *fh;

    fh = fopen (buff, "r");
    fgets (tbuff, sizeof (tbuff), fh);
    stringTrim (tbuff);
    fclose (fh);
    if (*tbuff) {
      strlcpy (sysvars [SV_LOCALE], tbuff, SV_MAX_SZ);
      lsysvars [SVL_LOCALE_SET] = 1;
    }
  }

  snprintf (buff, sizeof (buff), "%-.2s", tbuff);
  strlcpy (sysvars [SV_LOCALE_SHORT], buff, SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_VERSION], "unknown", SV_MAX_SZ);
  snprintf (buff, sizeof (buff), "%s/VERSION.txt", sysvars [SV_BDJ4MAINDIR]);
  if (fileopFileExists (buff)) {
    char    *data;
    char    *tokptr;
    char    *tokptrb;
    char    *tp;
    char    *vnm;
    char    *p;

    data = filedataReadAll (buff, NULL);
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

  snprintf (sysvars [SV_USER_AGENT], SV_MAX_SZ,
      "%s/%s ( https://ballroomdj.org/ )", BDJ4_NAME,
      sysvars [SV_BDJ4_VERSION]);

  sysvarsCheckPaths (NULL);

  if (strcmp (sysvars [SV_OSNAME], "darwin") == 0) {
    char *data;

    strlcpy (sysvars [SV_OSDISP], "MacOS", SV_MAX_SZ);
    data = osRunProgram (sysvars [SV_TEMP_A], "-productVersion", NULL);
    stringTrim (data);
    strlcpy (sysvars [SV_OSVERS], data, SV_MAX_SZ);
    if (data != NULL) {
      if (strcmp (data, "13") > 0) {
        strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
        strlcat (sysvars [SV_OSDISP], data, SV_MAX_SZ);
      } else if (strcmp (data, "13") > 0) {
        strlcat (sysvars [SV_OSDISP], " Ventura", SV_MAX_SZ);
      } else if (strcmp (data, "12") > 0) {
        strlcat (sysvars [SV_OSDISP], " Monterey", SV_MAX_SZ);
      } else if (strcmp (data, "11") > 0) {
        strlcat (sysvars [SV_OSDISP], " Big Sur", SV_MAX_SZ);
      } else if (strcmp (data, "10.15") > 0) {
        strlcat (sysvars [SV_OSDISP], " Catalina", SV_MAX_SZ);
      } else if (strcmp (data, "10.14") > 0) {
        strlcat (sysvars [SV_OSDISP], " Mojave", SV_MAX_SZ);
      } else {
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

    /* gtk cannot seem to retrieve the properties from settings */
    /* so run the gsettings program to get the info */
    if (*sysvars [SV_PATH_GSETTINGS]) {
      svGetLinuxDefaultTheme ();
    }
  }

  svGetSystemFont ();

  sysvarsCheckPython ();
  sysvarsCheckMutagen ();

  lsysvars [SVL_BDJIDX] = 0;
  lsysvars [SVL_BASEPORT] = 32548;
  strlcpy (buff, "data/baseport.txt", sizeof (buff));
  if (fileopFileExists (buff)) {
    FILE    *fh;

    fh = fopen (buff, "r");
    fgets (tbuff, sizeof (tbuff), fh);
    stringTrim (tbuff);
    fclose (fh);
    lsysvars [SVL_BASEPORT] = atoi (tbuff);
  }

  lsysvars [SVL_NUM_PROC] = 2;
  if (isWindows ()) {
    tptr = getenv ("NUMBER_OF_PROCESSORS");
    if (tptr != NULL) {
      lsysvars [SVL_NUM_PROC] = atoi (tptr);
    }
  } else {
#if _lib_sysconf
    lsysvars [SVL_NUM_PROC] = sysconf (_SC_NPROCESSORS_ONLN);
#endif
  }
  if (lsysvars [SVL_NUM_PROC] > 1) {
    lsysvars [SVL_NUM_PROC] -= 1;  // leave one process free
  }

  if (strcmp (sysvars [SV_BDJ4_RELEASELEVEL], "alpha") == 0) {
    enable_core_dump ();
  }
}

void
sysvarsCheckPaths (const char *otherpaths)
{
  char    *p;
  char    *tsep;
  char    *tokstr;
  char    tbuff [MAXPATHLEN];
  char    tpath [4096];

  strlcpy (sysvars [SV_PATH_GSETTINGS], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_PYTHON], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_PYTHON_PIP], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_XDGUSERDIR], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_TEMP_A], "", SV_MAX_SZ);

  tsep = ":";
  if (isWindows ()) {
    tsep = ";";
  }
  strlcpy (tpath, getenv ("PATH"), sizeof (tpath));
  stringTrimChar (tpath, *tsep);
  strlcat (tpath, tsep, sizeof (tpath));
  if (otherpaths != NULL && *otherpaths) {
    strlcat (tpath, otherpaths, sizeof (tpath));
  }
  p = strtok_r (tpath, tsep, &tokstr);
  while (p != NULL) {
    if (strstr (p, "WindowsApps") != NULL) {
      /* the windows python does not have a regular path for the pip3 user */
      /* installed scripts */
      p = strtok_r (NULL, tsep, &tokstr);
      continue;
    }

    strlcpy (tbuff, p, sizeof (tbuff));
    pathNormPath (tbuff, sizeof (tbuff));
    stringTrimChar (tbuff, '/');

    if (*sysvars [SV_PATH_PYTHON_PIP] == '\0') {
      checkForFile (tbuff, SV_PATH_PYTHON_PIP, "pip3", "pip", NULL);
    }

    if (*sysvars [SV_PATH_PYTHON] == '\0') {
      checkForFile (tbuff, SV_PATH_PYTHON, "python3", "python", NULL);
    }

    if (*sysvars [SV_PATH_GSETTINGS] == '\0') {
      checkForFile (tbuff, SV_PATH_GSETTINGS, "gsettings", NULL);
    }

    if (*sysvars [SV_PATH_XDGUSERDIR] == '\0') {
      checkForFile (tbuff, SV_PATH_XDGUSERDIR, "xdg-user-dir", NULL);
    }

    if (*sysvars [SV_TEMP_A] == '\0') {
      checkForFile (tbuff, SV_TEMP_A, "sw_vers", NULL);
    }

    p = strtok_r (NULL, tsep, &tokstr);
  }

  strlcpy (sysvars [SV_PATH_VLC], "", SV_MAX_SZ);
  if (isWindows ()) {
    strlcpy (tbuff, "C:/Program Files/VideoLAN/VLC", sizeof (tbuff));
  }
  if (isMacOS ()) {
    strlcpy (tbuff, "/Applications/VLC.app/Contents/MacOS/lib/", sizeof (tbuff));
  }
  if (isLinux ()) {
    strlcpy (tbuff, "/usr/lib/x86_64-linux-gnu/libvlc.so.5", sizeof (tbuff));
  }
  if (fileopIsDirectory (tbuff) || fileopFileExists (tbuff)) {
    strlcpy (sysvars [SV_PATH_VLC], tbuff, SV_MAX_SZ);
  }
}

void
sysvarsCheckPython (void)
{
  if (*sysvars [SV_PATH_PYTHON]) {
    char    buff [SV_MAX_SZ];
    char    *data;
    char    *p;
    int     j;

    data = osRunProgram (sysvars [SV_PATH_PYTHON], "--version", NULL);

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
    } else {
      /* possibly the windows store version that is not installed */
      /* clear the path */
      strcpy (sysvars [SV_PATH_PYTHON], "");
    }
    free (data);
  } /* if python was found */
}

void
sysvarsCheckMutagen (void)
{
  char  buff [SV_MAX_SZ];

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
          "%s/.local/bin/%s", sysvars [SV_HOME], "mutagen-inspect");
      if (fileopFileExists (buff)) {
        strlcpy (sysvars [SV_PYTHON_MUTAGEN], buff, SV_MAX_SZ);
      }
    }
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
    snprintf (buff, sizeof (buff), "%s/%s%s", path, fn, sysvars [SV_OS_EXEC_EXT]);
    if (fileopFileExists (buff)) {
      strlcpy (sysvars [idx], buff, SV_MAX_SZ);
      found = true;
    }
  }

  va_end (valist);
}

static bool
svGetLinuxOSInfo (char *fn)
{
  static char *prettytag = "PRETTY_NAME=";
  static char *desctag = "DISTRIB_DESCRIPTION=";
  static char *reltag = "DISTRIB_RELEASE=";
  static char *verstag = "VERSION_ID=";
  FILE        *fh;
  char        tbuff [MAXPATHLEN];
  char        buff [MAXPATHLEN];
  bool        rc = false;
  bool        haveprettyname = false;
  bool        havevers = false;

  fh = fopen (fn, "r");
  if (fh != NULL) {
    while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
      if (! haveprettyname &&
          strncmp (tbuff, prettytag, strlen (prettytag)) == 0) {
        strlcpy (buff, tbuff + strlen (prettytag) + 1, sizeof (buff));
        stringTrim (buff);
        stringTrimChar (buff, '"');
        strlcpy (sysvars [SV_OSDISP], buff, SV_MAX_SZ);
        haveprettyname = true;
        rc = true;
      }
      if (! haveprettyname &&
          strncmp (tbuff, desctag, strlen (desctag)) == 0) {
        strlcpy (buff, tbuff + strlen (desctag) + 1, sizeof (buff));
        stringTrim (buff);
        stringTrimChar (buff, '"');
        strlcpy (sysvars [SV_OSDISP], buff, SV_MAX_SZ);
        haveprettyname = true;
        rc = true;
      }
      if (! havevers &&
          strncmp (tbuff, reltag, strlen (reltag)) == 0) {
        strlcpy (buff, tbuff + strlen (reltag), sizeof (buff));
        stringTrim (buff);
        strlcpy (sysvars [SV_OSVERS], buff, SV_MAX_SZ);
        rc = true;
      }
      if (! havevers &&
          strncmp (tbuff, verstag, strlen (verstag)) == 0) {
        strlcpy (buff, tbuff + strlen (verstag) + 1, sizeof (buff));
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

static void
svGetLinuxDefaultTheme (void)
{
  char    *tptr;

  tptr = osRunProgram (sysvars [SV_PATH_GSETTINGS], "get",
      "org.gnome.desktop.interface", "gtk-theme", NULL);
  if (tptr != NULL) {
    /* gsettings puts quotes around the data */
    stringTrim (tptr);
    stringTrimChar (tptr, '\'');
    strlcpy (sysvars [SV_THEME_DEFAULT], tptr + 1, SV_MAX_SZ);
  }
  free (tptr);
}

static void
svGetSystemFont (void)
{
  char    *tptr;

  tptr = osGetSystemFont (sysvars [SV_PATH_GSETTINGS]);
  if (tptr != NULL) {
    strlcpy (sysvars [SV_FONT_DEFAULT], tptr, SV_MAX_SZ);
  }
  free (tptr);
}
