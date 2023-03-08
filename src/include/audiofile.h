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

#endif /* INC_AUDIOFILE_H */
