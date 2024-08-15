/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_INSTUTIL_H
#define INC_INSTUTIL_H

#include "sysvars.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  INST_ATI_BDJ4,
  INST_ATI_MAX,
};

#define WINUSERNAME       "%USERNAME%"

#define INST_HL_COLOR "#b16400"
#define INST_HL_CLASS "insthl"
#define INST_SEP_COLOR "#733000"
#define INST_SEP_CLASS "instsep"

#define INST_DISP_ACTION "-- "
#define INST_DISP_STATUS "   "
#define INST_DISP_ERROR "** "
#define INST_DISP_FIN "## "

typedef struct {
  const char  *name;
} instati_t;

enum {
  INST_TYPE_STD,
  INST_TYPE_ALT,
  INST_TYPE_UNKNOWN,
};

typedef struct {
  int     insttype;
} instinfo_t;

extern instati_t instati [INST_ATI_MAX];

void  instutilCreateLauncher (const char *name, const char *maindir,
          const char *target, int profilenum);
void  instutilCopyTemplates (void);
void  instutilCopyHttpFiles (void);
void  instutilGetMusicDir (char *homemusicdir, size_t sz);
void  instutilScanMusicDir (const char *musicdir, const char *rundir, char *ati, size_t atisz);
void  instutilAppendNameToTarget (char *buff, size_t sz, const char *name, int macosonly);
bool  instutilCheckForExistingInstall (const char *dir, const char *macospfx);
bool  instutilIsStandardInstall (const char *dir, const char *macospfx);
void  instutilRegister (const char *data);
void  instutilOldVersionString (sysversinfo_t *versinfo, char *buff, size_t sz);
void  instutilInstallCleanTmp (const char *rundir);
void  instutilCreateDataDirectories (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_INSTUTIL_H */
