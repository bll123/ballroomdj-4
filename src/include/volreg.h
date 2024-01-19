/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VOLREG_H
#define INC_VOLREG_H

int volregSave (const char *sink, int originalVolume);
int volregClear (const char *sink);
bool volregCheckBDJ3Flag (void);
void volregCreateBDJ4Flag (void);
void volregClearBDJ4Flag (void);
void volregClean (void);

#endif /* INC_VOLREG_H */
