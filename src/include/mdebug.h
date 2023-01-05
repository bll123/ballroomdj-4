/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MDEBUG_H
#define INC_MDEBUG_H

#if defined (BDJ4_MEM_DEBUG)
# define mdfree(d)        mdfree_r (d, __FILE__, __LINE__)
# define mdmalloc(sz)     mdmalloc_r (sz, __FILE__, __LINE__)
# define mdrealloc(d,sz)  mdrealloc_r (d, sz, __FILE__, __LINE__)
# define mdstrdup(s)      mdstrdup_r (s, __FILE__, __LINE__)
# define dataFree(d)      mddataFree_r (d, __FILE__, __LINE__)
#else
# define mdfree free
# define mdmalloc malloc
# define mdrealloc realloc
# define mdstrdup strdup
# define dataFree dataFree_r
#endif

void mdfree_r (void *data, const char *fn, int lineno);
void * mdmalloc_r (size_t sz, const char *fn, int lineno);
void * mdrealloc_r (void *data, size_t sz, const char *fn, int lineno);
char * mdstrdup_r (const char *s, const char *fn, int lineno);
void dataFree_r (void *data);
void mddataFree_r (void *data, const char *fn, int lineno);
void mdebugReport (void);
void mdebugInit (const char *tag);
void mdebugCleanup (void);
long mdebugCount (void);
long mdebugErrors (void);

#endif /* INC_MDEBUG_H */
