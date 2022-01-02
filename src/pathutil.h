#ifndef INC_PATHUTIL_H
#define INC_PATHUTIL_H

typedef struct {
  char    *dirname;
  char    *filename;
  char    *basename;
  char    *extension;
  size_t  dlen;
  size_t  flen;
  size_t  blen;
  size_t  elen;
} pathinfo_t;

pathinfo_t *  pathInfo (char *path);
void          pathInfoFree (pathinfo_t *);
void          pathToWinPath (char *from, char *to, size_t maxlen);
void          pathNormPath (char *buff, size_t len);
void          pathRealPath (char *from, char *to);

#endif /* INC_PATHUTIL_H */
