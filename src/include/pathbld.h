/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PATHBLD_H
#define INC_PATHBLD_H

typedef enum {
  PATHBLD_MP_NONE         = 0x00000000,
  /* relative paths */
  PATHBLD_IS_RELATIVE     = 0x10000000,
  PATHBLD_MP_DATA         = 0x10000001,
  PATHBLD_MP_HTTPDIR      = 0x10000002,
  PATHBLD_MP_TMPDIR       = 0x10000004,
  /* absolute paths based in main */
  PATHBLD_IS_ABSOLUTE     = 0x20000000,
  PATHBLD_MP_MAINDIR      = 0x20000008,
  PATHBLD_MP_EXECDIR      = 0x20000010,
  PATHBLD_MP_IMGDIR       = 0x20000020,
  PATHBLD_MP_LOCALEDIR    = 0x20000040,
  PATHBLD_MP_TEMPLATEDIR  = 0x20000080,
  PATHBLD_MP_INSTDIR      = 0x20000100,
  /* data directory (absolute) */
  PATHBLD_MP_DATATOPDIR   = 0x20000200,
  /* other paths */
  PATHBLD_IS_OTHER        = 0x40000000,
  PATHBLD_MP_CONFIGDIR    = 0x40000400,
  /* flags */
  PATHBLD_MP_DSTAMP       = 0x00000800,
  PATHBLD_MP_HOSTNAME     = 0x00001000,   // adds hostname to path
  PATHBLD_MP_USEIDX       = 0x00002000,   // adds profile dir to path
  PATHBLD_LOCK_FFN        = 0x00004000,   // used by lock.c
  /* for testing locks */
  LOCK_TEST_OTHER_PID     = 0x00008000,   // other process id
  LOCK_TEST_SKIP_SELF     = 0x00010000,   // for 'already' test
} pathbld_mp_t;

#define PATH_PROFILES   "profiles"

char *        pathbldMakePath (char *buff, size_t buffsz,
                  const char *base, const char *extension, int flags);

#endif /* INC_PATHBLD_H */
