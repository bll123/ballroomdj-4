/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_TEMPLATEUTIL_H
#define INC_TEMPLATEUTIL_H

void templateImageCopy (const char *color);
void templateFileCopy (const char *fromfn, const char *tofn);
void templateProfileCopy (const char *fromfn, const char *tofn);
void templateHttpCopy (const char *fromfn, const char *tofn);
void templateDisplaySettingsCopy (void);

#endif /* INC_TEMPLATEUTIL_H */
