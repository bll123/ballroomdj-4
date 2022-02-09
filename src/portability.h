#ifndef INC_PORTABILITY_H
#define INC_PORTABILITY_H

/* instead of using the real thing, just put a reasonable number in */
/* saves space, and the database can't handle it either. */
#if ! defined (MAXPATHLEN)
# define MAXPATHLEN         512
#endif

double        dRandom (void);
void          sRandom (void);

#endif /* INC_PORTABILITY_H */
