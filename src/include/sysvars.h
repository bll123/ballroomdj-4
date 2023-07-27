/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H

#include <stdint.h>

typedef enum {
  SV_BDJ4_BUILD,
  SV_BDJ4_BUILDDATE,
  SV_BDJ4_DIR_DATATOP,  // path to the directory above data/, tmp/, img/, http/
  SV_BDJ4_DIR_EXEC,     // main + /bin
  SV_BDJ4_DIR_IMG,      // main + /img
  SV_BDJ4_DIR_INST,     // main + /install
  SV_BDJ4_DIR_LOCALE,   // main + /locale
  SV_BDJ4_DIR_MAIN,     // path to the main directory above bin/, etc.
  SV_BDJ4_DIR_SCRIPT,   // main + /scripts
  SV_BDJ4_DIR_TEMPLATE, // main + /templates
  SV_BDJ4_DREL_DATA,    // data
  SV_BDJ4_DREL_HTTP,    // http
  SV_BDJ4_DREL_IMG,     // img - some images are in main/img and some here.
  SV_BDJ4_DREL_TMP,     // tmp
  SV_BDJ4_DEVELOPMENT,
  SV_BDJ4_RELEASELEVEL,
  SV_BDJ4_VERSION,
  SV_CA_FILE,
  SV_DIR_CACHE,         // {DIR_CACHE_BASE}/BDJ4
  SV_DIR_CACHE_BASE,    // from XDG_CACHE_HOME or $HOME/.cache
                        // windows: %TEMP% or %USERPROFILE%/AppData/Local/Temp
  SV_DIR_CONFIG,        // {DIR_CONFIG_BASE}/BDJ4
  SV_DIR_CONFIG_BASE,   // from XDG_CONFIG_HOME or $HOME/.config
                        // windows: %USERPROFILE%/AppData/Roaming
  SV_FILE_ALTCOUNT,
  SV_FILE_ALT_INST_PATH,
  SV_FILE_INST_PATH,
  SV_FONT_DEFAULT,
  SV_HOME,
  SV_HOST_DOWNLOAD,
  SV_HOST_FORUM,
  SV_HOSTNAME,
  SV_HOST_SUPPORTMSG,
  SV_HOST_TICKET,
  SV_HOST_WEB,
  SV_HOST_WIKI,
  SV_LOCALE,
  SV_LOCALE_ISO_639_2,
  SV_LOCALE_ORIG,
  SV_LOCALE_RADIX,
  SV_LOCALE_SHORT,
  SV_LOCALE_SYSTEM,
  SV_OSARCH,
  SV_OSBUILD,
  SV_OSDISP,
  SV_OS_EXEC_EXT,
  SV_OSNAME,
  SV_OSVERS,
  SV_PATH_FFMPEG,
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
  SV_URI_DOWNLOAD,
  SV_URI_FORUM,
  SV_URI_REGISTER,
  SV_URI_SUPPORTMSG,
  SV_URI_TICKET,
  SV_URI_WIKI,
  SV_USER,
  SV_USER_AGENT,
  SV_USER_MUNGE,
  SV_VLC_VERSION,
  SV_WEB_VERSION_FILE,
  SV_MAX
} sysvarkey_t;

typedef enum {
  SVL_DATAPATH,
  SVL_BASEPORT,
  SVL_BDJIDX,
  SVL_INITIAL_PORT,
  SVL_IS_LINUX,
  SVL_IS_MACOS,
  SVL_IS_WINDOWS,
  SVL_LOCALE_SET,
  SVL_LOCALE_SYS_SET,
  SVL_NUM_PROC,
  SVL_OSBITS,
  SVL_MAX
} sysvarlkey_t;

enum {
  SYSVARS_DATAPATH_ALT,
  SYSVARS_DATAPATH_LOCAL,
  SYSVARS_DATAPATH_NORM,
  SYSVARS_DATAPATH_UNKNOWN,
  SYSVARS_LOCALE_NOT_SET,
  SYSVARS_LOCALE_SET,
};

typedef struct {
  char    *data;
  char    *version;
  char    *build;
  char    *builddate;
  char    *releaselevel;
  char    *dev;
} sysversinfo_t;

void    sysvarsInit (const char *argv0);
void    sysvarsCheckPaths (const char *otherpaths);
void    sysvarsGetPythonVersion (void);
void    sysvarsCheckMutagen (void);
char    * sysvarsGetStr (sysvarkey_t idx);
int64_t sysvarsGetNum (sysvarlkey_t idx);
void    sysvarsSetStr (sysvarkey_t, const char *value);
void    sysvarsSetNum (sysvarlkey_t, int64_t);
const char * sysvarsDesc (sysvarkey_t idx);
const char * sysvarslDesc (sysvarlkey_t idx);
bool    isMacOS (void);
bool    isWindows (void);
bool    isLinux (void);
sysversinfo_t *sysvarsParseVersionFile (const char *path);
void    sysvarsParseVersionFileFree (sysversinfo_t *versinfo);

#endif /* INC_SYSVARS_H */
