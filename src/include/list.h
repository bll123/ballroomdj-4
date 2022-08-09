#ifndef INC_LIST_H
#define INC_LIST_H

#include <stdint.h>
#include <stdbool.h>

typedef int32_t listidx_t;

typedef enum {
  LIST_KEY_STR,
  LIST_KEY_NUM,
} keytype_t;

typedef enum {
  LIST_UNORDERED,
  LIST_ORDERED,
} listorder_t;

typedef enum {
  LIST_TYPE_UNKNOWN,
  LIST_BASIC,
  LIST_NAMEVALUE,
} listtype_t;

typedef enum {
  VALUE_NONE,
  VALUE_DOUBLE,
  VALUE_DATA,
  VALUE_STR,
  VALUE_NUM,
  VALUE_LIST,
} valuetype_t;

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
  ssize_t     num;
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

enum {
  LIST_LOC_INVALID    = -1,
  LIST_VALUE_INVALID  = -65534,
  LIST_END_LIST       = -1,
  LIST_NO_VERSION     = -1,
};
#define LIST_DOUBLE_INVALID -65534.0

  /*
   * simple lists only store a list of data.
   */
list_t      *listAlloc (const char *name, listorder_t ordered,
                listFree_t valueFreeHook);
void        listFree (void *list);
void        listSetVersion (list_t *list, int version);
int         listGetVersion (list_t *list);
listidx_t     listGetCount (list_t *list);
listidx_t   listGetIdx (list_t *list, listkeylookup_t *key);
void        listSetSize (list_t *list, listidx_t size);
void        listSetVersion (list_t *list, listidx_t version);
void        listSet (list_t *list, listitem_t *item);
void        *listGetData (list_t *list, const char *keystr);
void        *listGetDataByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNumByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNum (list_t *list, const char *keystr);
void        listDeleteByIdx (list_t *, listidx_t idx);
void        listSort (list_t *list);
void        listStartIterator (list_t *list, listidx_t *iteridx);
listidx_t   listIterateKeyNum (list_t *list, listidx_t *iteridx);
char        *listIterateKeyStr (list_t *list, listidx_t *iteridx);
void        *listIterateValue (list_t *list, listidx_t *iteridx);
ssize_t     listIterateValueNum (list_t *list, listidx_t *iteridx);
listidx_t   listIterateGetIdx (list_t *list, listidx_t *iteridx);
void        listDumpInfo (list_t *list);

#endif /* INC_LIST_H */
