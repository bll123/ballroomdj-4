/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
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
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif
#if _hdr_intrin
# include <intrin.h>
#endif

#include "bdj4.h"
#include "sysvars.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "mdebug.h"
#include "osnetutils.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathutil.h"

typedef struct {
  const char  *desc;
} sysvarsdesc_t;

/* for debugging */
static sysvarsdesc_t sysvarsdesc [SV_MAX] = {
  [SV_AUDIOID_MUSICBRAINZ_URI] = { "AUDIOID_MUSICBRAINZ_URI" },
  [SV_AUDIOID_ACOUSTID_URI] = { "AUDIOID_ACOUSTID_URI" },
  [SV_BDJ4_BUILD] = { "BDJ4_BUILD" },
  [SV_BDJ4_BUILDDATE] = { "BDJ4_BUILDDATE" },
  [SV_BDJ4_DEVELOPMENT] = { "BDJ4_DEVELOPMENT" },
  [SV_BDJ4_DIR_DATATOP] = { "BDJ4_DIR_DATATOP" },
  [SV_BDJ4_DIR_EXEC] = { "BDJ4_DIR_EXEC" },
  [SV_BDJ4_DIR_IMG] = { "BDJ4_DIR_IMG" },
  [SV_BDJ4_DIR_INST] = { "BDJ4_DIR_INST" },
  [SV_BDJ4_DIR_LOCALE] = { "BDJ4_DIR_LOCALE" },
  [SV_BDJ4_DIR_MAIN] = { "BDJ4_DIR_MAIN" },
  [SV_BDJ4_DIR_SCRIPT] = { "BDJ4_DIR_SCRIPT" },
  [SV_BDJ4_DIR_TEMPLATE] = { "BDJ4_DIR_TEMPLATE" },
  [SV_BDJ4_DREL_DATA] = { "BDJ4_DREL_DATA" },
  [SV_BDJ4_DREL_HTTP] = { "BDJ4_DREL_HTTP" },
  [SV_BDJ4_DREL_IMG] = { "BDJ4_DREL_IMG" },
  [SV_BDJ4_DREL_TMP] = { "BDJ4_DREL_TMP" },
  [SV_BDJ4_RELEASELEVEL] = { "BDJ4_RELEASELEVEL" },
  [SV_BDJ4_VERSION] = { "BDJ4_VERSION" },
  [SV_CA_FILE] = { "CA_FILE" },
  [SV_DIR_CACHE] = { "DIR_CACHE" },
  [SV_DIR_CACHE_BASE] = { "DIR_CACHE_BASE" },
  [SV_DIR_CONFIG] = { "DIR_CONFIG" },
  [SV_DIR_CONFIG_BASE] = { "DIR_CONFIG_BASE" },
  [SV_FILE_ALTCOUNT] = { "FILE_ALTCOUNT" },
  [SV_FILE_ALT_INST_PATH] = { "FILE_ALT_INST_PATH" },
  [SV_FILE_INST_PATH] = { "FILE_INST_PATH" },
  [SV_FONT_DEFAULT] = { "FONT_DEFAULT" },
  [SV_HOME] = { "HOME" },
  [SV_HOST_DOWNLOAD] = { "HOST_DOWNLOAD" },
  [SV_HOST_FORUM] = { "HOST_FORUM" },
  [SV_HOSTNAME] = { "HOSTNAME" },
  [SV_HOST_SUPPORTMSG] = { "HOST_SUPPORTMSG" },
  [SV_HOST_TICKET] = { "HOST_TICKET" },
  [SV_HOST_WEB] = { "HOST_WEB" },
  [SV_HOST_WIKI] = { "HOST_WIKI" },
  [SV_LOCALE] = { "LOCALE" },
  [SV_LOCALE_ISO639_2] = { "LOCALE_ISO639_3" },
  [SV_LOCALE_ORIG] = { "LOCALE_ORIG" },
  [SV_LOCALE_RADIX] = { "LOCALE_RADIX" },
  [SV_LOCALE_SHORT] = { "LOCALE_SHORT" },
  [SV_LOCALE_SYSTEM] = { "LOCALE_SYSTEM" },
  [SV_OSARCH] = { "OSARCH" },
  [SV_OSBUILD] = { "OSBUILD" },
  [SV_OSDISP] = { "OSDISP" },
  [SV_OS_EXEC_EXT] = { "OS_EXEC_EXT" },
  [SV_OSNAME] = { "OSNAME" },
  [SV_OSVERS] = { "OSVERS" },
  [SV_PATH_ACRCLOUD] = { "PATH_ACRCLOUD" },
  [SV_PATH_CRONTAB] = { "PATH_CRONTAB" },
  [SV_PATH_FFMPEG] = { "PATH_FFMPEG" },
  [SV_PATH_FPCALC] = { "PATH_FPCALC" },
  [SV_PATH_MUTAGEN] = { "PATH_MUTAGEN" },
  [SV_PATH_GSETTINGS] = { "PATH_GSETTINGS" },
  [SV_PATH_PYTHON] = { "PATH_PYTHON" },
  [SV_PATH_PYTHON_PIP] = { "PATH_PYTHON_PIP" },
  [SV_PATH_URI_OPEN] = { "PATH_URI_OPEN" },
  [SV_PATH_VLC] = { "PATH_VLC" },
  [SV_PATH_XDGUSERDIR] = { "PATH_XDGUSERDIR" },
  [SV_PYTHON_DOT_VERSION] = { "PYTHON_DOT_VERSION" },
  [SV_PYTHON_VERSION] = { "PYTHON_VERSION" },
  [SV_SHLIB_EXT] = { "SHLIB_EXT" },
  [SV_THEME_DEFAULT] = { "THEME_DEFAULT" },
  [SV_URI_DOWNLOAD] = { "URI_DOWNLOAD" },
  [SV_URI_FORUM] = { "URI_FORUM" },
  [SV_URI_REGISTER] = { "URI_REGISTER" },
  [SV_URI_SUPPORTMSG] = { "URI_SUPPORTMSG" },
  [SV_URI_TICKET] = { "URI_TICKET" },
  [SV_URI_WIKI] = { "URI_WIKI" },
  [SV_USER_AGENT] = { "USER_AGENT" },
  [SV_USER_MUNGE] = { "USER_MUNGE" },
  [SV_USER] = { "USER" },
  [SV_VLC_VERSION] = { "VLC_VERSION" },
  [SV_WEB_VERSION_FILE] = { "WEB_VERSION_FILE" },
};

