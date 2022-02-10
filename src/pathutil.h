#ifndef INC_PATHUTIL_H
#define INC_PATHUTIL_H

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
void          pathToWinPath (char *to, const char *from, size_t maxlen);
void          pathNormPath (char *buff, size_t len);
void          pathRealPath (char *to, const char *from);

#endif /* INC_PATHUTIL_H */
