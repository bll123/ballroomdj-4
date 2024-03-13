/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PATHBLD_H
#define INC_PATHBLD_H

typedef enum {
  PATHBLD_MP_NONE         = 0,
  /* relative paths */
  PATHBLD_IS_RELATIVE     = 0x10000000,
  PATHBLD_MP_DREL_DATA    = ((1 << 0) | PATHBLD_IS_RELATIVE),
  PATHBLD_MP_DREL_HTTP    = ((1 << 1) | PATHBLD_IS_RELATIVE),
  PATHBLD_MP_DREL_TMP     = ((1 << 2) | PATHBLD_IS_RELATIVE),
  PATHBLD_MP_DREL_IMG     = ((1 << 3) | PATHBLD_IS_RELATIVE),
  /* used for testing only */
  PATHBLD_MP_DREL_TEST_TMPL = ((1 << 4) | PATHBLD_IS_RELATIVE),
  /* absolute paths based in main */
  PATHBLD_IS_ABSOLUTE     = 0x20000000,
  PATHBLD_MP_DIR_MAIN     = ((1 << 0) | PATHBLD_IS_ABSOLUTE),
  PATHBLD_MP_DIR_EXEC     = ((1 << 1) | PATHBLD_IS_ABSOLUTE),
  PATHBLD_MP_DIR_IMG      = ((1 << 2) | PATHBLD_IS_ABSOLUTE),
  PATHBLD_MP_DIR_LOCALE   = ((1 << 3) | PATHBLD_IS_ABSOLUTE),
  PATHBLD_MP_DIR_TEMPLATE = ((1 << 4) | PATHBLD_IS_ABSOLUTE),
  PATHBLD_MP_DIR_INST     = ((1 << 5) | PATHBLD_IS_ABSOLUTE),
  /* data directory (absolute) */
  PATHBLD_MP_DIR_DATATOP  = ((1 << 6) | PATHBLD_IS_ABSOLUTE),
  /* other paths */
  PATHBLD_IS_OTHER        = 0x40000000,
  PATHBLD_MP_DIR_CACHE    = ((1 << 0) | PATHBLD_IS_OTHER),
  PATHBLD_MP_DIR_CONFIG   = ((1 << 1) | PATHBLD_IS_OTHER),
  PATHBLD_MP_DIR_LOCK     = ((1 << 2) | PATHBLD_IS_OTHER),
  /* flags */
  PATHBLD_MP_DSTAMP       = (1 << 16),
  PATHBLD_MP_HOSTNAME     = (1 << 17),   // adds hostname to path
  PATHBLD_MP_USEIDX       = (1 << 18),   // adds profile dir to path
  PATHBLD_LOCK_FFN        = (1 << 19),   // used by lock.c
  /* for testing locks */
  LOCK_TEST_OTHER_PID     = (1 << 20),   // other process id
  LOCK_TEST_SKIP_SELF     = (1 << 21),   // for 'already' test
} pathbld_mp_t;

#define PATH_PROFILES   "profiles"

char *        pathbldMakePath (char *buff, size_t buffsz,
                  const char *base, const char *extension, int flags);

#endif /* INC_PATHBLD_H */
