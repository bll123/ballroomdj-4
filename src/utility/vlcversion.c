/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#if _lib_libvlc3_new || _lib_libvlc4_new
# include <vlc/vlc.h>
# include <vlc/libvlc_version.h>
#endif

int
main (int argc, char *argv [])
{
#if _lib_libvlc3_new || _lib_libvlc4_new
  fprintf (stderr, "%s\n", libvlc_get_version());
#else
  fprintf (stderr, "NG\n");
#endif /* libvlc_new() */
}
