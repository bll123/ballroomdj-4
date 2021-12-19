#ifndef _INC_LIST
#define _INC_LIST

typedef enum {
  LIST_ORDERED,
  LIST_UNORDERED
} listorder_t;

typedef enum {
  LIST_BASIC,
  LIST_NAMEVALUE
} listtype_t;

typedef enum {
  VALUE_NONE,
  VALUE_DATA,
  VALUE_LONG
} valuetype_t;

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
  listorder_t     ordered;
  listtype_t      type;
  valuetype_t     valuetype;
  listFree_t      freeHook;
  listFree_t      freeHookB;
} list_t;

list_t *  listAlloc (size_t, listorder_t, listCompare_t, listFree_t);
void      listFree (list_t *);
void      listSetSize (list_t *, size_t);
list_t *  listSet (list_t *, void *);
long      listFind (list_t *, void *);
void      listSort (list_t *);

/* value list */

typedef struct {
  char        *name;
  union {
    void        *data;
    long        l;
  } u;
} namevalue_t;

list_t *  vlistAlloc (listorder_t, valuetype_t, listCompare_t, listFree_t, listFree_t);
void      vlistFree (list_t *);
void      vlistSetSize (list_t *, size_t);
list_t *  vlistSetData (list_t *, char *, void *);
list_t *  vlistSetLong (list_t *, char *, long);
long      vlistFind (list_t *, char *);
void      vlistSort (list_t *);

#endif /* _INC_LIST */
