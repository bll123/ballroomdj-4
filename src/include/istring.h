#ifndef INC_ISTRING_H
#define INC_ISTRING_H

void      istringInit (const char *locale);
void      istringCleanup (void);
int       istringCompare (const char *, const char *);
size_t    istrlen (const char *);
void      istringToLower (char *str);
void      istringToUpper (char *str);

#endif /* INC_ISTRING_H */
