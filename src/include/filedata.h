/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILEDATA_H
#define INC_FILEDATA_H

char * filedataReadAll (const char *fname, size_t *len);
char * filedataReplace (char *data, size_t *dlen, const char *target, const char *repl);

#endif /* INC_FILEDATA_H */
