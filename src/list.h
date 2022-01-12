#ifndef INC_LIST_H
#define INC_LIST_H

typedef ssize_t listidx_t;

typedef enum {
  KEY_STR,
  KEY_NUM,
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

typedef int (*listCompare_t)(void *, void *);
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
  ssize_t         count;
  ssize_t         allocCount;
  keytype_t       keytype;
  listorder_t     ordered;
  long            bumper1;
  listitem_t      *data;        /* array */
  long            bumper2;
  listCompare_t   compare;
  ssize_t         iteratorIndex;
  ssize_t         currentIndex;
  listkey_t       keyCache;
  listidx_t       locCache;
  long            readCacheHits;
  long            writeCacheHits;
  listFree_t      keyFreeHook;
  listFree_t      valueFreeHook;
} list_t;

#define LIST_LOC_INVALID        -1L

  /*
   * simple lists only store a list of data.
   */
list_t      *listAlloc (char *name, listorder_t ordered, listCompare_t compare,
                listFree_t keyFreeHook, listFree_t valueFreeHook);
void        listFree (void *list);
ssize_t     listGetCount (list_t *list);
void        listSetSize (list_t *list, ssize_t size);
listidx_t   listSetData (list_t *list, char *keystr, void *data);
listidx_t   listSetNum (list_t *list, char *keystr, ssize_t lval);
listidx_t   listSetDouble (list_t *list, char *keystr, double dval);
listidx_t   listSetList (list_t *list, char *keystr, list_t *data);
void        *listGetData (list_t *list, char *keystr);
void        *listGetDataByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNumByIdx (list_t *list, listidx_t idx);
ssize_t     listGetNum (list_t *list, char *keystr);
double      listGetDouble (list_t *list, char *keystr);
list_t      *listGetList (list_t *list, char *keystr);
listidx_t   listGetStrIdx (list_t *list, char *keystr);
void        listSort (list_t *list);
void        listStartIterator (list_t *list);
void *      listIterateKeyStr (list_t *list);
void *      listIterateValue (list_t *list);
ssize_t     listIterateNum (list_t *list);
listidx_t   listIterateGetIdx (list_t *list);
void        listDumpInfo (list_t *list);

  /* keyed by a listidx_t */
list_t *  llistAlloc (char *name, listorder_t, listFree_t);
void      llistFree (void *);
ssize_t   llistGetCount (list_t *list);
void      llistSetSize (list_t *, ssize_t);
void      llistSetFreeHook (list_t *, listFree_t valueFreeHook);
listidx_t llistSetData (list_t *, listidx_t idx, void *data);
listidx_t llistSetNum (list_t *, listidx_t idx, ssize_t lval);
listidx_t llistSetDouble (list_t *, listidx_t idx, double dval);
listidx_t llistSetList (list_t *, listidx_t idx, list_t *listval);
void *    llistGetData (list_t *, listidx_t idx);
void *    llistGetDataByIdx (list_t *, listidx_t idx);
ssize_t   llistGetNumByIdx (list_t *list, listidx_t idx);
ssize_t   llistGetNum (list_t *, listidx_t idx);
double    llistGetDouble (list_t *, listidx_t idx);
list_t    *llistGetList (list_t *, listidx_t idx);
void      llistSort (list_t *);
void      llistStartIterator (list_t *list);
listidx_t llistIterateKeyNum (list_t *list);
void *    llistIterateValue (list_t *list);
ssize_t   llistIterateNum (list_t *list);
void      llistDumpInfo (list_t *list);

listidx_t ilistSetData (list_t *, listidx_t ikey, listidx_t lidx, void *value);
listidx_t ilistSetNum (list_t *, listidx_t ikey, listidx_t lidx, ssize_t value);
listidx_t ilistSetDouble (list_t *, listidx_t ikey, listidx_t lidx, double value);
void *    ilistGetData (list_t *, listidx_t ikey, listidx_t lidx);
ssize_t   ilistGetNum (list_t *, listidx_t ikey, listidx_t lidx);
double    ilistGetDouble (list_t *, listidx_t ikey, listidx_t lidx);

#endif /* INC_LIST_H */