static sysvarsdesc_t sysvarsldesc [SVL_MAX] = {
  [SVL_ALTIDX] = { "ALTIDX" },
  [SVL_DATAPATH] = { "DATAPATH" },
  [SVL_BASEPORT] = { "BASEPORT" },
  [SVL_BDJIDX] = { "BDJIDX" },
  [SVL_INITIAL_PORT] = { "INITIAL_PORT" },
  [SVL_IS_LINUX] = { "IS_LINUX" },
  [SVL_IS_MACOS] = { "IS_MACOS" },
  [SVL_IS_VM] = { "IS_VM" },
  [SVL_IS_WINDOWS] = { "IS_WINDOWS" },
  [SVL_LOCALE_SET] = { "LOCALE_SET" },
  [SVL_LOCALE_SYS_SET] = { "LOCALE_SYS_SET" },
  [SVL_NUM_PROC] = { "NUM_PROC" },
  [SVL_OSBITS] = { "OSBITS" },
  [SVL_USER_ID] = { "USER_ID" },
};

enum {
  SV_MAX_SZ = 512,
};

static char       sysvars [SV_MAX][SV_MAX_SZ];
static int64_t    lsysvars [SVL_MAX];

static char *cacertFiles [] = {
  "/etc/ssl/certs/ca-certificates.crt",
  "/usr/share/ssl/certs/ca-bundle.crt",
  "/etc/pki/tls/certs/ca-bundle.crt",
  /* updated each time a package is created, used on windows, macos */
  "templates/curl-ca-bundle.crt",
  "http/curl-ca-bundle.crt",
  /* macos */
  "/opt/local/etc/openssl/cert.pem",
  "/opt/local/share/curl/curl-ca-bundle.crt",
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
  char          tcwd [SV_MAX_SZ+1];
  char          tbuff [SV_MAX_SZ+1];
  char          altpath [SV_MAX_SZ+1];
  char          buff [SV_MAX_SZ+1];
  char          rochkbuff [MAXPATHLEN];
  char          *p;
  size_t        dlen;
  bool          alternatepath = false;
  sysversinfo_t *versinfo;
#if _lib_uname
  struct utsname  ubuf;
#endif
#if _lib_RtlGetVersion
  RTL_OSVERSIONINFOEXW osvi;
  NTSTATUS RtlGetVersion (PRTL_OSVERSIONINFOEXW lpVersionInformation);
#endif
#if _lib_GetNativeSystemInfo
  SYSTEM_INFO winsysinfo;
#endif

  enable_core_dump ();

  (void) ! getcwd (tcwd, sizeof (tcwd));
  pathNormalizePath (tcwd, SV_MAX_SZ);

  strlcpy (sysvars [SV_OSNAME], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSDISP], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSVERS], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSARCH], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSBUILD], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_OS_EXEC_EXT], "", SV_MAX_SZ);
  lsysvars [SVL_IS_MACOS] = false;
  lsysvars [SVL_IS_LINUX] = false;
  lsysvars [SVL_IS_WINDOWS] = false;
  lsysvars [SVL_IS_VM] = false;

#if _lib_uname
  uname (&ubuf);
  strlcpy (sysvars [SV_OSNAME], ubuf.sysname, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSDISP], ubuf.sysname, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSVERS], ubuf.version, SV_MAX_SZ);
  strlcpy (sysvars [SV_OSARCH], ubuf.machine, SV_MAX_SZ);

#endif
#if _lib_RtlGetVersion
  memset (&osvi, 0, sizeof (RTL_OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOEXW);
  RtlGetVersion (&osvi);

  snprintf (sysvars [SV_OSVERS], SV_MAX_SZ, "%ld.%ld",
      osvi.dwMajorVersion, osvi.dwMinorVersion);
  snprintf (sysvars [SV_OSBUILD], SV_MAX_SZ, "%ld",
      osvi.dwBuildNumber);
#endif
#if _lib_GetNativeSystemInfo
  GetNativeSystemInfo (&winsysinfo);
  /* dwNumberOfProcessors may not reflect the number of system processors */
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
    strlcpy (sysvars [SV_OSARCH], "intel", SV_MAX_SZ);
    lsysvars [SVL_OSBITS] = 32;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
    strlcpy (sysvars [SV_OSARCH], "arm", SV_MAX_SZ);
    lsysvars [SVL_OSBITS] = 32;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
    strlcpy (sysvars [SV_OSARCH], "arm", SV_MAX_SZ);
    lsysvars [SVL_OSBITS] = 64;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
    strlcpy (sysvars [SV_OSARCH], "", SV_MAX_SZ);
    lsysvars [SVL_OSBITS] = 64;
  }
