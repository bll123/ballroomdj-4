/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PATHINFO_H
#define INC_PATHINFO_H

typedef struct {
  const char  *dirname;
  const char  *filename;
  const char  *basename;
  const char  *extension;
  size_t      dlen;
  size_t      flen;
  size_t      blen;
  size_t      elen;
} pathinfo_t;

pathinfo_t *  pathInfo (const char *path);
void          pathInfoFree (pathinfo_t *);
bool          pathInfoExtCheck (pathinfo_t *, const char *extension);
void          pathInfoGetDir (pathinfo_t *pi, char *buff, size_t sz);
void          pathInfoGetExt (pathinfo_t *pi, char *buff, size_t sz);

#endif /* INC_PATHINFO_H */
