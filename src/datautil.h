#ifndef INC_DATAUTIL_H
#define INC_DATAUTIL_H

#define DATAUTIL_MP_NONE          0b00000000
#define DATAUTIL_MP_HOSTNAME      0b00000001
#define DATAUTIL_MP_USEIDX        0b00000010

char *        datautilMakePath (char *buff, size_t buffsz, const char *subpath,
                  const char *base, const char *extension, int flags);

#endif /* INC_DATAUTIL_H */
