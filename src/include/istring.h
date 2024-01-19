/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ISTRING_H
#define INC_ISTRING_H

void      istringInit (const char *locale);
void      istringCleanup (void);
int       istringCompare (const char *, const char *);
size_t    istrlen (const char *);
void      istringToLower (char *str);
void      istringToUpper (char *str);
char *    istring16ToUTF8 (wchar_t *instr);

#endif /* INC_ISTRING_H */
