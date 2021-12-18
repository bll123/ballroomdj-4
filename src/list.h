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

typedef struct {
  long            bumper1;
  void            **data;      /* array of pointers */
  size_t          dsiz;
  size_t          count;
  listorder_t     ordered;
  listtype_t      type;
  valuetype_t     valuetype;
  long            bumper2;
} list_t;

typedef int (*listCompare_t)(void *, void *);

list_t *  listAlloc (size_t, listorder_t);
void      listFree (list_t *);
void      listFreeAll (list_t *);
list_t *  listAdd (list_t *, void *, listCompare_t);
void      listInsert (list_t *, size_t, void *);
long      listFind (list_t *, void *, listCompare_t);
void      listSort (list_t *, listCompare_t);

/* value list */

typedef struct {
  char        *name;
  union {
    char        *str;
    long        l;
  } u;
} namevalue_t;

list_t *  vlistAlloc (listorder_t, valuetype_t);
void      vlistFree (list_t *);
void      vlistFreeAll (list_t *);
list_t *  vlistAddStr (list_t *, char *, char *, listCompare_t);
list_t *  vlistAddLong (list_t *, char *, long, listCompare_t);
long      vlistFind (list_t *, char *, listCompare_t);
void      vlistSort (list_t *, listCompare_t);

#endif /* _INC_LIST */
