/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdint.h>

#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* supported types */
/* if new audio sources are added, configui/confas.c must be manually */
/* updated to include the new types */
/* libbasic/asconf.c must also be updated to include the new types */
enum {
  AUDIOSRC_TYPE_NONE,
  AUDIOSRC_TYPE_FILE,
  AUDIOSRC_TYPE_BDJ4,
  AUDIOSRC_TYPE_PODCAST,
  AUDIOSRC_TYPE_MAX,
};

typedef enum {
  AS_ITER_AUDIO_SRC,
  AS_ITER_DIR,
  AS_ITER_PL_NAMES,
  AS_ITER_PL,
  AS_ITER_TAGS,
} asitertype_t;

#define AS_FILE_PFX "file://"
#define AS_BDJ4_PFX "bdj4://"
#define AS_HTTP_PFX "http://"
#define AS_HTTPS_PFX "https://"
#define AS_XML_SFX ".xml"
enum {
  AS_FILE_PFX_LEN = strlen (AS_FILE_PFX),
  AS_BDJ4_PFX_LEN = strlen (AS_BDJ4_PFX),
  AS_HTTP_PFX_LEN = strlen (AS_HTTP_PFX),
  AS_HTTPS_PFX_LEN = strlen (AS_HTTPS_PFX),
  AS_XML_SFX_LEN = strlen (AS_XML_SFX),
};

/* audiosrc.c */

typedef struct audiosrc audiosrc_t;
typedef struct asdata asdata_t;
typedef struct asiter asiter_t;
typedef struct asiterdata asiterdata_t;

void audiosrcInit (void);
void audiosrcCleanup (void);
void audiosrcPostInit (void);
int audiosrcGetActiveCount (void);
int audiosrcGetType (const char *nm);
bool audiosrcCheckConnection (int askey, const char *uri);
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
asiter_t *audiosrcStartIterator (int type, asitertype_t asitertype, const char *uri, const char *nm, int askey);
void audiosrcCleanIterator (asiter_t *asiiter);
int32_t audiosrcIterCount (asiter_t *asiter);
const char *audiosrcIterate (asiter_t *asiter);
const char *audiosrcIterateValue (asiter_t *asiter, const char *key);

void asiDesc (const char **ret, int max);
asdata_t *asiInit (const char *delpfx, const char *origext);
void asiFree (asdata_t *asdata);
void asiPostInit (asdata_t *asdata, const char *uri);
int asiTypeIdent (void);
bool asiIsTypeMatch (asdata_t *asdata, const char *nm);
bool asiCheckConnection (asdata_t *asdata, int askey, const char *uri);
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
asiterdata_t *asiStartIterator (asdata_t *asdata, asitertype_t asitertype, const char *uri, const char *nm, int askey);
void asiCleanIterator (asdata_t *asdata, asiterdata_t *asidata);
int32_t asiIterCount (asdata_t *asdata, asiterdata_t *asidata);
const char *asiIterate (asdata_t *asdata, asiterdata_t *asidata);
const char * asiIterateValue (asdata_t *asdata, asiterdata_t *asidata, const char *key);
bool asiGetPlaylistNames (asdata_t *asdata, int askey);
bool asiSongTags (asdata_t *asdata, const char *uri);

/* audiosrcutil.c */
void audiosrcutilMakeTempName (const char *ffn, char *tempnm, size_t maxlen);
bool audiosrcutilPreCacheFile (const char *fn);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

