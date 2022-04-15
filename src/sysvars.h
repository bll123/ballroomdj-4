#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H


typedef enum {
  SV_BDJ4_BUILD,
  SV_BDJ4_BUILDDATE,
  SV_BDJ4DATADIR,     // data
  SV_BDJ4DATATOPDIR,  // path to the directory above data/ and tmp/ and http/
  SV_BDJ4EXECDIR,     // main + /bin
  SV_BDJ4HTTPDIR,     // http
  SV_BDJ4IMGDIR,      // main + /img
  SV_BDJ4LOCALEDIR,   // main + /locale
  SV_BDJ4MAINDIR,     // path to the main directory above bin/, etc.
  SV_BDJ4_RELEASELEVEL,
  SV_BDJ4TEMPLATEDIR, // main + /templates
  SV_BDJ4TMPDIR,      // tmp
  SV_BDJ4_VERSION,
  SV_CA_FILE,
  SV_GETCONF_PATH,
  SV_HOME,
  SV_HOSTNAME,
  SV_LOCALE,
  SV_LOCALE_SHORT,
  SV_LOCALE_SYSTEM,
  SV_MOBMQ_HOST,
  SV_MOBMQ_URL,
  SV_OSBUILD,
  SV_OSDISP,
  SV_OSNAME,
  SV_OSVERS,
  SV_PYTHON_DOT_VERSION,  // 3.10.2 => 3.10
  SV_PYTHON_MUTAGEN,
  SV_PYTHON_PATH,
  SV_PYTHON_PIP_PATH,
  SV_PYTHON_VERSION,      // 3.10.2 => 310
  SV_SHLIB_EXT,
  SV_WEB_HOST,
  SV_MAX
} sysvarkey_t;

typedef enum {
  SVL_BDJIDX,
  SVL_BASEPORT,
  SVL_OSBITS,
  SVL_NUM_PROC,
  SVL_LOCALE_SET,
  SVL_MAX
} sysvarlkey_t;

#define SV_TMP_FILE "tmp/sysvars.txt"

void    sysvarsInit (const char *);
char    * sysvarsGetStr (sysvarkey_t idx);
ssize_t sysvarsGetNum (sysvarlkey_t idx);
void    sysvarsSetStr (sysvarkey_t, const char *value);
void    sysvarsSetNum (sysvarlkey_t, ssize_t);
bool    isMacOS (void);
bool    isWindows (void);
bool    isLinux (void);

#endif /* INC_SYSVARS_H */
