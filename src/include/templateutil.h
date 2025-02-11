/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_TEMPLATEUTIL_H
#define INC_TEMPLATEUTIL_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void templateImageCopy (const char *color);
void templateFileCopy (const char *fromfn, const char *tofn);
void templateProfileCopy (const char *fromfn, const char *tofn);
void templateHttpCopy (const char *fromfn, const char *tofn);
void templateDisplaySettingsCopy (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_TEMPLATEUTIL_H */
