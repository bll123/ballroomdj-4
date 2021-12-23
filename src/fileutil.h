#ifndef INC_FILEUTIL_H
#define INC_FILEUTIL_H

typedef struct {
  char    *dirname;
  char    *filename;
  char    *basename;
  char    *extension;
  size_t  dlen;
  size_t  flen;
  size_t  blen;
  size_t  elen;
} fileinfo_t;

fileinfo_t *    fileInfo (char *path);
void            fileInfoFree (fileinfo_t *);

void          makeBackups (char *, int);
int           fileExists (char *);
int           fileCopy (char *, char *);
int           fileMove (char *, char *);

#endif /* INC_FILEUTIL_H */