#endif

/* is a windows machine */
#if _lib_GetNativeSystemInfo || _lib_RtlGetVersion
  strlcpy (sysvars [SV_OS_EXEC_EXT], ".exe", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSNAME], "windows", SV_MAX_SZ);
  strlcpy (sysvars [SV_OSDISP], "Windows ", SV_MAX_SZ);
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

  if (strcmp (sysvars [SV_OSNAME], "darwin") == 0) {
    lsysvars [SVL_IS_MACOS] = true;
  }
  if (strcmp (sysvars [SV_OSNAME], "linux") == 0) {
    lsysvars [SVL_IS_LINUX] = true;
  }
  if (strcmp (sysvars [SV_OSNAME], "windows") == 0) {
    lsysvars [SVL_IS_WINDOWS] = true;
  }

  getHostname (sysvars [SV_HOSTNAME], SV_MAX_SZ);

  if (isWindows ()) {
    osGetEnv ("USERPROFILE", sysvars [SV_HOME], SV_MAX_SZ);
    pathNormalizePath (sysvars [SV_HOME], SV_MAX_SZ);
    osGetEnv ("USERNAME", sysvars [SV_USER], SV_MAX_SZ);
  } else {
    osGetEnv ("HOME", sysvars [SV_HOME], SV_MAX_SZ);
    osGetEnv ("USER", sysvars [SV_USER], SV_MAX_SZ);
  }
  dlen = strlen (sysvars [SV_USER]);
  strlcpy (sysvars [SV_USER_MUNGE], sysvars [SV_USER], SV_MAX_SZ);
  for (size_t i = 0; i < dlen; ++i) {
    if (sysvars [SV_USER_MUNGE][i] == ' ') {
      sysvars [SV_USER_MUNGE][i] = '-';
    }
  }
  lsysvars [SVL_USER_ID] = SVC_USER_ID_NONE;
#if _lib_getuid
  lsysvars [SVL_USER_ID] = getuid ();
