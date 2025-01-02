/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VOLREG_H
#define INC_VOLREG_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int volregSave (const char *sink, int originalVolume);
int volregClear (const char *sink);
bool volregCheckBDJ3Flag (void);
void volregCreateBDJ4Flag (void);
void volregClearBDJ4Flag (void);
void volregClean (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_VOLREG_H */
