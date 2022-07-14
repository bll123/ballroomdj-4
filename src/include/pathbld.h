#ifndef INC_PATHBLD_H
#define INC_PATHBLD_H

typedef enum {
  PATHBLD_MP_NONE         = 0x00000000,
  PATHBLD_MP_DATA         = 0x00000001,
  PATHBLD_MP_EXECDIR      = 0x00000002,
  PATHBLD_MP_HOSTNAME     = 0x00000004,
  PATHBLD_MP_HTTPDIR      = 0x00000008,
  PATHBLD_MP_IMGDIR       = 0x00000010,
  PATHBLD_MP_LOCALEDIR    = 0x00000020,
  PATHBLD_MP_MAINDIR      = 0x00000040,
  PATHBLD_MP_MUSICDIR     = 0x00000080,
  PATHBLD_MP_TEMPLATEDIR  = 0x00000100,
  PATHBLD_MP_TMPDIR       = 0x00000200,
  PATHBLD_MP_USEIDX       = 0x00000400,
  PATHBLD_MP_INSTDIR      = 0x00000800,
  PATHBLD_MP_DSTAMP       = 0x00001000,
  PATHBLD_MP_DATATOPDIR   = 0x00002000,
  PATHBLD_MP_CONFIGDIR    = 0x00004000,
  PATHBLD_LOCK_FFN        = 0x00008000,   // used by lock.c
} pathbld_mp_t;

#define PATH_PROFILES   "profiles"

char *        pathbldMakePath (char *buff, size_t buffsz,
                  const char *base, const char *extension, int flags);

#endif /* INC_PATHBLD_H */
