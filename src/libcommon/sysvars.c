/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>

#if __has_include (<sys/resource.h>)
# include <sys/resource.h>
#endif
#if __has_include (<sys/utsname.h>)
# include <sys/utsname.h>
#endif

#if __has_include (<windows.h>)
# define NOGDI 1
# include <windows.h>
#endif
#if __has_include (<windows.h>) && __has_include (<intrin.h>)
/* has a failure on macos if not checked on windows */
# include <intrin.h>
#endif

#include "bdj4.h"
#include "sysvars.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "mdebug.h"
#include "osdirutil.h"
#include "osenv.h"
#include "osnetutils.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathdisp.h"
#include "pathutil.h"

#include "log.h"

typedef struct {
  char    *data;
  char    *dist;
} sysdistinfo_t;

/* for debugging */
static const char *sysvarsdesc [SV_MAX] = {
  [SV_BDJ4_BUILD] = "BDJ4_BUILD",
  [SV_BDJ4_BUILDDATE] = "BDJ4_BUILDDATE",
  [SV_BDJ4_DEVELOPMENT] = "BDJ4_DEVELOPMENT",
  [SV_BDJ4_DIR_DATATOP] = "BDJ4_DIR_DATATOP",
  [SV_BDJ4_DIR_EXEC] = "BDJ4_DIR_EXEC",
  [SV_BDJ4_DIR_IMG] = "BDJ4_DIR_IMG",
  [SV_BDJ4_DIR_INST] = "BDJ4_DIR_INST",
  [SV_BDJ4_DIR_LOCALE] = "BDJ4_DIR_LOCALE",
  [SV_BDJ4_DIR_MAIN] = "BDJ4_DIR_MAIN",
  [SV_BDJ4_DIR_SCRIPT] = "BDJ4_DIR_SCRIPT",
  [SV_BDJ4_DIR_TEMPLATE] = "BDJ4_DIR_TEMPLATE",
  [SV_BDJ4_DREL_DATA] = "BDJ4_DREL_DATA",
  [SV_BDJ4_DREL_HTTP] = "BDJ4_DREL_HTTP",
  [SV_BDJ4_DREL_IMG] = "BDJ4_DREL_IMG",
  [SV_BDJ4_DREL_TMP] = "BDJ4_DREL_TMP",
  [SV_BDJ4_RELEASELEVEL] = "BDJ4_RELEASELEVEL",
  [SV_BDJ4_VERSION] = "BDJ4_VERSION",
  [SV_CA_FILE] = "CA_FILE",
  [SV_CA_FILE_LOCAL] = "CA_FILE_LOCAL",
  [SV_DIR_CACHE] = "DIR_CACHE",
  [SV_DIR_CACHE_BASE] = "DIR_CACHE_BASE",
  [SV_DIR_CONFIG] = "DIR_CONFIG",
  [SV_DIR_CONFIG_BASE] = "DIR_CONFIG_BASE",
  [SV_FILE_ALTCOUNT] = "FILE_ALTCOUNT",
  [SV_FILE_INST_PATH] = "FILE_INST_PATH",
  [SV_FONT_DEFAULT] = "FONT_DEFAULT",
  [SV_HOME] = "HOME",
  [SV_HOSTNAME] = "HOSTNAME",
  [SV_HOST_REGISTER] = "HOST_REGISTER",
  [SV_LOCALE] = "LOCALE",
  [SV_LOCALE_ORIG] = "LOCALE_ORIG",
  [SV_LOCALE_ORIG_SHORT] = "LOCALE_ORIG_SHORT",
  [SV_LOCALE_RADIX] = "LOCALE_RADIX",
  [SV_LOCALE_SHORT] = "LOCALE_SHORT",
  [SV_LOCALE_SYSTEM] = "LOCALE_SYSTEM",
  [SV_LOCALE_639_2] = "LOCALE_639_2",
  [SV_OS_ARCH] = "OS_ARCH",
  [SV_OS_ARCH_TAG] = "OS_ARCH_TAG",
  [SV_OS_BUILD] = "OS_BUILD",
  [SV_OS_DISP] = "OS_DISP",
  [SV_OS_DIST_TAG] = "OS_DIST_TAG",
  [SV_OS_EXEC_EXT] = "OS_EXEC_EXT",
  [SV_OS_NAME] = "OS_NAME",
  [SV_OS_PKG_SYS] = "OS_PKG_SYS",
  [SV_OS_PLATFORM] = "OS_PLATFORM",
  [SV_OS_VERS] = "OS_VERS",
  [SV_PATH_ACRCLOUD] = "PATH_ACRCLOUD",
  [SV_PATH_CRONTAB] = "PATH_CRONTAB",
  [SV_PATH_FFMPEG] = "PATH_FFMPEG",
  [SV_PATH_FPCALC] = "PATH_FPCALC",
  [SV_PATH_GSETTINGS] = "PATH_GSETTINGS",
  [SV_PATH_ICONDIR] = "PATH_ICONDIR",
  [SV_PATH_URI_OPEN] = "PATH_URI_OPEN",
  [SV_PATH_VLC] = "PATH_VLC",
  [SV_PATH_VLC_LIB] = "PATH_VLC_LIB",
  [SV_PATH_XDGUSERDIR] = "PATH_XDGUSERDIR",
  [SV_SHLIB_EXT] = "SHLIB_EXT",
  [SV_THEME_DEFAULT] = "THEME_DEFAULT",
  [SV_URI_REGISTER] = "URI_REGISTER",
  [SV_USER] = "USER",
  [SV_USER_MUNGE] = "USER_MUNGE",
};

