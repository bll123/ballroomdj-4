#ifndef INC_LIST
#define INC_LIST

typedef enum {
  LIST_ORDERED,
  LIST_UNORDERED
} listorder_t;

typedef enum {
  LIST_BASIC,
  LIST_NAMEVALUE
} listtype_t;

typedef enum {
  KEY_STR,
  KEY_LONG
} keytype_t;

typedef enum {
  VALUE_DOUBLE,
  VALUE_DATA,
  VALUE_LONG
} valuetype_t;

typedef union {
  char        *name;
  long        key;
} listkey_t;

/* basic list */

typedef int (*listCompare_t)(void *, void *);
typedef void (*listFree_t)(void *);

typedef struct {
  long            bumper1;
  void            **data;      /* array of pointers */
  long            bumper2;
  listCompare_t   compare;
  size_t          dsiz;
  size_t          count;
  size_t          allocCount;
  keytype_t       keytype;
  listorder_t     ordered;
  listtype_t      type;
  listFree_t      freeHook;
  listFree_t      freeHookB;
} list_t;

list_t *  listAlloc (size_t, listorder_t, listCompare_t, listFree_t);
void      listFree (void *);
void      listSetSize (list_t *, size_t);
list_t *  listSet (list_t *, void *);
long      listFind (list_t *, listkey_t);
void      listSort (list_t *);

/* value list */

typedef struct {
  listkey_t         key;
  valuetype_t   valuetype;
  union {
    void        *data;
    long        l;
    double      d;
  } u;
} namevalue_t;

list_t *  vlistAlloc (keytype_t, listorder_t, listCompare_t,
              listFree_t, listFree_t);
void      vlistFree (void *);
void      vlistSetSize (list_t *, size_t);
list_t *  vlistSetData (list_t *, listkey_t, void *);
list_t *  vlistSetLong (list_t *, listkey_t, long);
list_t *  vlistSetDouble (list_t *, listkey_t, double);
void *    vlistGetData (list_t *, listkey_t);
long      vlistGetLong (list_t *, listkey_t);
double    vlistGetDouble (list_t *, listkey_t);
long      vlistFind (list_t *, listkey_t);
void      vlistSort (list_t *);

#endif /* INC_LIST */
