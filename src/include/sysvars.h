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
  SV_BDJ4INSTDIR,     // main + /install
  SV_BDJ4LOCALEDIR,   // main + /locale
  SV_BDJ4MAINDIR,     // path to the main directory above bin/, etc.
  SV_BDJ4_RELEASELEVEL,
  SV_BDJ4TEMPLATEDIR, // main + /templates
  SV_BDJ4TMPDIR,      // tmp
  SV_BDJ4_VERSION,
  SV_CA_FILE,
  SV_CONFIG_DIR,      // .config/BDJ4 or AppData/Roaming/BDJ4
  SV_FONT_DEFAULT,
  SV_HOME,
  SV_HOST_FORUM,
  SV_HOST_MOBMQ,
  SV_HOSTNAME,
  SV_HOST_SUPPORTMSG,
  SV_HOST_TICKET,
  SV_HOST_WEB,
  SV_HOST_WIKI,
  SV_LOCALE,
  SV_LOCALE_SHORT,
  SV_LOCALE_SYSTEM,
  SV_OSBUILD,
  SV_OSDISP,
  SV_OS_EXEC_EXT,
  SV_OSNAME,
  SV_OSVERS,
  SV_PATH_GSETTINGS,
  SV_PATH_PYTHON,
  SV_PATH_PYTHON_PIP,
  SV_PATH_VLC,
  SV_PATH_XDGUSERDIR,
  SV_PYTHON_DOT_VERSION,  // 3.10.2 => 3.10
  SV_PYTHON_MUTAGEN,
  SV_PYTHON_VERSION,      // 3.10.2 => 310
  SV_SHLIB_EXT,
  SV_TEMP_A,
  SV_TEMP_B,
  SV_THEME_DEFAULT,
  SV_URI_FORUM,
  SV_URI_MOBMQ,
  SV_URI_MOBMQ_POST,
  SV_URI_REGISTER,
  SV_URI_SUPPORTMSG,
  SV_URI_TICKET,
  SV_URI_WIKI,
  SV_USER,
  SV_USER_AGENT,
  SV_USER_MUNGE,
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

void    sysvarsInit (const char *argv0);
void    sysvarsCheckPaths (const char *otherpaths);
void    sysvarsCheckPython (void);
void    sysvarsCheckMutagen (void);
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
