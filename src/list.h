#ifndef INC_LIST_H
#define INC_LIST_H

typedef enum {
  KEY_STR,
  KEY_LONG,
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
  VALUE_LONG,
  VALUE_LIST,
} valuetype_t;

typedef int (*listCompare_t)(void *, void *);
typedef void (*listFree_t)(void *);

typedef union {
  char        *strkey;
  long        lkey;
} listkey_t;

typedef union {
  void        *data;
  long        l;
  double      d;
} listvalue_t;

typedef struct {
  listkey_t     key;
  valuetype_t   valuetype;
  listvalue_t   value;
} listitem_t;

typedef struct {
  char            *name;
  size_t          count;
  size_t          allocCount;
  keytype_t       keytype;
  listorder_t     ordered;
  long            bumper1;
  listitem_t      *data;        /* array */
  long            bumper2;
  listCompare_t   compare;
  size_t          iteratorIndex;
  size_t          currentIndex;
  listkey_t       keyCache;
  long            locCache;
  long            cacheHits;
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
size_t      listGetSize (list_t *list);
void        listSetSize (list_t *list, size_t size);
void        listSetData (list_t *list, char *keystr, void *data);
void        listSetLong (list_t *list, char *keystr, long lval);
void        listSetDouble (list_t *list, char *keystr, double dval);
void        listSetList (list_t *list, char *keystr, list_t *data);
void        *listGetData (list_t *list, char *keystr);
long        listGetLong (list_t *list, char *keystr);
double      listGetDouble (list_t *list, char *keystr);
list_t      *listGetList (list_t *list, char *keystr);
valuetype_t listGetValueType (list_t *, char *keystr);
long        listGetStrIdx (list_t *list, char *keystr);
void        listSort (list_t *list);
void        listStartIterator (list_t *list);
void *      listIterateKeyStr (list_t *list);

  /* keyed by a long */
list_t *  llistAlloc (char *name, listorder_t, listFree_t);
void      llistFree (void *);
void      llistSetSize (list_t *, size_t);
void      llistSetFreeHook (list_t *, listFree_t valueFreeHook);
void      llistSetData (list_t *, long, void *);
void      llistSetLong (list_t *, long, long);
void      llistSetDouble (list_t *, long, double);
void      llistSetList (list_t *, long, list_t *);
void *    llistGetData (list_t *, long);
long      llistGetLong (list_t *, long);
double    llistGetDouble (list_t *, long);
list_t    *llistGetList (list_t *, long);
valuetype_t llistGetValueType (list_t *, long);
long      listGetLongIdx (list_t *list, long lkey);
void      llistSort (list_t *);
void      llistStartIterator (list_t *list);
long      llistIterateKeyLong (list_t *list);

#endif /* INC_LIST_H */
