#ifndef INC_AUDIOFILE_H
#define INC_AUDIOFILE_H

enum {
  /* some of the AF_ flags are only internal to audiotag.c */
  AF_REWRITE_NONE     = 0x0000,
  AF_REWRITE_MB       = 0x0001,
  AF_REWRITE_DURATION = 0x0002,
  AF_REWRITE_VARIOUS  = 0x0004,
  AF_FORCE_WRITE_BDJ  = 0x0008,
};

/* these are file types that have audio tag support */
/* other audio file types may still be playable by vlc */
enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MP3,
  AFILE_TYPE_MP4,
  AFILE_TYPE_OGG,     // ogg container, codec unknown
  AFILE_TYPE_OPUS,
  AFILE_TYPE_VORBIS,
  AFILE_TYPE_WMA,
  AFILE_TYPE_MAX,
};

#endif /* INC_AUDIOFILE_H */