#endif

  strlcpy (tbuff, argv0, sizeof (tbuff));
  strlcpy (buff, argv0, sizeof (buff));
  pathNormalizePath (buff, SV_MAX_SZ);
  /* handle relative pathnames */
  if ((strlen (buff) > 2 && *(buff + 1) == ':' && *(buff + 2) != '/') ||
     (*buff != '/' && strlen (buff) > 1 && *(buff + 1) != ':')) {
    strlcpy (tbuff, tcwd, sizeof (tbuff));
    strlcat (tbuff, "/", sizeof (tbuff));
    strlcat (tbuff, buff, sizeof (tbuff));
  }

  /* save this path so that it can be used to check for a data dir */
  strlcpy (altpath, tbuff, sizeof (altpath));
  pathStripPath (altpath, sizeof (altpath));
  pathNormalizePath (altpath, sizeof (altpath));

  /* this gives us the real path to the executable */
  pathRealPath (buff, tbuff, sizeof (buff));
  pathNormalizePath (buff, sizeof (buff));

  if (strcmp (altpath, buff) != 0) {
    alternatepath = true;
  }

  /* strip off the filename */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4_DIR_EXEC] = '\0';
  if (p != NULL) {
    *p = '\0';
    strlcpy (sysvars [SV_BDJ4_DIR_EXEC], buff, SV_MAX_SZ);
  }

  /* strip off '/bin' */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4_DIR_MAIN] = '\0';
  if (p != NULL) {
    *p = '\0';
    strlcpy (sysvars [SV_BDJ4_DIR_MAIN], buff, SV_MAX_SZ);
  }

  snprintf (rochkbuff, sizeof (rochkbuff), "%s%s",
       READONLY_FN, BDJ4_CONFIG_EXT);
  if (fileopIsDirectory ("data") && ! fileopFileExists (rochkbuff)) {
    /* if there is a data directory in the current working directory */
    /* and there is no 'readonly.txt' file */
    /* a change of directories is contra-indicated. */

    strlcpy (sysvars [SV_BDJ4_DIR_DATATOP], tcwd, SV_MAX_SZ);
    lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_LOCAL;
  } else {
    bool found = false;

    /* check for a data directory in the original run-path */
    if (alternatepath) {
      size_t    tlen;

      /* strip filename */
      p = strrchr (altpath, '/');
      if (p != NULL) {
        *p = '\0';
      }
      /* strip /bin */
      p = strrchr (altpath, '/');
      if (p != NULL) {
        *p = '\0';
      }

      snprintf (rochkbuff, sizeof (rochkbuff), "%s/%s%s",
          altpath, READONLY_FN, BDJ4_CONFIG_EXT);
      tlen = strlen (altpath);
      strlcat (altpath, "/data", sizeof (altpath));
      if (fileopIsDirectory (altpath) && ! fileopFileExists (rochkbuff)) {
        /* remove the /data suffix */
        altpath [tlen] = '\0';
        strlcpy (sysvars [SV_BDJ4_DIR_DATATOP], altpath, SV_MAX_SZ);
        found = true;
        lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_ALT;
      }
    }

    if (! found) {
      lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_NORM;

      snprintf (rochkbuff, sizeof (rochkbuff), "%s/%s%s",
         sysvars [SV_BDJ4_DIR_MAIN], READONLY_FN, BDJ4_CONFIG_EXT);

      if (fileopFileExists (rochkbuff)) {
        lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_UNKNOWN;
        *sysvars [SV_BDJ4_DIR_DATATOP]= '\0';
      } else {
        if (isMacOS ()) {
          strlcpy (buff, sysvars [SV_HOME], SV_MAX_SZ);
          strlcat (buff, "/Library/Application Support/BDJ4", SV_MAX_SZ);
          strlcpy (sysvars [SV_BDJ4_DIR_DATATOP], buff, SV_MAX_SZ);
        } else {
          strlcpy (sysvars [SV_BDJ4_DIR_DATATOP], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
        }
      }
    }
  }

  /* on mac os, the data directory is separated */
  /* full path is also needed so that symlinked bdj4 directories will work */

  strlcpy (sysvars [SV_BDJ4_DREL_DATA], "data", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DIR_IMG], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4_DIR_IMG], "/img", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DIR_INST], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4_DIR_INST], "/install", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DIR_LOCALE], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4_DIR_LOCALE], "/locale", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DIR_TEMPLATE], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4_DIR_TEMPLATE], "/templates", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DIR_SCRIPT], sysvars [SV_BDJ4_DIR_MAIN], SV_MAX_SZ);
  strlcat (sysvars [SV_BDJ4_DIR_SCRIPT], "/scripts", SV_MAX_SZ);

  strlcpy (sysvars [SV_BDJ4_DREL_HTTP], "http", SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_DREL_TMP], "tmp", SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_DREL_IMG], "img", SV_MAX_SZ);

  strlcpy (sysvars [SV_SHLIB_EXT], SHLIB_EXT, SV_MAX_SZ);

  strlcpy (sysvars [SV_HOST_DOWNLOAD], "https://sourceforge.net", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_FORUM], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_SUPPORTMSG], "https://ballroomdj.org", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_TICKET], "https://sourceforge.net", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_WEB], "https://ballroomdj4.sourceforge.io", SV_MAX_SZ);
  strlcpy (sysvars [SV_HOST_WIKI], "https://sourceforge.net", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_DOWNLOAD], "/projects/ballroomdj4/files", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_FORUM], "/forum/index.php", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_REGISTER], "/bdj4register.php", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_SUPPORTMSG], "/bdj4support.php", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_TICKET], "/p/ballroomdj4/tickets/", SV_MAX_SZ);
  strlcpy (sysvars [SV_URI_WIKI], "/p/ballroomdj4/wiki/Home", SV_MAX_SZ);
  strlcpy (sysvars [SV_WEB_VERSION_FILE], "bdj4version.txt", SV_MAX_SZ);

  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopFileExists (cacertFiles [i])) {
      strlcpy (sysvars [SV_CA_FILE], cacertFiles [i], SV_MAX_SZ);
      break;
    }
    if (*cacertFiles [i] != '/') {
      snprintf (tbuff, sizeof (tbuff), "%s/%s",
          sysvars [SV_BDJ4_DIR_MAIN], cacertFiles [i]);
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
  strlcpy (sysvars [SV_LOCALE_ORIG], "en_GB", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE], "en_GB", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE_SHORT], "en", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE_ISO639_2], "eng", SV_MAX_SZ);
  strlcpy (sysvars [SV_LOCALE_RADIX], ".", SV_MAX_SZ);

  lsysvars [SVL_LOCALE_SET] = SYSVARS_LOCALE_NOT_SET;
  lsysvars [SVL_LOCALE_SYS_SET] = SYSVARS_LOCALE_NOT_SET;

  /* the installer creates this file to save the original system locale */
  snprintf (buff, sizeof (buff), "%s/localeorig.txt", sysvars [SV_BDJ4_DREL_DATA]);
  if (fileopFileExists (buff)) {
    FILE    *fh;

    *tbuff = '\0';
    fh = fileopOpen (buff, "r");
    if (fh != NULL) {
      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      mdextfclose (fh);
      fclose (fh);
    }
    stringTrim (tbuff);
    if (*tbuff) {
      /* save the system locale */
      strlcpy (sysvars [SV_LOCALE_SYSTEM], tbuff, SV_MAX_SZ);
      /* do not mark locale-set, only locale-sys-set */
      lsysvars [SVL_LOCALE_SYS_SET] = SYSVARS_LOCALE_SET;
      /* localeInit() will set locale-orig and the other variables */
    }
  }

  snprintf (buff, sizeof (buff), "%s/locale.txt", sysvars [SV_BDJ4_DREL_DATA]);
  if (fileopFileExists (buff)) {
    FILE    *fh;

    *tbuff = '\0';
    fh = fileopOpen (buff, "r");
    if (fh != NULL) {
      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      mdextfclose (fh);
      fclose (fh);
    }
    stringTrim (tbuff);
    if (*tbuff) {
      if (strcmp (tbuff, sysvars [SV_LOCALE_SYSTEM]) != 0) {
        strlcpy (sysvars [SV_LOCALE], tbuff, SV_MAX_SZ);
        snprintf (buff, sizeof (buff), "%-.2s", tbuff);
        strlcpy (sysvars [SV_LOCALE_SHORT], buff, SV_MAX_SZ);
        lsysvars [SVL_LOCALE_SET] = SYSVARS_LOCALE_SET;
      }
    }
  }

  strlcpy (sysvars [SV_BDJ4_VERSION], "unknown", SV_MAX_SZ);
  snprintf (buff, sizeof (buff), "%s/VERSION.txt", sysvars [SV_BDJ4_DIR_MAIN]);
  versinfo = sysvarsParseVersionFile (buff);
  strlcpy (sysvars [SV_BDJ4_VERSION], versinfo->version, SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_BUILD], versinfo->build, SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_BUILDDATE], versinfo->builddate, SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_RELEASELEVEL], versinfo->releaselevel, SV_MAX_SZ);
  strlcpy (sysvars [SV_BDJ4_DEVELOPMENT], versinfo->dev, SV_MAX_SZ);
  sysvarsParseVersionFileFree (versinfo);

  if (isWindows ()) {
    snprintf (sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ,
        "%s/AppData/Roaming", sysvars [SV_HOME]);
  } else {
    osGetEnv ("XDG_CONFIG_HOME", sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ);
    if (! *sysvars [SV_DIR_CONFIG_BASE]) {
      snprintf (sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ, "%s/.config",
          sysvars [SV_HOME]);
    }
  }
  pathNormalizePath (sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ);

  snprintf (tbuff, sizeof (tbuff), "%s/%s", sysvars [SV_DIR_CONFIG_BASE], BDJ4_NAME);
  strlcpy (sysvars [SV_DIR_CONFIG], tbuff, SV_MAX_SZ);

  snprintf (tbuff, sizeof (tbuff), "%s/%s%s%s", sysvars [SV_DIR_CONFIG],
      ALT_COUNT_FN, sysvars [SV_BDJ4_DEVELOPMENT], BDJ4_CONFIG_EXT);
  strlcpy (sysvars [SV_FILE_ALTCOUNT], tbuff, SV_MAX_SZ);
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s%s", sysvars [SV_DIR_CONFIG],
      INST_PATH_FN, sysvars [SV_BDJ4_DEVELOPMENT], BDJ4_CONFIG_EXT);
  strlcpy (sysvars [SV_FILE_INST_PATH], tbuff, SV_MAX_SZ);
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s%s", sysvars [SV_DIR_CONFIG],
      ALT_INST_PATH_FN, sysvars [SV_BDJ4_DEVELOPMENT], BDJ4_CONFIG_EXT);
  strlcpy (sysvars [SV_FILE_ALT_INST_PATH], tbuff, SV_MAX_SZ);

  if (isWindows ()) {
    osGetEnv ("TEMP", sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ);
    if (! *sysvars [SV_DIR_CACHE_BASE]) {
      snprintf (sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ,
          "%s/AppData/Local/Temp", sysvars [SV_HOME]);
    }
  } else {
    osGetEnv ("XDG_CACHE_HOME", sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ);
    if (! *sysvars [SV_DIR_CACHE_BASE]) {
      snprintf (sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ, "%s/.cache",
          sysvars [SV_HOME]);
    }
  }
  pathNormalizePath (sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ);

  snprintf (tbuff, sizeof (tbuff), "%s/%s", sysvars [SV_DIR_CACHE_BASE], BDJ4_NAME);
  strlcpy (sysvars [SV_DIR_CACHE], tbuff, SV_MAX_SZ);

  snprintf (sysvars [SV_USER_AGENT], SV_MAX_SZ, "%s/%s ( %s )",
      BDJ4_NAME, sysvars [SV_BDJ4_VERSION], sysvars [SV_HOST_WEB]);

  sysvarsCheckPaths (NULL);

  if (strcmp (sysvars [SV_OSNAME], "darwin") == 0) {
    char  *data;
    char  *tdata;

    strlcpy (sysvars [SV_OSDISP], "macOS", SV_MAX_SZ);
    data = osRunProgram (sysvars [SV_TEMP_A], "-ProductVersion", NULL);
    stringTrim (data);
    strlcpy (sysvars [SV_OSVERS], data, SV_MAX_SZ);
    dataFree (data);

    tdata = osRunProgram (sysvars [SV_TEMP_A], "-ProductVersionExtra", NULL);
    if (tdata != NULL && *tdata == '(') {
      size_t    len;

      stringTrim (tdata);
      len = strlen (tdata);
      *(tdata + len - 1) = '\0';
      strlcat (sysvars [SV_OSVERS], "-", SV_MAX_SZ);
      strlcat (sysvars [SV_OSVERS], tdata + 1, SV_MAX_SZ);
    }
    dataFree (tdata);

    strlcpy (sysvars [SV_OSBUILD], "", SV_MAX_SZ);
    data = osRunProgram (sysvars [SV_TEMP_A], "-BuildVersion", NULL);
    stringTrim (data);
    strlcpy (sysvars [SV_OSBUILD], data, SV_MAX_SZ);
    dataFree (data);

    data = sysvars [SV_OSVERS];
    if (data != NULL) {
      if (strcmp (data, "15") > 0) {
        strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
        strlcat (sysvars [SV_OSDISP], data, SV_MAX_SZ);
      } else if (strcmp (data, "14") > 0) {
        strlcat (sysvars [SV_OSDISP], " Sonoma", SV_MAX_SZ);
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
      } else if (strcmp (data, "10.13") > 0) {
        strlcat (sysvars [SV_OSDISP], " High Sierra", SV_MAX_SZ);
      } else if (strcmp (data, "10.12") > 0) {
        strlcat (sysvars [SV_OSDISP], " Sierra", SV_MAX_SZ);
      } else {
        strlcat (sysvars [SV_OSDISP], " ", SV_MAX_SZ);
        strlcat (sysvars [SV_OSDISP], data, SV_MAX_SZ);
      }
    }
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

  /* the SVL_IS_VM flag is only used for the test suite */
  if (strcmp (sysvars [SV_OSNAME], "linux") == 0) {
    FILE        *fh;
    char        tbuff [2048];
    static char *flagtag = "flags";
    static char *vmtag = " hypervisor ";

    fh = fileopOpen ("/proc/cpuinfo", "r");
    if (fh != NULL) {
      while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
        if (strncmp (tbuff, flagtag, strlen (flagtag)) == 0) {
          if (strstr (tbuff, vmtag) != NULL) {
            lsysvars [SVL_IS_VM] = true;
          }
          break;
        }
      }
      mdextfclose (fh);
      fclose (fh);
    }
  }
  if (strcmp (sysvars [SV_OSNAME], "windows") == 0) {
#if _lib___cpuid
    int     cpuinfo [4];

    __cpuid (cpuinfo, 1);
    if (cpuinfo [2] >> 31 & 1) {
      lsysvars [SVL_IS_VM] = true;
    }
#endif
  }

  svGetSystemFont ();
  sysvarsGetPythonVersion ();
  sysvarsCheckMutagen ();

  lsysvars [SVL_ALTIDX] = 0;
  lsysvars [SVL_BDJIDX] = 0;
  lsysvars [SVL_INITIAL_PORT] = 32548;
  lsysvars [SVL_BASEPORT] = 32548;
  snprintf (buff, sizeof (buff), "data/%s%s", BASE_PORT_FN, BDJ4_CONFIG_EXT);
  if (fileopFileExists (buff)) {
    FILE    *fh;

    *tbuff = '\0';
    fh = fileopOpen (buff, "r");
    if (fh != NULL) {
      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      mdextfclose (fh);
      fclose (fh);
    }
    stringTrim (tbuff);
    if (*tbuff) {
      lsysvars [SVL_BASEPORT] = atoi (tbuff);
    }
  }

  snprintf (buff, sizeof (buff), "data/%s%s", ALT_IDX_FN, BDJ4_CONFIG_EXT);
  if (fileopFileExists (buff)) {
    FILE    *fh;

    *tbuff = '\0';
    fh = fileopOpen (buff, "r");
    if (fh != NULL) {
      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      mdextfclose (fh);
      fclose (fh);
    }
    stringTrim (tbuff);
    if (*tbuff) {
      lsysvars [SVL_ALTIDX] = atoi (tbuff);
    }
  }

  lsysvars [SVL_NUM_PROC] = 2;
  if (isWindows ()) {
    char    tmp [80];

    osGetEnv ("NUMBER_OF_PROCESSORS", tmp, sizeof (tmp));
    if (*tmp) {
      lsysvars [SVL_NUM_PROC] = atoi (tmp);
    }
  } else {
#if _lib_sysconf
    lsysvars [SVL_NUM_PROC] = sysconf (_SC_NPROCESSORS_ONLN);
#endif
  }
  if (lsysvars [SVL_NUM_PROC] > 1) {
    lsysvars [SVL_NUM_PROC] -= 1;  // leave one process free
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

  strlcpy (sysvars [SV_PATH_ACRCLOUD], "", SV_MAX_SZ);
  /* crontab is used on macos during installation */
  strlcpy (sysvars [SV_PATH_CRONTAB], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_FFMPEG], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_FPCALC], "", SV_MAX_SZ);
  /* gsettings is used on linux to get the current theme */
  strlcpy (sysvars [SV_PATH_GSETTINGS], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_MUTAGEN], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_PYTHON_PIP], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_PYTHON], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_PATH_XDGUSERDIR], "", SV_MAX_SZ);
  strlcpy (sysvars [SV_TEMP_A], "", SV_MAX_SZ);

  tsep = ":";
  if (isWindows ()) {
    tsep = ";";
  }
  osGetEnv ("PATH", tpath, sizeof (tpath));
  stringTrimChar (tpath, *tsep);
  strlcat (tpath, tsep, sizeof (tpath));
  if (otherpaths != NULL && *otherpaths) {
    strlcat (tpath, otherpaths, sizeof (tpath));
  }
  p = strtok_r (tpath, tsep, &tokstr);
  while (p != NULL) {
    if (strstr (p, "WindowsApps") != NULL) {
      /* the windows python does not have a regular path for the pip3 */
      /* user installed scripts */
      p = strtok_r (NULL, tsep, &tokstr);
      continue;
    }

    strlcpy (tbuff, p, sizeof (tbuff));
    pathNormalizePath (tbuff, sizeof (tbuff));
    stringTrimChar (tbuff, '/');

    if (*sysvars [SV_PATH_ACRCLOUD] == '\0') {
      checkForFile (tbuff, SV_PATH_ACRCLOUD, "acrcloud", NULL);
    }

    if (*sysvars [SV_PATH_CRONTAB] == '\0') {
      checkForFile (tbuff, SV_PATH_CRONTAB, "crontab", NULL);
    }

    if (*sysvars [SV_PATH_FFMPEG] == '\0') {
      checkForFile (tbuff, SV_PATH_FFMPEG, "ffmpeg", NULL);
    }

    if (*sysvars [SV_PATH_FPCALC] == '\0') {
      checkForFile (tbuff, SV_PATH_FPCALC, "fpcalc", NULL);
    }

    if (*sysvars [SV_PATH_GSETTINGS] == '\0') {
      checkForFile (tbuff, SV_PATH_GSETTINGS, "gsettings", NULL);
    }

    if (*sysvars [SV_PATH_MUTAGEN] == '\0') {
      checkForFile (tbuff, SV_PATH_MUTAGEN, "mutagen-inspect", "mutagen-inspect-3.11", NULL);
    }

    if (*sysvars [SV_PATH_PYTHON] == '\0') {
      checkForFile (tbuff, SV_PATH_PYTHON, "python3", "python", NULL);
    }

    if (*sysvars [SV_PATH_PYTHON_PIP] == '\0') {
      checkForFile (tbuff, SV_PATH_PYTHON_PIP, "pip3", "pip", NULL);
    }

    if (*sysvars [SV_PATH_URI_OPEN] == '\0') {
      checkForFile (tbuff, SV_PATH_URI_OPEN, "xdg-open", "open", NULL);
    }

    if (*sysvars [SV_PATH_XDGUSERDIR] == '\0') {
      checkForFile (tbuff, SV_PATH_XDGUSERDIR, "xdg-user-dir", NULL);
    }

    if (*sysvars [SV_TEMP_A] == '\0') {
      checkForFile (tbuff, SV_TEMP_A, "sw_vers", NULL);
    }

    p = strtok_r (NULL, tsep, &tokstr);
  }

  strlcpy (sysvars [SV_AUDIOID_MUSICBRAINZ_URI],
      "https://musicbrainz.org/ws/2", SV_MAX_SZ);
  strlcpy (sysvars [SV_AUDIOID_ACOUSTID_URI],
      "https://api.acoustid.org/v2/lookup", SV_MAX_SZ);

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
  } else {
    /* one more try for linux (opensuse) */
    if (isLinux ()) {
      strlcpy (tbuff, "/usr/lib64/libvlc.so.5", sizeof (tbuff));
    }
    if (fileopFileExists (tbuff)) {
      strlcpy (sysvars [SV_PATH_VLC], tbuff, SV_MAX_SZ);
    }
  }
}

