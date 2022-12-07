/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PATHBLD_H
#define INC_PATHBLD_H

typedef enum {
  PATHBLD_MP_NONE         = 0x00000000,
  /* relative paths */
  PATHBLD_IS_RELATIVE     = 0x10000001,
  PATHBLD_MP_DREL_DATA    = 0x10000002,
  PATHBLD_MP_DREL_HTTP    = 0x10000004,
  PATHBLD_MP_DREL_TMP     = 0x10000008,
  PATHBLD_MP_DREL_IMG     = 0x10000010,
  /* absolute paths based in main */
  PATHBLD_IS_ABSOLUTE     = 0x20000001,
  PATHBLD_MP_DIR_MAIN     = 0x20000002,
  PATHBLD_MP_DIR_EXEC     = 0x20000004,
  PATHBLD_MP_DIR_IMG      = 0x20000008,
  PATHBLD_MP_DIR_LOCALE   = 0x20000010,
  PATHBLD_MP_DIR_TEMPLATE = 0x20000020,
  PATHBLD_MP_DIR_INST     = 0x20000040,
  /* data directory (absolute) */
  PATHBLD_MP_DIR_DATATOP  = 0x20000200,
  /* other paths */
  PATHBLD_IS_OTHER        = 0x40000000,
  PATHBLD_MP_DIR_CONFIG   = 0x40000001,
  /* flags */
  PATHBLD_MP_DSTAMP       = 0x00010000,
  PATHBLD_MP_HOSTNAME     = 0x00020000,   // adds hostname to path
  PATHBLD_MP_USEIDX       = 0x00040000,   // adds profile dir to path
  PATHBLD_LOCK_FFN        = 0x00080000,   // used by lock.c
  /* for testing locks */
  LOCK_TEST_OTHER_PID     = 0x00100000,   // other process id
  LOCK_TEST_SKIP_SELF     = 0x00200000,   // for 'already' test
} pathbld_mp_t;

#define PATH_PROFILES   "profiles"

char *        pathbldMakePath (char *buff, size_t buffsz,
                  const char *base, const char *extension, int flags);

#endif /* INC_PATHBLD_H */
