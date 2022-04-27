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
  SV_FORUM_HOST,
  SV_FORUM_URI,
  SV_HOME,
  SV_HOSTNAME,
  SV_LOCALE,
  SV_LOCALE_SHORT,
  SV_LOCALE_SYSTEM,
  SV_MOBMQ_HOST,
  SV_MOBMQ_URI,
  SV_MOBMQ_POST_URI,
  SV_OSBUILD,
  SV_OSDISP,
  SV_OS_EXEC_EXT,
  SV_OSNAME,
  SV_OSVERS,
  SV_PATH_GETCONF,
  SV_PATH_PYTHON,
  SV_PATH_PYTHON_PIP,
  SV_PATH_VLC,
  SV_PATH_XDGUSERDIR,
  SV_PYTHON_DOT_VERSION,  // 3.10.2 => 3.10
  SV_PYTHON_MUTAGEN,
  SV_PYTHON_VERSION,      // 3.10.2 => 310
  SV_SHLIB_EXT,
  SV_REGISTER_URI,
  SV_SUPPORTMSG_HOST,
  SV_SUPPORTMSG_URI,
  SV_TICKET_HOST,
  SV_TICKET_URI,
  SV_TEMP_A,
  SV_TEMP_B,
  SV_USER,
  SV_USER_AGENT,
  SV_USER_MUNGE,
  SV_WEB_HOST,
  SV_WEB_VERSION_FILE,
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

#define SV_TMP_FILE "tmpsysvars"

void    sysvarsInit (const char *);
void    sysvarsCheckPaths (void);
char    * sysvarsGetStr (sysvarkey_t idx);
ssize_t sysvarsGetNum (sysvarlkey_t idx);
void    sysvarsSetStr (sysvarkey_t, const char *value);
void    sysvarsSetNum (sysvarlkey_t, ssize_t);
const char * sysvarsDesc (sysvarkey_t idx);
const char * sysvarslDesc (sysvarlkey_t idx);
bool    isMacOS (void);
bool    isWindows (void);
bool    isLinux (void);

#endif /* INC_SYSVARS_H */
