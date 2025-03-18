/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOSRC_H
#define INC_AUDIOSRC_H

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* supported types */
enum {
  AUDIOSRC_TYPE_NONE,
  AUDIOSRC_TYPE_FILE,
  AUDIOSRC_TYPE_BDJ4,
  AUDIOSRC_TYPE_MAX,
};

#define AS_FILE_PFX "file://"
#define AS_FILE_PFX_LEN (strlen (AS_FILE_PFX))

#define AS_BDJ4_PFX "bdj4://"
#define AS_BDJ4_PFX_LEN (strlen (AS_BDJ4_PFX))

/* audiosrc.c */

typedef struct audiosrc audiosrc_t;
typedef struct asdata asdata_t;
typedef struct asiter asiter_t;
typedef struct asiterdata asiterdata_t;

bool audiosrcEnabled (void);
void audiosrcInit (void);
void audiosrcCleanup (void);
void audiosrcPostInit (void);
int audiosrcGetCount (void);
int audiosrcGetType (const char *nm);
bool audiosrcExists (const char *nm);
bool audiosrcOriginalExists (const char *nm);
bool audiosrcRemove (const char *nm);
bool audiosrcPrep (const char *sfname, char *tempnm, size_t sz);
void audiosrcPrepClean (const char *sfname, const char *tempnm);
const char *audiosrcPrefix (const char *sfname);
void audiosrcURI (const char *sfname, char *uri, size_t sz, const char *prefix, int pfxlen);
void audiosrcFullPath (const char *sfname, char *fullpath, size_t sz, const char *prefix, int pfxlen);
const char * audiosrcRelativePath (const char *sfname, int pfxlen);

size_t audiosrcDir (const char *sfname, char *dir, size_t sz, int pfxlen);
asiter_t *audiosrcStartIterator (const char *uri);
void audiosrcCleanIterator (asiter_t *asiiter);
int32_t audiosrcIterCount (asiter_t *asiter);
const char *audiosrcIterate (asiter_t *asiter);

bool audiosrcGetPlaylistNames (int type);

void asiDesc (const char **ret, int max);
bool asiEnabled (void);
asdata_t *asiInit (const char *delpfx, const char *origext);
void asiFree (asdata_t *asdata);
void asiPostInit (asdata_t *asdata, const char *musicdir);
int asiTypeIdent (void);
bool asiIsTypeMatch (asdata_t *asdata, const char *nm);
bool asiExists (asdata_t *asdata, const char *nm);
bool asiOriginalExists (asdata_t *asdata, const char *nm);
bool asiRemove (asdata_t *asdata, const char *nm);
bool asiPrep (asdata_t *asdata, const char *sfname, char *tempnm, size_t sz);
void asiPrepClean (asdata_t *asdata, const char *tempnm);
const char *asiPrefix (asdata_t *asdata);
void asiURI (asdata_t *asdata, const char *sfname, char *uri, size_t sz, const char *prefix, int pfxlen);
void asiFullPath (asdata_t *asdata, const char *sfname, char *fullpath, size_t sz, const char *prefix, int pfxlen);
const char * asiRelativePath (asdata_t *asdata, const char *nm, int pfxlen);
size_t asiDir (asdata_t *asdata, const char *sfname, char *dir, size_t sz, int pfxlen);
asiterdata_t *asiStartIterator (asdata_t *asdata, const char *dir);
void asiCleanIterator (asdata_t *asdata, asiterdata_t *asidata);
int32_t asiIterCount (asdata_t *asdata, asiterdata_t *asidata);
const char *asiIterate (asdata_t *asdata, asiterdata_t *asidata);
bool asiGetPlaylistNames (asdata_t *asdata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_AUDIOSRC_H */
