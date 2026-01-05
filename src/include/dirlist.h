/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  DIRLIST_FILES = 0x01,
  DIRLIST_DIRS  = 0x02,
  /* If DIRLIST_LINKS is specified, links will be treated separately */
  /* and added to the file list.  Only for recursive. */
  DIRLIST_LINKS = 0x04,
};

/* dirlist.c */
NODISCARD slist_t * dirlistBasicDirList (const char *dir, const char *extension);
NODISCARD slist_t * dirlistRecursiveDirList (const char *dir, int flags);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

