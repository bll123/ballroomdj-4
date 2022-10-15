#ifndef INC_INLINE_H
#define INC_INLINE_H

inline void
dataFree (void *data)
{
  if (data != NULL) {
    free (data);
  }
}

#endif /* INC_INLINE_H */
