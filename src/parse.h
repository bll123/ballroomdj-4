#ifndef _INC_PARSE_H
#define _INC_PARSE_H

typedef struct {
  char    **strdata;
  size_t  allocCount;
} parseinfo_t;

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
int           parse (parseinfo_t *, char *);

#endif /* _INC_PARSE_H */