void
sysvarsGetPythonVersion (void)
{
  if (*sysvars [SV_PATH_PYTHON]) {
    char        tfn [MAXPATHLEN];
    char        buff [SV_MAX_SZ];
    FILE        *fh;

    *sysvars [SV_PYTHON_DOT_VERSION] = '\0';
    *sysvars [SV_PYTHON_VERSION] = '\0';

    /* on windows, use the cache files */
    /* do not use these on linux and macos */
    if (isWindows ()) {
      snprintf (tfn, sizeof (tfn), "%s/%s%s", sysvars [SV_BDJ4_DREL_DATA],
          SYSVARS_PY_DOT_VERS_FN, BDJ4_CONFIG_EXT);
      if (fileopFileExists (tfn)) {
        *buff = '\0';
        fh = fileopOpen (tfn, "r");
        if (fh != NULL) {
          (void) ! fgets (buff, sizeof (buff), fh);
          mdextfclose (fh);
          fclose (fh);
        }
        stringTrim (buff);
        strlcpy (sysvars [SV_PYTHON_DOT_VERSION], buff, SV_MAX_SZ);
      }
      snprintf (tfn, sizeof (tfn), "%s/%s%s", sysvars [SV_BDJ4_DREL_DATA],
          SYSVARS_PY_VERS_FN, BDJ4_CONFIG_EXT);
      if (fileopFileExists (tfn)) {
        *buff = '\0';
        fh = fileopOpen (tfn, "r");
        if (fh != NULL) {
          (void) ! fgets (buff, sizeof (buff), fh);
          mdextfclose (fh);
          fclose (fh);
        }
        stringTrim (buff);
        strlcpy (sysvars [SV_PYTHON_VERSION], buff, SV_MAX_SZ);
      }
    }

    if (! *sysvars [SV_PYTHON_DOT_VERSION]) {
      char        *data;
      char        *p;

      data = osRunProgram (sysvars [SV_PATH_PYTHON], "--version", NULL);

      // Python 3.9.2

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
          }
        } /* found the first '.' */
      }
      mdfree (data);

      snprintf (tfn, sizeof (tfn), "%s/%s%s", sysvars [SV_BDJ4_DREL_DATA],
          SYSVARS_PY_DOT_VERS_FN, BDJ4_CONFIG_EXT);
      fh = fileopOpen (tfn, "w");
      if (fh != NULL) {
        fprintf (fh, "%s\n", sysvars [SV_PYTHON_DOT_VERSION]);
        mdextfclose (fh);
        fclose (fh);
      }
    }

    if (! *sysvars [SV_PYTHON_VERSION]) {
      size_t  j = 0;

      for (size_t i = 0; i < strlen (sysvars [SV_PYTHON_DOT_VERSION]); ++i) {
        if (sysvars [SV_PYTHON_DOT_VERSION][i] != '.') {
          sysvars [SV_PYTHON_VERSION][j++] = sysvars [SV_PYTHON_DOT_VERSION][i];
        }
      }
      sysvars [SV_PYTHON_VERSION][j] = '\0';

      snprintf (tfn, sizeof (tfn), "%s/%s%s", sysvars [SV_BDJ4_DREL_DATA],
          SYSVARS_PY_VERS_FN, BDJ4_CONFIG_EXT);
      fh = fileopOpen (tfn, "w");
      if (fh != NULL) {
        fprintf (fh, "%s\n", sysvars [SV_PYTHON_VERSION]);
        mdextfclose (fh);
        fclose (fh);
      }
    }
  } /* if python was found */
}

