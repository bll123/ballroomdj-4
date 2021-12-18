#ifndef _INC_RAFILE_H
#define _INC_RAFILE_H

#include <stdio.h>

typedef struct {
  FILE        *fh;
  char        *fname;
  int         version;
  int         size;
  size_t      count;
} rafile_t;

#define RAFILE_NEW          0L
#define RAFILE_REC_SIZE     2048
#define RAFILE_HDR_SIZE     128

#define RRN_TO_OFFSET(rrn)  (((long) (rrn) - 1L) * RAFILE_REC_SIZE + RAFILE_HDR_SIZE)

rafile_t *    raOpen (char *, int);
void          raClose (rafile_t *);
int           raWrite (rafile_t *, size_t, char *);
int           raClear (rafile_t *, size_t);
size_t        raRead (rafile_t *, size_t, char *);
size_t        raGetNextRRN (rafile_t *);

#endif /* _INC_RAFILE_H */
