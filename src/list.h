#ifndef INC_LIST_H
#define INC_LIST_H

#include <stdbool.h>

typedef ssize_t listidx_t;

typedef enum {
  LIST_KEY_STR,
  LIST_KEY_NUM,
} keytype_t;

typedef enum {
  LIST_ORDERED,
  LIST_UNORDERED,
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
  VALUE_NUM,
  VALUE_LIST,
} valuetype_t;

typedef void (*listFree_t)(void *);

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
  ssize_t         version;
  ssize_t         count;
  ssize_t         allocCount;
  int             maxKeyWidth;
  int             maxDataWidth;
  keytype_t       keytype;
  listorder_t     ordered;
  listitem_t      *data;        /* array */
  listkey_t       keyCache;
  listidx_t       locCache;
  long            readCacheHits;
  long            writeCacheHits;
  listFree_t      keyFreeHook;
  listFree_t      valueFreeHook;
  bool            replace : 1;
} list_t;

#define LIST_LOC_INVALID        -1L
#define LIST_VALUE_INVALID      -65534L
#define LIST_DOUBLE_INVALID     -65534.0

  /*
   * simple lists only store a list of data.
   */
list_t      *listAlloc (char *name, listorder_t ordered,
                listFree_t keyFreeHook, listFree_t valueFreeHook);
void        listFree (void *list);
ssize_t     listGetVersion (list_t *list);
ssize_t     listGetCount (list_t *list);
listidx_t   listGetIdx (list_t *list, listkey_t *key);
void        listSetSize (list_t *list, ssize_t size);
void        listSetVersion (list_t *list, ssize_t version);
void        listSet (list_t *list, listitem_t *item);
void        *listGetData (list_t *list, char *keystr);
void        *listGetDataByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNumByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNum (list_t *list, char *keystr);
void        listSort (list_t *list);
void        listStartIterator (list_t *list, listidx_t *iteridx);
void *      listIterateValue (list_t *list, listidx_t *iteridx);
ssize_t     listIterateNum (list_t *list, listidx_t *iteridx);
listidx_t   listIterateGetIdx (list_t *list, listidx_t *iteridx);
void        listDumpInfo (list_t *list);

#endif /* INC_LIST_H */
