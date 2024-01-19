#ifndef INC_AUDIOSRC_H
#define INC_AUDIOSRC_H

#include <stdint.h>

/* supported types */
enum {
  AUDIOSRC_TYPE_FILE,
  AUDIOSRC_TYPE_YOUTUBE,
};

/* audiosrc.c */

typedef struct asiter asiter_t;
typedef struct asiterdata asiterdata_t;

int audiosrcGetType (const char *nm);
bool audiosrcExists (const char *nm);
bool audiosrcOriginalExists (const char *nm);
bool audiosrcRemove (const char *nm);
bool audiosrcPrep (const char *sfname, char *tempnm, size_t sz);
void audiosrcPrepClean (const char *sfname, const char *tempnm);
void audiosrcFullPath (const char *sfname, char *tempnm, size_t sz, int pfxlen, const char *ffn);
const char * audiosrcRelativePath (const char *sfname, int pfxlen);
asiter_t *audiosrcStartIterator (const char *uri);
void audiosrcCleanIterator (asiter_t *asiiter);
int32_t audiosrcIterCount (asiter_t *asiter);
const char *audiosrcIterate (asiter_t *asiter);

/* audiosrcfile.c */

bool audiosrcfileExists (const char *nm);
bool audiosrcfileOriginalExists (const char *nm);
bool audiosrcfileRemove (const char *nm);
bool audiosrcfilePrep (const char *sfname, char *tempnm, size_t sz);
void audiosrcfilePrepClean (const char *tempnm);
void audiosrcfileFullPath (const char *sfname, char *tempnm, size_t sz, int pfxlen, const char *oldfn);
const char * audiosrcfileRelativePath (const char *nm, int pfxlen);
asiterdata_t *audiosrcfileStartIterator (const char *dir);
void audiosrcfileCleanIterator (asiterdata_t *asidata);
int32_t audiosrcfileIterCount (asiterdata_t *asidata);
const char *audiosrcfileIterate (asiterdata_t *asidata);

/* audiosrcyoutube.c */

bool audiosrcyoutubeExists (const char *nm);
asiterdata_t *audiosrcyoutubeStartIterator (const char *dir);
void audiosrcyoutubeCleanIterator (asiterdata_t *asidata);
int32_t audiosrcyoutubeIterCount (asiterdata_t *asidata);
const char *audiosrcyoutubeIterate (asiterdata_t *asidata);

#endif /* INC_AUDIOSRC_H */
