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

#define FILE_MP_NONE          0x0
#define FILE_MP_HOSTNAME      0x1
#define FILE_MP_USEIDX        0x2

fileinfo_t *    fileInfo (char *path);
void            fileInfoFree (fileinfo_t *);

void          makeBackups (char *, int);
int           fileExists (char *);
int           fileCopy (char *, char *);
int           fileMove (char *, char *);
int           fileDelete (char *fname);
int           fileMakeDir (char *dirname);
char *        fileReadAll (char *);
void          fileConvWinPath (char *from, char *to, size_t maxlen);
char *        fileMakePath (char *buff, size_t buffsz, const char *subpath,
                  const char *base, const char *extension, int flags);

#endif /* INC_FILEUTIL_H */
