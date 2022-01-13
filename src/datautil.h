#ifndef INC_DATAUTIL_H
#define INC_DATAUTIL_H

typedef enum {
  DATAUTIL_MP_NONE        = 0x0000,
  DATAUTIL_MP_HOSTNAME    = 0x0001,
  DATAUTIL_MP_USEIDX      = 0x0002,
  DATAUTIL_MP_TMPPREFIX   = 0x0004,   // defaults to data/
  DATAUTIL_MP_EXECPREFIX  = 0x0008,   // defaults to data/
} datautil_mp_t;

char *        datautilMakePath (char *buff, size_t buffsz, const char *subpath,
                  const char *base, const char *extension, int flags);

#endif /* INC_DATAUTIL_H */
