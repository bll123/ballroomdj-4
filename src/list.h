#ifndef INC_LIST
#define INC_LIST

typedef enum {
  KEY_STR,
  KEY_LONG
} keytype_t;

typedef enum {
  LIST_ORDERED,
  LIST_UNORDERED
} listorder_t;

typedef enum {
  LIST_BASIC,
  LIST_NAMEVALUE
} listtype_t;

typedef enum {
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

typedef struct {
  char            *name;
  long            bumper1;
  void            **data;      /* array of pointers */
  long            bumper2;
  listCompare_t   compare;
  size_t          dsiz;
  size_t          count;
  size_t          allocCount;
  size_t          iteratorIndex;
  keytype_t       keytype;
  listorder_t     ordered;
  listtype_t      type;
  listFree_t      freeHook;
  listFree_t      freeHookB;
  keytype_t       cacheKeyType;
  listkey_t       keyCache;
  long            locCache;
  long            cacheHits;
} list_t;

#define LIST_LOC_INVALID        -1L

  /*
   * simple lists only store a list of data.
   */
list_t *  listAlloc (char *name, size_t datasize,
              listCompare_t compare, listFree_t freefunc);
void      listFree (void *list);
void      listSetSize (list_t *list, size_t size);
list_t *  listSet (list_t *list, void *data);
void      listSort (list_t *list);
void      listStartIterator (list_t *list);
void *    listIterateData (list_t *list);

  /*
   * key/value list.  keyed by a string.
   */
list_t *  slistAlloc (char *name, listorder_t, listCompare_t,
              listFree_t, listFree_t);
void      slistFree (void *);
void      slistSetSize (list_t *, size_t);
list_t *  slistSetData (list_t *, char *, void *);
list_t *  slistSetLong (list_t *, char *, long);
list_t *  slistSetDouble (list_t *, char *, double);
list_t *  slistSetList (list_t *, char *, list_t *);
void *    slistGetData (list_t *, char *);
long      slistGetLong (list_t *, char *);
double    slistGetDouble (list_t *, char *);
void      slistSort (list_t *);
void      slistStartIterator (list_t *list);
char *    slistIterateKeyStr (list_t *list);

  /*
   * key/value list.  keyed by a long.
   */
list_t *  llistAlloc (char *name, listorder_t, listFree_t);
void      llistFree (void *);
void      llistSetFreeHook (list_t *, listFree_t freefunc);
void      llistSetSize (list_t *, size_t);
list_t *  llistSetData (list_t *, long, void *);
list_t *  llistSetLong (list_t *, long, long);
list_t *  llistSetDouble (list_t *, long, double);
list_t *  llistSetList (list_t *, long, list_t *);
void *    llistGetData (list_t *, long);
long      llistGetLong (list_t *, long);
double    llistGetDouble (list_t *, long);
void      llistSort (list_t *);
void      llistStartIterator (list_t *list);
long      llistIterateKeyLong (list_t *list);

#endif /* INC_LIST */