static_assert (sizeof (sysvarsdesc) / sizeof (const char *) == SV_MAX,
    "missing sysvars sv_ entry");

static const char *sysvarsldesc [SVL_MAX] = {
  [SVL_ALTIDX] = "ALTIDX",
  [SVL_BASEPORT] = "BASEPORT",
  [SVL_DATAPATH] = "DATAPATH",
  [SVL_INITIAL_PORT] = "INITIAL_PORT",
  [SVL_IS_LINUX] = "IS_LINUX",
  [SVL_IS_MACOS] = "IS_MACOS",
  [SVL_IS_MSYS] = "IS_MSYS",
  [SVL_IS_READONLY] = "IS_READONLY",
  [SVL_IS_VM] = "IS_VM",
  [SVL_IS_WINDOWS] = "IS_WINDOWS",
  [SVL_LOCALE_TEXT_DIR] = "LOCALE_TEXT_DIR",
  [SVL_LOCALE_SET] = "LOCALE_SET",
  [SVL_LOCALE_SYS_SET] = "LOCALE_SYS_SET",
  [SVL_NUM_PROC] = "NUM_PROC",
  [SVL_OS_BITS] = "OS_BITS",
  [SVL_PROFILE_IDX] = "PROFILE_IDX",
  [SVL_USER_ID] = "USER_ID",
  [SVL_VLC_VERSION] = "VLC_VERSION",
};

static_assert (sizeof (sysvarsldesc) / sizeof (const char *) == SVL_MAX,
    "missing sysvars svl_ entry");

enum {
  SV_MAX_SZ = 1024,
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
  /* macos, these may be older than the supplied curl-ca-bundle.crt */
  "/opt/local/etc/openssl/cert.pem",
  "/opt/local/share/curl/curl-ca-bundle.crt",
  "/opt/homebrew/etc/ca-certificates/cert.pem",
};
enum {
  CACERT_FILE_COUNT = (sizeof (cacertFiles) / sizeof (char *))
};

static void enable_core_dump (void);
static void checkForFile (char *path, int idx, ...);
static bool svGetLinuxOSInfo (char *fn);
static void svGetLinuxDefaultTheme (void);
static void svGetSystemFont (void);
static sysdistinfo_t *sysvarsParseDistFile (const char *path);
static void sysvarsParseDistFileFree (sysdistinfo_t *distinfo);
static void sysvarsBuildVLCLibPath (char *tbuff, char *lbuff, size_t sz, const char *libnm);

