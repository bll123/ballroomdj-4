#ifndef INC_AUDIOSRC_H
#define INC_AUDIOSRC_H

/* supported types */
enum {
  AUDIOSRC_TYPE_FILE,
  AUDIOSRC_TYPE_YOUTUBE,
};

/* audiosrc.c */

int audiosrcGetType (const char *nm);
bool audiosrcExists (const char *nm);
bool audiosrcRemove (const char *nm);
bool audiosrcPrep (const char *sfname, char *tempnm, size_t sz);
void audiosrcPrepClean (const char *sfname, const char *tempnm);
void audiosrcFullPath (const char *sfname, char *tempnm, size_t sz);

/* audiosrcfile.c */

bool audiosrcfileExists (const char *nm);
bool audiosrcfileRemove (const char *nm);
bool audiosrcfilePrep (const char *sfname, char *tempnm, size_t sz);
void audiosrcfilePrepClean (const char *tempnm);
void audiosrcfileFullPath (const char *sfname, char *tempnm, size_t sz);

#endif /* INC_AUDIOSRC_H */
