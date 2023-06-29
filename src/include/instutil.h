/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_INSTUTIL_H
#define INC_INSTUTIL_H

void  instutilCreateShortcut (const char *name, const char *maindir,
          const char *target, int profilenum);
void  instutilCopyTemplates (void);
void  instutilCopyHttpFiles (void);
void  instutilGetMusicDir (char *homemusicdir, size_t sz);

#endif /* INC_INSTUTIL_H */