void
sysvarsInit (const char *argv0, int flags)
{
  char          tcwd [SV_MAX_SZ];
  char          tbuff [SV_MAX_SZ];
  char          altpath [SV_MAX_SZ];
  char          buff [SV_MAX_SZ];
  char          *p;
  char          *end;
  size_t        dlen;
  bool          alternatepath = false;
  sysversinfo_t *versinfo;
  sysdistinfo_t *distinfo;
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

  osGetCurrentDir (tcwd, sizeof (tcwd));
  pathRealPath (tcwd, SV_MAX_SZ);
  pathNormalizePath (tcwd, SV_MAX_SZ);

  sysvarsSetStr (SV_OS_ARCH, "");
  sysvarsSetStr (SV_OS_ARCH_TAG, "");
  sysvarsSetStr (SV_OS_BUILD, "");
  sysvarsSetStr (SV_OS_DISP, "");
  sysvarsSetStr (SV_OS_DIST_TAG, "");
  sysvarsSetStr (SV_OS_EXEC_EXT, "");
  sysvarsSetStr (SV_OS_NAME, "");
  sysvarsSetStr (SV_OS_PKG_SYS, "");
  sysvarsSetStr (SV_OS_PLATFORM, "");
  sysvarsSetStr (SV_OS_VERS, "");
  lsysvars [SVL_IS_MSYS] = false;
  lsysvars [SVL_IS_LINUX] = false;
  lsysvars [SVL_IS_WINDOWS] = false;
  lsysvars [SVL_IS_VM] = false;
  lsysvars [SVL_IS_READONLY] = false;

#if _lib_uname
  uname (&ubuf);
  sysvarsSetStr (SV_OS_NAME, ubuf.sysname);
  sysvarsSetStr (SV_OS_DISP, ubuf.sysname);
  sysvarsSetStr (SV_OS_VERS, ubuf.version);
  sysvarsSetStr (SV_OS_ARCH, ubuf.machine);
#endif
#if _lib_RtlGetVersion
  memset (&osvi, 0, sizeof (RTL_OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOEXW);
  RtlGetVersion (&osvi);

  snprintf (sysvars [SV_OS_VERS], SV_MAX_SZ, "%ld.%ld",
      osvi.dwMajorVersion, osvi.dwMinorVersion);
  snprintf (sysvars [SV_OS_BUILD], SV_MAX_SZ, "%ld", osvi.dwBuildNumber);
  if (osvi.dwBuildNumber >= 22000) {
    /* this is the official way to determine windows 11 at this time */
    sysvarsSetStr (SV_OS_VERS, "11.0");
  }
#endif
#if _lib_GetNativeSystemInfo
  GetNativeSystemInfo (&winsysinfo);
  /* dwNumberOfProcessors may not reflect the number of system processors */
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
    sysvarsSetStr (SV_OS_ARCH, "intel");
    lsysvars [SVL_OS_BITS] = 32;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
    sysvarsSetStr (SV_OS_ARCH, "arm");
    lsysvars [SVL_OS_BITS] = 32;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
    sysvarsSetStr (SV_OS_ARCH, "arm");
    lsysvars [SVL_OS_BITS] = 64;
  }
  if (winsysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
    sysvarsSetStr (SV_OS_ARCH, "amd64");
    lsysvars [SVL_OS_BITS] = 64;
  }
#endif

/* is a windows machine */
#if _lib_GetNativeSystemInfo || _lib_RtlGetVersion
  {
    sysvarsSetStr (SV_OS_EXEC_EXT, ".exe");
    sysvarsSetStr (SV_OS_NAME, "windows");
    p = sysvars [SV_OS_DISP];
    end = sysvars [SV_OS_DISP] + SV_MAX_SZ;
    p = stpecpy (p, end, "Windows ");
    if (strcmp (sysvars [SV_OS_VERS], "5.0") == 0) {
      p = stpecpy (p, end, "2000");
    } else if (strcmp (sysvars [SV_OS_VERS], "5.1") == 0) {
      p = stpecpy (p, end, "XP");
    } else if (strcmp (sysvars [SV_OS_VERS], "5.2") == 0) {
      p = stpecpy (p, end, "XP Pro");
    } else if (strcmp (sysvars [SV_OS_VERS], "6.0") == 0) {
      p = stpecpy (p, end, "Vista");
    } else if (strcmp (sysvars [SV_OS_VERS], "6.1") == 0) {
      p = stpecpy (p, end, "7");
    } else if (strcmp (sysvars [SV_OS_VERS], "6.2") == 0) {
      p = stpecpy (p, end, "8.0");
    } else if (strcmp (sysvars [SV_OS_VERS], "6.3") == 0) {
      p = stpecpy (p, end, "8.1");
    } else {
      p = stpecpy (p, end, sysvars [SV_OS_VERS]);
    }
    p = stpecpy (p, end, " ");
    p = stpecpy (p, end, sysvars [SV_OS_BUILD]);
  }
#endif

  stringAsciiToLower (sysvars [SV_OS_NAME]);
  if (sizeof (void *) == 8) {
    lsysvars [SVL_OS_BITS] = 64;
  } else {
    lsysvars [SVL_OS_BITS] = 32;
  }

  if (strcmp (sysvars [SV_OS_NAME], "darwin") == 0) {
    const char  *tdir = NULL;
    bool        found = false;

    lsysvars [SVL_IS_MACOS] = true;
    sysvarsSetStr (SV_OS_PLATFORM, "macos");
    /* arch: arm64, 86_64 */
    /* be sure to include the leading - */
    if (strcmp (sysvars [SV_OS_ARCH], "x86_64") == 0) {
      sysvarsSetStr (SV_OS_ARCH_TAG, "-intel");
    }
    if (strcmp (sysvars [SV_OS_ARCH], "arm64") == 0) {
      sysvarsSetStr (SV_OS_ARCH_TAG, "-applesilicon");
    }

    /* macos has multiple packaging systems */
    /* this is not necessarily the best method to check. */
    /* the user could have multiple packaging systems installed */
    tdir = "/opt/local/bin";
    if (fileopIsDirectory (tdir)) {
      sysvarsSetStr (SV_OS_PKG_SYS, "-macports");
      found = true;
    }
    tdir = "/opt/homebrew/bin";
    if (! found && fileopIsDirectory (tdir)) {
      sysvarsSetStr (SV_OS_PKG_SYS, "-homebrew");
      found = true;
    }
    tdir = "/opt/pkg/bin";
    if (! found && fileopIsDirectory (tdir)) {
      sysvarsSetStr (SV_OS_PKG_SYS, "-pkgsrc");
      found = true;
    }
  }
  if (strcmp (sysvars [SV_OS_NAME], "linux") == 0) {
    lsysvars [SVL_IS_LINUX] = true;
    sysvarsSetStr (SV_OS_PLATFORM, "linux");
  }
  if (strcmp (sysvars [SV_OS_NAME], "windows") == 0) {
    lsysvars [SVL_IS_WINDOWS] = true;
    sysvarsSetStr (SV_OS_PLATFORM, "win64");
  }

  osGetEnv ("MSYSTEM", tbuff, SV_MAX_SZ);
  if (*tbuff) {
    lsysvars [SVL_IS_MSYS] = true;
  }

  getHostname (sysvars [SV_HOSTNAME], SV_MAX_SZ);

  if (isWindows ()) {
    /* the home directory is left as the long name */
    /* any process using it must do the conversion for windows */
    osGetEnv ("USERPROFILE", sysvars [SV_HOME], SV_MAX_SZ);
    osGetEnv ("USERNAME", sysvars [SV_USER], SV_MAX_SZ);
  } else {
    osGetEnv ("HOME", sysvars [SV_HOME], SV_MAX_SZ);
    osGetEnv ("USER", sysvars [SV_USER], SV_MAX_SZ);
  }
  dlen = strlen (sysvars [SV_USER]);
  sysvarsSetStr (SV_USER_MUNGE, sysvars [SV_USER]);
  for (size_t i = 0; i < dlen; ++i) {
    if (sysvars [SV_USER_MUNGE][i] == ' ') {
      sysvars [SV_USER_MUNGE][i] = '-';
    }
  }
  lsysvars [SVL_USER_ID] = SVC_USER_ID_NONE;
#if _lib_getuid
  lsysvars [SVL_USER_ID] = getuid ();
#endif

  stpecpy (tbuff, tbuff + SV_MAX_SZ, argv0);
  stpecpy (buff, buff + SV_MAX_SZ, argv0);

  pathNormalizePath (buff, SV_MAX_SZ);
  /* handle relative pathnames */
  if ((strlen (buff) > 2 && *(buff + 1) == ':' && *(buff + 2) != '/') ||
     (*buff != '/' && strlen (buff) > 1 && *(buff + 1) != ':')) {
    p = tbuff;
    end = tbuff + sizeof (tbuff);
    p = stpecpy (p, end, tcwd);
    p = stpecpy (p, end, "/");
    p = stpecpy (p, end, buff);
  }

  /* save this path so that it can be used to check for a data dir */
  stpecpy (altpath, altpath + SV_MAX_SZ, tbuff);
  pathStripPath (altpath, sizeof (altpath));
  pathNormalizePath (altpath, sizeof (altpath));

  /* this gives us the real path to the executable */
  stpecpy (buff, buff + sizeof (buff), tbuff);
  pathRealPath (buff, sizeof (buff));
  pathNormalizePath (buff, sizeof (buff));

  if (strcmp (altpath, buff) != 0) {
    alternatepath = true;
  }

  /* strip off the filename */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4_DIR_EXEC] = '\0';
  if (p != NULL) {
    *p = '\0';
    sysvarsSetStr (SV_BDJ4_DIR_EXEC, buff);
  }

  /* strip off '/bin' */
  p = strrchr (buff, '/');
  *sysvars [SV_BDJ4_DIR_MAIN] = '\0';
  if (p != NULL) {
    *p = '\0';
    sysvarsSetStr (SV_BDJ4_DIR_MAIN, buff);
  }

  /* the readonly file lives in the top level of the main dir */
  snprintf (tbuff, sizeof (tbuff), "%s%s", READONLY_FN, BDJ4_CONFIG_EXT);
  if (fileopFileExists (tbuff)) {
    lsysvars [SVL_IS_READONLY] = true;
  }

  if (fileopIsDirectory ("data") && lsysvars [SVL_IS_READONLY] == false) {
    /* if there is a data directory in the current working directory */
    /* and there is no 'readonly.txt' file */
    /* a change of directories is contra-indicated. */

    sysvarsSetStr (SV_BDJ4_DIR_DATATOP, tcwd);

    lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_LOCAL;
  } else {
    bool found = false;

    /* check for a data directory in the original run-path */
    if (alternatepath) {
      char    *bp;
      char    *bend = buff + SV_MAX_SZ;

      if (isMacOS ()) {
        /* altpath is something like: */
        /* /Users/bll/Applications/BDJ4alt.app/Contents/MacOS/bin/bdj4g */
        p = strstr (altpath, "/Contents");
        if (p != NULL) {
          *p = '\0';

          p = strrchr (altpath, '/');
          if (p != NULL) {
            const char *tp;

            *p = '\0';
            ++p;
            tp = p;

            p = strstr (tp, MACOS_APP_EXT);
            if (p != NULL) {
              *p = '\0';
            }

            bp = buff;
            bp = stpecpy (bp, bend, sysvars [SV_HOME]);
            bp = stpecpy (bp, bend, "/Library/Application Support/");
            bp = stpecpy (bp, bend, tp);
            if (fileopIsDirectory (buff)) {
              sysvarsSetStr (SV_BDJ4_DIR_DATATOP, buff);
              found = true;
            }
          }
        }
      } else {
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

        snprintf (tbuff, sizeof (tbuff), "%s/%s%s",
            altpath, READONLY_FN, BDJ4_CONFIG_EXT);
        tlen = strlen (altpath);
        stpecpy (altpath + tlen, altpath + sizeof (altpath), "/data");
        if (fileopFileExists (tbuff)) {
          lsysvars [SVL_IS_READONLY] = true;
        }

        if (fileopIsDirectory (altpath) && lsysvars [SVL_IS_READONLY] == false) {
          /* remove the /data suffix */
          altpath [tlen] = '\0';
          sysvarsSetStr (SV_BDJ4_DIR_DATATOP, altpath);
          found = true;
          lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_ALT;
        }
      }
    }

    if (! found) {
      lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_NORM;

      snprintf (tbuff, sizeof (tbuff), "%s/%s%s",
          sysvars [SV_BDJ4_DIR_MAIN], READONLY_FN, BDJ4_CONFIG_EXT);
      if (fileopFileExists (tbuff)) {
        lsysvars [SVL_IS_READONLY] = true;
      }

      if (lsysvars [SVL_IS_READONLY] == true) {
        lsysvars [SVL_DATAPATH] = SYSVARS_DATAPATH_UNKNOWN;
        *sysvars [SV_BDJ4_DIR_DATATOP]= '\0';
      } else {
        if (isMacOS ()) {
          char        tmp [BDJ4_PATH_MAX];
          ssize_t     offset;
          const char  *tp;
          char        *bend = buff + SV_MAX_SZ;
          char        *bp;

          /* extract the name of the app from the main-dir */
          stpecpy (tmp, tmp + sizeof (tmp), sysvars [SV_BDJ4_DIR_MAIN]);
          tp = BDJ4_NAME;
          if (strstr (tmp, MACOS_APP_PREFIX) != NULL) {
            offset = strlen (tmp) -
                strlen (MACOS_APP_PREFIX) -
                strlen (MACOS_APP_EXT);
            if (offset >= 0) {
              tmp [offset] = '\0';
            }
            tp = strrchr (tmp, '/');
            if (tp != NULL) {
              ++tp;
            }
          }

          bp = buff;
          bp = stpecpy (bp, bend, sysvars [SV_HOME]);
          bp = stpecpy (bp, bend, "/Library/Application Support/");
          bp = stpecpy (bp, bend, tp);
          sysvarsSetStr (SV_BDJ4_DIR_DATATOP, buff);
        } else {
          sysvarsSetStr (SV_BDJ4_DIR_DATATOP, sysvars [SV_BDJ4_DIR_MAIN]);
        }
      }
    }
  }

  /* on mac os, the data directory is separated */
  /* full path is also needed so that symlinked bdj4 directories will work */

  sysvarsSetStr (SV_BDJ4_DREL_DATA, "data");

  p = sysvars [SV_BDJ4_DIR_IMG];
  end = sysvars [SV_BDJ4_DIR_IMG] + SV_MAX_SZ;
  /* this must be the full path for macos and for alternate installations */
  p = stpecpy (p, end, sysvars [SV_BDJ4_DIR_MAIN]);
  p = stpecpy (p, end, "/img");

  p = sysvars [SV_BDJ4_DIR_INST];
  end = sysvars [SV_BDJ4_DIR_INST] + SV_MAX_SZ;
  p = stpecpy (p, end, sysvars [SV_BDJ4_DIR_MAIN]);
  p = stpecpy (p, end, "/install");

  p = sysvars [SV_BDJ4_DIR_LOCALE];
  end = sysvars [SV_BDJ4_DIR_LOCALE] + SV_MAX_SZ;
  p = stpecpy (p, end, sysvars [SV_BDJ4_DIR_MAIN]);
  p = stpecpy (p, end, "/locale");

  p = sysvars [SV_BDJ4_DIR_TEMPLATE];
  end = sysvars [SV_BDJ4_DIR_TEMPLATE] + SV_MAX_SZ;
  p = stpecpy (p, end, sysvars [SV_BDJ4_DIR_MAIN]);
  p = stpecpy (p, end, "/templates");

  p = sysvars [SV_BDJ4_DIR_SCRIPT];
  end = sysvars [SV_BDJ4_DIR_SCRIPT] + SV_MAX_SZ;
  p = stpecpy (p, end, sysvars [SV_BDJ4_DIR_MAIN]);
  p = stpecpy (p, end, "/scripts");

  sysvarsSetStr (SV_BDJ4_DREL_HTTP, "http");
  sysvarsSetStr (SV_BDJ4_DREL_TMP, "tmp");
  sysvarsSetStr (SV_BDJ4_DREL_IMG, "img");

  sysvarsSetStr (SV_SHLIB_EXT, SHLIB_EXT);

  sysvarsSetStr (SV_HOST_REGISTER, "https://ballroomdj.org");
  sysvarsSetStr (SV_URI_REGISTER, "/bdj4register.php");

  sysvarsSetStr (SV_CA_FILE_LOCAL, "http/ca.crt");
  for (size_t i = 0; i < CACERT_FILE_COUNT; ++i) {
    if (fileopFileExists (cacertFiles [i])) {
      sysvarsSetStr (SV_CA_FILE, cacertFiles [i]);
      break;
    }
    if (*cacertFiles [i] != '/') {
      snprintf (tbuff, sizeof (tbuff), "%s/%s",
          sysvars [SV_BDJ4_DIR_MAIN], cacertFiles [i]);
      if (fileopFileExists (tbuff)) {
        sysvarsSetStr (SV_CA_FILE, tbuff);
        break;
      }
    }
  }

  /* want to avoid having setlocale() linked in to sysvars. */
  /* so these defaults are all wrong */
  /* the locale is reset by localeinit */
  /* localeinit will also convert the windows names to something normal */
  sysvarsSetStr (SV_LOCALE_SYSTEM, "en_GB.UTF-8");
  sysvarsSetStr (SV_LOCALE_ORIG, "en_GB");
  sysvarsSetStr (SV_LOCALE_ORIG_SHORT, "en");
  sysvarsSetStr (SV_LOCALE, "en_GB");
  sysvarsSetStr (SV_LOCALE_SHORT, "en");
  sysvarsSetStr (SV_LOCALE_RADIX, ".");
  sysvarsSetStr (SV_LOCALE_639_2, "eng");

  lsysvars [SVL_LOCALE_SET] = SYSVARS_LOCALE_NOT_SET;
  lsysvars [SVL_LOCALE_SYS_SET] = SYSVARS_LOCALE_NOT_SET;

  sysvarsSetStr (SV_BDJ4_VERSION, "unknown");
  snprintf (buff, sizeof (buff), "%s/VERSION.txt", sysvars [SV_BDJ4_DIR_MAIN]);
  versinfo = sysvarsParseVersionFile (buff);
  sysvarsSetStr (SV_BDJ4_VERSION, versinfo->version);
  sysvarsSetStr (SV_BDJ4_BUILD, versinfo->build);
  sysvarsSetStr (SV_BDJ4_BUILDDATE, versinfo->builddate);
  sysvarsSetStr (SV_BDJ4_RELEASELEVEL, versinfo->releaselevel);
  sysvarsSetStr (SV_BDJ4_DEVELOPMENT, versinfo->dev);
  sysvarsParseVersionFileFree (versinfo);

  snprintf (buff, sizeof (buff), "%s/DIST.txt", sysvars [SV_BDJ4_DIR_MAIN]);
  distinfo = sysvarsParseDistFile (buff);
  sysvarsSetStr (SV_OS_DIST_TAG, distinfo->dist);
  sysvarsParseDistFileFree (distinfo);

  if (isWindows ()) {
    char    tbuff [BDJ4_PATH_MAX];

    snprintf (tbuff, sizeof (tbuff),
        "%s/AppData/Roaming", sysvars [SV_HOME]);
    pathRealPath (tbuff, sizeof (tbuff));
    strcpy (sysvars [SV_DIR_CONFIG_BASE], tbuff);
  } else {
    osGetEnv ("XDG_CONFIG_HOME", sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ);
    if (! *sysvars [SV_DIR_CONFIG_BASE]) {
      snprintf (sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ, "%s/.config",
          sysvars [SV_HOME]);
    }
  }
  pathNormalizePath (sysvars [SV_DIR_CONFIG_BASE], SV_MAX_SZ);

  snprintf (tbuff, sizeof (tbuff), "%s/%s", sysvars [SV_DIR_CONFIG_BASE], BDJ4_NAME);
  sysvarsSetStr (SV_DIR_CONFIG, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%s/%s%s%s", sysvars [SV_DIR_CONFIG],
      ALT_COUNT_FN, sysvars [SV_BDJ4_DEVELOPMENT], BDJ4_CONFIG_EXT);
  sysvarsSetStr (SV_FILE_ALTCOUNT, tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s%s", sysvars [SV_DIR_CONFIG],
      INST_PATH_FN, sysvars [SV_BDJ4_DEVELOPMENT], BDJ4_CONFIG_EXT);
  sysvarsSetStr (SV_FILE_INST_PATH, tbuff);

  if (isWindows ()) {
    char    tbuff [BDJ4_PATH_MAX];

    snprintf (tbuff, sizeof (tbuff),
        "%s/AppData/Local/Temp", sysvars [SV_HOME]);
    pathRealPath (tbuff, sizeof (tbuff));
    strcpy (sysvars [SV_DIR_CACHE_BASE], tbuff);
  } else {
    osGetEnv ("XDG_CACHE_HOME", sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ);
    if (! *sysvars [SV_DIR_CACHE_BASE]) {
      snprintf (sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ, "%s/.cache",
          sysvars [SV_HOME]);
    }
  }
  pathNormalizePath (sysvars [SV_DIR_CACHE_BASE], SV_MAX_SZ);

  snprintf (tbuff, sizeof (tbuff), "%s/%s", sysvars [SV_DIR_CACHE_BASE], BDJ4_NAME);
  sysvarsSetStr (SV_DIR_CACHE, tbuff);

  /* the installer creates this file to save the original system locale */
  snprintf (buff, sizeof (buff), "%s/%s%s",
      sysvars [SV_BDJ4_DREL_DATA], LOCALE_ORIG_FN, BDJ4_CONFIG_EXT);
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
      sysvarsSetStr (SV_LOCALE_SYSTEM, tbuff);
      /* do not mark locale-set, only locale-sys-set */
      lsysvars [SVL_LOCALE_SYS_SET] = SYSVARS_LOCALE_SET;
      /* localeInit() will set locale-orig and the other variables */
    }
  }

  snprintf (buff, sizeof (buff), "%s/%s%s",
      sysvars [SV_BDJ4_DREL_DATA], LOCALE_FN, BDJ4_CONFIG_EXT);
  sysvarsLoadLocale (buff);

  sysvarsCheckPaths (NULL);

  if (flags == SYSVARS_FLAG_ALL &&
      strcmp (sysvars [SV_OS_NAME], "darwin") == 0) {
    char  *data;
    char  *tdata;

    sysvarsSetStr (SV_OS_DISP, "MacOS");
    data = osRunProgram (sysvars [SV_TEMP_A], "-ProductVersion", NULL);
    stringTrim (data);
    p = sysvars [SV_OS_VERS];
    end = p + SV_MAX_SZ;
    p = stpecpy (p, end, data);
    dataFree (data);

    tdata = osRunProgram (sysvars [SV_TEMP_A], "-ProductVersionExtra", NULL);
    if (tdata != NULL && *tdata == '(') {
      size_t    len;

      stringTrim (tdata);
      len = strlen (tdata);
      *(tdata + len - 1) = '\0';
      p = stpecpy (p, end, "-");
      p = stpecpy (p, end, tdata + 1);
    }
    dataFree (tdata);

    sysvarsSetStr (SV_OS_BUILD, "");
    data = osRunProgram (sysvars [SV_TEMP_A], "-BuildVersion", NULL);
    stringTrim (data);
    sysvarsSetStr (SV_OS_BUILD, data);
    dataFree (data);

    data = sysvars [SV_OS_VERS];
    if (data != NULL) {
      p = sysvars [SV_OS_DISP] + strlen (sysvars [SV_OS_DISP]);
      end = p + SV_MAX_SZ;
      p = stpecpy (p, end, " ");
      p = stpecpy (p, end, data);
    }
  }

  /* set the default theme */
  sysvarsSetStr (SV_THEME_DEFAULT, "Adwaita:dark");

  if (strcmp (sysvars [SV_OS_NAME], "linux") == 0) {
    static char *fna = "/etc/lsb-release";
    static char *fnb = "/etc/os-release";

    if (! svGetLinuxOSInfo (fna)) {
      svGetLinuxOSInfo (fnb);
    }

    if (flags == SYSVARS_FLAG_ALL && *sysvars [SV_PATH_GSETTINGS]) {
      /* override the default theme with what the user has set */
      svGetLinuxDefaultTheme ();
    }
  }

  /* the SVL_IS_VM flag is only used for the test suite */
  /* this is not set for mac-os (how to do is unknown) */
  if (strcmp (sysvars [SV_OS_NAME], "linux") == 0) {
    FILE        *fh;
    char        tbuff [BDJ4_PATH_MAX];
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
  if (strcmp (sysvars [SV_OS_NAME], "windows") == 0) {
#if _lib___cpuid
    int     cpuinfo [4];

    __cpuid (cpuinfo, 1);
    if (cpuinfo [2] >> 31 & 1) {
      lsysvars [SVL_IS_VM] = true;
    }
#endif
  }

  if (flags == SYSVARS_FLAG_ALL) {
    svGetSystemFont ();
  }

  lsysvars [SVL_ALTIDX] = 0;
  lsysvars [SVL_PROFILE_IDX] = 0;
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
      lsysvars [SVL_BASEPORT] = strtoul (tbuff, NULL, 10);
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
  char    *end;
  char    *tsep;
  char    *tokstr;
  char    tbuff [BDJ4_PATH_MAX];
  char    tpath [16384];

  sysvarsSetStr (SV_PATH_ACRCLOUD, "");
  /* crontab is used on macos during installation */
  sysvarsSetStr (SV_PATH_CRONTAB, "");
  sysvarsSetStr (SV_PATH_FFMPEG, "");
  sysvarsSetStr (SV_PATH_FPCALC, "");
  /* gsettings is used on linux to get the current theme */
  sysvarsSetStr (SV_PATH_GSETTINGS, "");
  sysvarsSetStr (SV_PATH_XDGUSERDIR, "");
  sysvarsSetStr (SV_PATH_ICONDIR, "");
  sysvarsSetStr (SV_TEMP_A, "");

  tsep = ":";
  if (isWindows ()) {
    tsep = ";";
  }
  osGetEnv ("PATH", tpath, sizeof (tpath));
  stringTrimChar (tpath, *tsep);
  p = tpath + strlen (tpath);
  end = tpath + sizeof (tpath);
  p = stpecpy (p, end, tsep);
  if (otherpaths != NULL && *otherpaths) {
    p = stpecpy (p, end, otherpaths);
  }

  p = strtok_r (tpath, tsep, &tokstr);
  while (p != NULL) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), p);
    pathNormalizePath (tbuff, sizeof (tbuff));
    stringTrimChar (tbuff, '/');

    if (*sysvars [SV_PATH_ACRCLOUD] == '\0') {
      checkForFile (tbuff, SV_PATH_ACRCLOUD, "acr_extr", NULL);
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

  if (isWindows ()) {
    snprintf (sysvars [SV_PATH_ICONDIR], SV_MAX_SZ,
        "%s/plocal/share/icons/hicolor/scalable/apps", sysvars [SV_BDJ4_DIR_MAIN]);
  } else {
    snprintf (sysvars [SV_PATH_ICONDIR], SV_MAX_SZ,
        "%s/.local/share/icons/hicolor/scalable/apps", sysvars [SV_HOME]);
  }

  lsysvars [SVL_VLC_VERSION] = 3;     // unknown at this point
  sysvarsSetStr (SV_PATH_VLC, "");
  sysvarsSetStr (SV_PATH_VLC_LIB, "");
  sysvarsCheckVLCPath ();
}

