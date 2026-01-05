/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  /* some of the AF_ flags are only internal to audiotag.c */
  AF_REWRITE_NONE     = 0,
  AF_REWRITE_MB       = (1 << 0),
  AF_REWRITE_DURATION = (1 << 1),
  AF_REWRITE_VARIOUS  = (1 << 2),
  AF_FORCE_WRITE_BDJ  = (1 << 3),
};

/* these are file types that have audio tag support */
/* other audio file types may still be playable by vlc */
enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MK,      // matroksa
  AFILE_TYPE_MP3,
  AFILE_TYPE_MP4,
  AFILE_TYPE_OGG,     // ogg container, codec unknown
  AFILE_TYPE_OPUS,
  AFILE_TYPE_VORBIS,
  AFILE_TYPE_RIFF,    // .wav
  AFILE_TYPE_ASF,     // advanced system format, .wma
  AFILE_TYPE_MAX,
};

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

