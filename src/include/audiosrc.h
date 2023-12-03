#ifndef INC_AUDIOSRC_H
#define INC_AUDIOSRC_H

/* supported types */
enum {
  AUDIOSRC_TYPE_FILE,
  AUDIOSRC_TYPE_YOUTUBE,
};

/* audiosrc.c */

bool audiosrcExists (const char *nm);
bool audiosrcRemove (const char *nm);
char *audiosrcPrep (const char *nm);

/* audiosrcfile.c */

bool audiosrcfileExists (const char *nm);
bool audiosrcfileRemove (const char *nm);
char *audiosrcfilePrep (const char *nm);

#endif /* INC_AUDIOSRC_H */
