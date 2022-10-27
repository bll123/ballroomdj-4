#ifndef INC_INLINE_H
#define INC_INLINE_H

#if ! defined (BDJ4_NO_INLINE)

inline void
dataFree (void *data)
{
  if (data != NULL) {
    free (data);
  }
}

#else

void dataFree (void *data);

#endif /* BDJ4_NO_INLINE */

#endif /* INC_INLINE_H */
