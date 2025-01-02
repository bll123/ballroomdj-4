/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_LISTMODULE_H
#define INC_LISTMODULE_H

#include "list.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  LIST_KEY_STR,
  LIST_KEY_NUM,
  LIST_KEY_IND,
} keytype_t;

list_t      *listAlloc (const char *name, keytype_t keytype, listorder_t ordered, listFree_t valueFreeHook);
void        listFree (keytype_t keytype, void *list);
/* list management */
void        listSetSize (keytype_t keytype, list_t *list, listidx_t size);
void        listSort (keytype_t keytype, list_t *list);
void        listCalcMaxValueWidth (keytype_t keytype, list_t *list);
const char  *listGetName (keytype_t keytype, list_t *list);
void        listSetFreeHook (keytype_t keytype, list_t *list, listFree_t valueFreeHook);

/* counts */
listidx_t   listGetCount (keytype_t keytype, list_t *list);
int         listGetMaxValueWidth (keytype_t keytype, list_t *list);

/* version */
void        listSetVersion (keytype_t keytype, list_t *list, int version);
int         listGetVersion (keytype_t keytype, list_t *list);

/* iterators */
listidx_t   listIterateKeyNum (keytype_t keytype, list_t *list, listidx_t *iteridx);
listidx_t   listIterateKeyPreviousNum (keytype_t keytype, list_t *list, listidx_t *iteridx);
const char  *listIterateKeyStr (keytype_t keytype, list_t *list, listidx_t *iteridx);
void        *listIterateValue (keytype_t keytype, list_t *list, listidx_t *iteridx);
listnum_t   listIterateValueNum (keytype_t keytype, list_t *list, listidx_t *iteridx);

/* get */
listidx_t   listGetIdxNumKey (keytype_t keytype, list_t *list, listidx_t key);
listidx_t   listGetIdxStrKey (keytype_t keytype, list_t *list, const char *strkey);
listidx_t   listGetKeyNumByIdx (keytype_t keytype, list_t *list, listidx_t idx);
const char  *listGetKeyStrByIdx (keytype_t keytype, list_t *list, listidx_t idx);
void        *listGetDataByIdx (keytype_t keytype, list_t *list, listidx_t idx);
const char  *listGetStrByIdx (keytype_t keytype, list_t *list, listidx_t idx);
listnum_t   listGetNumByIdx (keytype_t keytype, list_t *list, listidx_t idx);
double      listGetDoubleByIdx (keytype_t keytype, list_t *list, listidx_t idx);

/* set */
void        listSetStrData (keytype_t keytype, list_t *list, const char *key, void *data);
void        listSetStrList (keytype_t keytype, list_t *list, const char *key, void *data);
void        listSetStrStr (keytype_t keytype, list_t *list, const char *key, const char *str);
void        listSetStrNum (keytype_t keytype, list_t *list, const char *key, listnum_t val);
void        listSetNumData (keytype_t keytype, list_t *list, listidx_t key, void *data);
void        listSetNumList (keytype_t keytype, list_t *list, listidx_t key, void *data);
void        listSetNumStr (keytype_t keytype, list_t *list, listidx_t key, const char *str);
void        listSetNumNum (keytype_t keytype, list_t *list, listidx_t key, listnum_t val);
void        listSetNumDouble (keytype_t keytype, list_t *list, listidx_t key, double dval);

void        listDeleteByIdx (keytype_t keytype, list_t *, listidx_t idx);

/* debug / information */

listidx_t   listGetAllocCount (keytype_t keytype, list_t *list);
void        listDumpInfo (keytype_t keytype, list_t *list);
bool        listDebugIsCached (keytype_t keytype, list_t *list, listidx_t key);
int         listGetOrdering (keytype_t keytype, list_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_LISTMODULE_H */
