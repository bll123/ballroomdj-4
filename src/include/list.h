/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_LIST_H
#define INC_LIST_H

#include <stdint.h>
#include <stdbool.h>

typedef int32_t listidx_t;

typedef enum {
  LIST_UNORDERED,
  LIST_ORDERED,
} listorder_t;

typedef enum {
  VALUE_NONE,
  VALUE_DOUBLE,
  VALUE_DATA,
  VALUE_STR,
  VALUE_NUM,
  VALUE_LIST,
  VALUE_ALIAS,
} valuetype_t;

enum {
  LIST_LOC_INVALID    = -1,
  LIST_VALUE_INVALID  = -65534,
  LIST_END_LIST       = -1,
  LIST_NO_VERSION     = -1,
};
#define LIST_DOUBLE_INVALID -665534.0

typedef int64_t listnum_t;

typedef enum {
  LIST_KEY_STR,
  LIST_KEY_NUM,
} keytype_t;

typedef enum {
  LIST_TYPE_UNKNOWN,
  LIST_BASIC,
  LIST_NAMEVALUE,
} listtype_t;

typedef void (*listFree_t)(void *);

typedef union {
  const char  *strkey;
  listidx_t   idx;
} listkeylookup_t;

typedef union {
  char        *strkey;
  listidx_t   idx;
} listkey_t;

typedef union {
  void        *data;
  listnum_t   num;
  double      dval;
} listvalue_t;

typedef struct {
  listkey_t     key;
  valuetype_t   valuetype;
  listvalue_t   value;
} listitem_t;

typedef struct {
  char            *name;
  int             version;
  listidx_t       count;
  listidx_t       allocCount;
  int             maxKeyWidth;
  int             maxDataWidth;
  keytype_t       keytype;
  listorder_t     ordered;
  listitem_t      *data;        /* array */
  listkey_t       keyCache;
  listidx_t       locCache;
  long            readCacheHits;
  long            writeCacheHits;
  listFree_t      valueFreeHook;
  bool            replace : 1;
} list_t;

  /*
   * simple lists only store a list of data.
   */
list_t      *listAlloc (const char *name, keytype_t keytype, listorder_t ordered, listFree_t valueFreeHook);
void        listFree (keytype_t keytype, void *list);
listidx_t   listGetCount (keytype_t keytype, list_t *list);
listidx_t   listGetAllocCount (keytype_t keytype, list_t *list);
void        listSetVersion (keytype_t keytype, list_t *list, int version);
int         listGetVersion (keytype_t keytype, list_t *list);
listidx_t   listGetIdx (keytype_t keytype, list_t *list, listkeylookup_t *key);
void        listSetSize (keytype_t keytype, list_t *list, listidx_t size);
void        listSetVersion (keytype_t keytype, list_t *list, listidx_t version);
void        listSet (keytype_t keytype, list_t *list, listitem_t *item);
void        *listGetData (keytype_t keytype, list_t *list, const char *keystr);
void        *listGetDataByIdx (keytype_t keytype, list_t *list, listidx_t idx);
listnum_t   listGetNumByIdx (keytype_t keytype, list_t *list, listidx_t idx);
listnum_t   listGetNum (keytype_t keytype, list_t *list, const char *keystr);
void        listDeleteByIdx (keytype_t keytype, list_t *, listidx_t idx);
void        listSort (keytype_t keytype, list_t *list);
void        listStartIterator (keytype_t keytype, list_t *list, listidx_t *iteridx);
listidx_t   listIterateKeyNum (keytype_t keytype, list_t *list, listidx_t *iteridx);
listidx_t   listIterateKeyPreviousNum (keytype_t keytype, list_t *list, listidx_t *iteridx);
char        *listIterateKeyStr (keytype_t keytype, list_t *list, listidx_t *iteridx);
void        *listIterateValue (keytype_t keytype, list_t *list, listidx_t *iteridx);
listnum_t   listIterateValueNum (keytype_t keytype, list_t *list, listidx_t *iteridx);
listidx_t   listIterateGetIdx (keytype_t keytype, list_t *list, listidx_t *iteridx);
void        listDumpInfo (keytype_t keytype, list_t *list);
bool        listDebugIsCached (keytype_t keytype, list_t *list, listidx_t key);

#endif /* INC_LIST_H */