void
sysvarsCheckMutagen (void)
{
  char  buff [SV_MAX_SZ];

  /* use the system installed version if present */
  /* for windows, be sure to replace the .exe with the actual script */
  if (! isWindows () && *sysvars [SV_PATH_MUTAGEN] != '\0') {
    return;
  }

  /* On windows the mutagen-inspect script seems to be no longer installed */
  /* along with the package, and the converted executable pops up command */
  /* windows. */
  // $HOME/.local/bin/mutagen-inspect  (user-install: linux, macos, msys2)
  // $HOME/Library/Python/<pydotver>/bin/mutagen-inspect (macos)
  // %USERPROFILE%/AppData/Local/Programs/Python/Python<pyver>/Scripts/mutagen-inspect-script.py
  // %USERPROFILE%/AppData/Roaming/Python/Python<pyver>/Scripts/mutagen-inspect.exe

  *buff = '\0';
  if (isLinux ()) {
    snprintf (buff, sizeof (buff),
        "%s/.local/bin/%s", sysvars [SV_HOME], "mutagen-inspect");
  }
  if (isWindows ()) {
    char    thome [MAXPATHLEN];

    /* check the msys location first -- use the HOME env var */
    osGetEnv ("HOME", thome, sizeof (thome));
    if (*thome) {
      snprintf (buff, sizeof (buff),
          "%s/.local/bin/%s", thome, "mutagen-inspect");
    }
    /* use the windows script if it is available */
    if (! fileopFileExists (buff)) {
      snprintf (buff, sizeof (buff),
          "%s/AppData/Local/Programs/Python/Python%s/Scripts/%s",
          sysvars [SV_HOME], sysvars [SV_PYTHON_VERSION], "mutagen-inspect-script.py");
    }
  }
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff),
        "%s/Library/Python/%s/bin/%s",
        sysvars [SV_HOME], sysvars [SV_PYTHON_DOT_VERSION], "mutagen-inspect");
  }

  /* otherwise use our copy */
  if (! fileopFileExists (buff)) {
    snprintf (buff, sizeof (buff),
        "%s/scripts/%s",
        sysvars [SV_BDJ4_DIR_MAIN], "mutagen-inspect");
  }

  if (fileopFileExists (buff)) {
    strlcpy (sysvars [SV_PATH_MUTAGEN], buff, SV_MAX_SZ);
  } else {
    if (isWindows ()) {
      /* for msys2 testing */
      snprintf (buff, sizeof (buff),
          "%s/.local/bin/%s", sysvars [SV_HOME], "mutagen-inspect");
      if (fileopFileExists (buff)) {
        strlcpy (sysvars [SV_PATH_MUTAGEN], buff, SV_MAX_SZ);
      }
    }
  }
  pathNormalizePath (sysvars [SV_PATH_MUTAGEN], SV_MAX_SZ);
}

