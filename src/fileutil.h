#ifndef INC_FILEUTIL_H
#define INC_FILEUTIL_H

#if _hdr_windows
# include <windows.h>
#endif

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

typedef union {
#if _typ_HANDLE
  HANDLE  handle;
#endif
  int     fd;
} filehandle_t;

#define FILE_MP_NONE          0x0
#define FILE_MP_HOSTNAME      0x1
#define FILE_MP_USEIDX        0x2

fileinfo_t *    fileInfo (char *path);
void            fileInfoFree (fileinfo_t *);

void          makeBackups (char *, int);
int           fileExists (char *);
int           fileCopy (char *, char *);
int           fileMove (char *, char *);
int           fileDelete (const char *fname);
int           fileOpenShared (const char *fname, int truncflag,
                  filehandle_t *fileHandle);
size_t        fileWriteShared (filehandle_t *fileHandle, char *data, size_t len);
void          fileCloseShared (filehandle_t *fileHandle);
int           fileMakeDir (char *dirname);
char *        fileReadAll (char *);
void          fileToWinPath (char *from, char *to, size_t maxlen);
void          fileNormPath (char *buff, size_t len);
void          fileRealPath (char *from, char *to);
char *        fileMakePath (char *buff, size_t buffsz, const char *subpath,
                  const char *base, const char *extension, int flags);

#endif /* INC_FILEUTIL_H */