void
sysvarsCheckVLCPath (void)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        lbuff [BDJ4_PATH_MAX];
  char        cbuff [BDJ4_PATH_MAX];
  const char  *libnm = NULL;

  *tbuff = '\0';
  *lbuff = '\0';
  if (isWindows ()) {
    stpecpy (tbuff, tbuff + sizeof (tbuff),
        "C:/Program Files/VideoLAN/VLC");
    libnm = "libvlc.dll";
    sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
    libnm = "libvlccore.dll";
    sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
  }
  if (isMacOS ()) {
    /* determine if this is vlc-3 or vlc-4 */
    /* vlc-3 has the library in ../Contents/MacOS/lib */
    /* vlc-4 has the library in ../Contents/Frameworks */

    stpecpy (tbuff, tbuff + sizeof (tbuff),
        "/Applications/VLC.app/Contents/MacOS/lib");
    libnm = "libvlc.dylib";
    sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
    libnm = "libvlccore.dylib";
    sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
    if (! fileopFileExists (lbuff)) {
      stpecpy (tbuff, tbuff + sizeof (tbuff),
          "/Applications/VLC.app/Contents/Frameworks");
      libnm = "libvlc.dylib";
      sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
      libnm = "libvlccore.dylib";
      sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
      if (fileopFileExists (lbuff)) {
        lsysvars [SVL_VLC_VERSION] = 4;
        /* only on macos is the vlc version known */
      }
    }
  }
  if (isLinux ()) {
    /* this will probably fail to work with vlc-4, but that */
    /* is not a serious issue on linux */
    /* the usual bunch */
    snprintf (tbuff, sizeof (tbuff), "/usr/lib/%s-linux-gnu",
        sysvarsGetStr (SV_OS_ARCH));
    libnm = "libvlc.so.5";
    sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
    libnm = "libvlccore.so.9";
    sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
    if (! fileopFileExists (lbuff)) {
      /* opensuse et.al. */
      stpecpy (tbuff, tbuff + sizeof (tbuff), "/usr/lib64");
      libnm = "libvlc.so.5";
      sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
      libnm = "libvlccore.so.9";
      sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
      if (! fileopFileExists (lbuff)) {
        /* alpine linux */
        stpecpy (tbuff, tbuff + sizeof (tbuff), "/usr/lib");
        libnm = "libvlc.so.5";
        sysvarsBuildVLCLibPath (tbuff, lbuff, sizeof (lbuff), libnm);
        libnm = "libvlccore.so.9";
        sysvarsBuildVLCLibPath (tbuff, cbuff, sizeof (cbuff), libnm);
      }
    }
  }

  if (fileopFileExists (lbuff) && fileopFileExists (cbuff)) {
    sysvarsSetStr (SV_PATH_VLC, tbuff);
    sysvarsSetStr (SV_PATH_VLC_LIB, lbuff);
  }
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

  stpecpy (sysvars [idx], sysvars [idx] + SV_MAX_SZ, value);
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
  return sysvarsdesc [idx];
}