char *
sysvarsGetStr (sysvarkey_t idx)
{
  if (idx >= SV_MAX) {
    return NULL;
  }

  return sysvars [idx];
}

int64_t
sysvarsGetNum (sysvarlkey_t idx)
{
  if (idx >= SVL_MAX) {
    return -1;
  }

  return lsysvars [idx];
}


void
sysvarsSetStr (sysvarkey_t idx, const char *value)
{
  if (idx >= SV_MAX) {
    return;
  }

  strlcpy (sysvars [idx], value, SV_MAX_SZ);
}

void
sysvarsSetNum (sysvarlkey_t idx, int64_t value)
{
  if (idx >= SVL_MAX) {
    return;
  }

  lsysvars [idx] = value;
}

bool
isMacOS (void)
{
  return (bool) lsysvars [SVL_IS_MACOS];
}

bool
isWindows (void)
{
  return (bool) lsysvars [SVL_IS_WINDOWS];
}

bool
isLinux (void)
{
  return (bool) lsysvars [SVL_IS_LINUX];
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

sysversinfo_t *
sysvarsParseVersionFile (const char *path)
{
  sysversinfo_t *versinfo;

  versinfo = mdmalloc (sizeof (sysversinfo_t));
  versinfo->data = NULL;
  versinfo->version = "";
  versinfo->build = "";
  versinfo->builddate = "";
  versinfo->releaselevel = "";
  versinfo->dev = "";

  if (fileopFileExists (path)) {
    char    *tokptr;
    char    *tokptrb;
    char    *tp;
    char    *vnm;
    char    *p;

    versinfo->data = filedataReadAll (path, NULL);
    tp = strtok_r (versinfo->data, "\r\n", &tokptr);
    while (tp != NULL) {
      vnm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (vnm != NULL && p != NULL && strcmp (vnm, "VERSION") == 0) {
        versinfo->version = p;
      }
      if (vnm != NULL && p != NULL && strcmp (vnm, "BUILD") == 0) {
        versinfo->build = p;
      }
      if (vnm != NULL && p != NULL && strcmp (vnm, "BUILDDATE") == 0) {
        versinfo->builddate = p;
      }
      if (vnm != NULL && p != NULL && strcmp (vnm, "RELEASELEVEL") == 0) {
        versinfo->releaselevel = p;
      }
      if (vnm != NULL && p != NULL && strcmp (vnm, "DEVELOPMENT") == 0) {
        versinfo->dev = p;
      }
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
  }

  return versinfo;
}

void
sysvarsParseVersionFileFree (sysversinfo_t *versinfo)
{
  if (versinfo != NULL) {
    dataFree (versinfo->data);
    mdfree (versinfo);
  }
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

  fh = fileopOpen (fn, "r");
  if (fh == NULL) {
    return rc;
  }

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
  mdextfclose (fh);
  fclose (fh);

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
  mdfree (tptr);
}

static void
svGetSystemFont (void)
{
  char    *tptr;

  tptr = osGetSystemFont (sysvars [SV_PATH_GSETTINGS]);
  if (tptr != NULL) {
    strlcpy (sysvars [SV_FONT_DEFAULT], tptr, SV_MAX_SZ);
    mdfree (tptr);
  }
}
