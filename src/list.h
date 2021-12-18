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
  VALUE_STR,
  VALUE_LONG
} valuetype_t;

/* basic list */

typedef int (*listCompare_t)(void *, void *);

typedef struct {
  long            bumper1;
  void            **data;      /* array of pointers */
  long            bumper2;
  listCompare_t   compare;
  size_t          dsiz;
  size_t          count;
  listorder_t     ordered;
  listtype_t      type;
  valuetype_t     valuetype;
} list_t;

list_t *  listAlloc (size_t, listorder_t, listCompare_t);
void      listFree (list_t *);
void      listFreeAll (list_t *);
list_t *  listAdd (list_t *, void *);
void      listInsert (list_t *, size_t, void *);
long      listFind (list_t *, void *);
void      listSort (list_t *);

/* value list */

typedef struct {
  char        *name;
  union {
    char        *str;
    long        l;
  } u;
} namevalue_t;

list_t *  vlistAlloc (listorder_t, valuetype_t, listCompare_t);
void      vlistFree (list_t *);
void      vlistFreeAll (list_t *);
list_t *  vlistAddStr (list_t *, char *, char *);
list_t *  vlistAddLong (list_t *, char *, long);
long      vlistFind (list_t *, char *);
void      vlistSort (list_t *);

#endif /* _INC_LIST */