const char *
sysvarslDesc (sysvarlkey_t idx)
{
  if (idx >= SVL_MAX) {
    return "";
  }
  return sysvarsldesc [idx];
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
  if (versinfo == NULL) {
    return;
  }

  dataFree (versinfo->data);
  mdfree (versinfo);
}

void
sysvarsLoadLocale (const char *path)
{
  char    tbuff [BDJ4_PATH_MAX];

  if (fileopFileExists (path)) {
    FILE    *fh;

    *tbuff = '\0';
    fh = fileopOpen (path, "r");
    if (fh != NULL) {
      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      mdextfclose (fh);
      fclose (fh);
    }
    stringTrim (tbuff);
    if (*tbuff) {
      if (strcmp (tbuff, sysvars [SV_LOCALE_SYSTEM]) != 0) {
        char    buff [40];

        sysvarsSetStr (SV_LOCALE, tbuff);
        snprintf (buff, sizeof (buff), "%-.2s", tbuff);
        sysvarsSetStr (SV_LOCALE_SHORT, buff);
        lsysvars [SVL_LOCALE_SET] = SYSVARS_LOCALE_SET;
      }
    }
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
  char      buff [BDJ4_PATH_MAX];
  char      *fn;
  bool      found = false;

  va_start (valist, idx);

  while (! found && (fn = va_arg (valist, char *)) != NULL) {
    snprintf (buff, sizeof (buff), "%s/%s%s", path, fn, sysvars [SV_OS_EXEC_EXT]);
    if (fileopFileExists (buff)) {
      sysvarsSetStr (idx, buff);
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
  char        tbuff [BDJ4_PATH_MAX];
  char        buff [BDJ4_PATH_MAX];
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
      stpecpy (buff, buff + sizeof (buff), tbuff + strlen (prettytag) + 1);
      stringTrim (buff);
      stringTrimChar (buff, '"');
      sysvarsSetStr (SV_OS_DISP, buff);
      haveprettyname = true;
      rc = true;
    }
    if (! haveprettyname &&
        strncmp (tbuff, desctag, strlen (desctag)) == 0) {
      stpecpy (buff, buff + sizeof (buff), tbuff + strlen (desctag) + 1);
      stringTrim (buff);
      stringTrimChar (buff, '"');
      sysvarsSetStr (SV_OS_DISP, buff);
      haveprettyname = true;
      rc = true;
    }
    if (! havevers &&
        strncmp (tbuff, reltag, strlen (reltag)) == 0) {
      stpecpy (buff, buff + sizeof (buff), tbuff + strlen (reltag));
      stringTrim (buff);
      sysvarsSetStr (SV_OS_VERS, buff);
      rc = true;
    }
    if (! havevers &&
        strncmp (tbuff, verstag, strlen (verstag)) == 0) {
      stpecpy (buff, buff + sizeof (buff), tbuff + strlen (verstag) + 1);
      stringTrim (buff);
      stringTrimChar (buff, '"');
      sysvarsSetStr (SV_OS_VERS, buff);
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

  /* gtk cannot seem to retrieve the properties from settings */
  /* (or i don't know how) */
  /* so run the gsettings program to get the info */

  tptr = osRunProgram (sysvars [SV_PATH_GSETTINGS], "get",
      "org.gnome.desktop.interface", "gtk-theme", NULL);
  if (tptr != NULL) {
    /* gsettings puts quotes around the data */
    stringTrim (tptr);
    stringTrimChar (tptr, '\'');
    if (strlen (tptr) > 0) {
      sysvarsSetStr (SV_THEME_DEFAULT, tptr + 1);
    }
  }
  mdfree (tptr);
}

static void
svGetSystemFont (void)
{
  char    tptr [1024];

  osGetSystemFont (sysvars [SV_PATH_GSETTINGS], tptr, sizeof (tptr));
  if (*tptr) {
    sysvarsSetStr (SV_FONT_DEFAULT, tptr);
  }
}

sysdistinfo_t *
sysvarsParseDistFile (const char *path)
{
  sysdistinfo_t *distinfo;

  distinfo = mdmalloc (sizeof (sysdistinfo_t));
  distinfo->data = NULL;
  distinfo->dist = mdstrdup ("");

  if (fileopFileExists (path)) {
    char    *tokptr;
    char    *tokptrb;
    char    *tp;
    char    *vnm;
    char    *p;

    distinfo->data = filedataReadAll (path, NULL);
    tp = strtok_r (distinfo->data, "\r\n", &tokptr);
    while (tp != NULL) {
      vnm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (vnm != NULL && p != NULL && strcmp (vnm, "DIST_TAG") == 0) {
        distinfo->dist = mdstrdup (p);
      }
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
  }

  return distinfo;
}

static void
sysvarsParseDistFileFree (sysdistinfo_t *distinfo)
{
  if (distinfo == NULL) {
    return;
  }

  dataFree (distinfo->data);
  mdfree (distinfo);
}

static void
sysvarsBuildVLCLibPath (char *tbuff, char *lbuff, size_t sz, const char *libnm)
{
  char    *p;

  p = stpecpy (lbuff, lbuff + sz, tbuff);
  p = stpecpy (p, lbuff + sz, "/");
  p = stpecpy (p, lbuff + sz, libnm);
}

