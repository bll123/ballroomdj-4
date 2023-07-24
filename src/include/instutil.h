/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_INSTUTIL_H
#define INC_INSTUTIL_H

enum {
  INST_ATI_BDJ4,
  INST_ATI_MUTAGEN,
  INST_ATI_MAX,
};

#define BDJ4_MACOS_DIR    "BDJ4.app"
#define MACOS_PREFIX      "/Contents/MacOS"

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
  bool        needmutagen;
} instati_t;

extern instati_t instati [INST_ATI_MAX];

void  instutilCreateShortcut (const char *name, const char *maindir,
          const char *target, int profilenum);
void  instutilCopyTemplates (void);
void  instutilCopyHttpFiles (void);
void  instutilGetMusicDir (char *homemusicdir, size_t sz);
void  instutilScanMusicDir (const char *musicdir, const char *rundir, char *ati, size_t atisz);
void  instutilAppendNameToTarget (char *buff, size_t sz, int macosonly);
bool  instutilCheckForExistingInstall (const char *rundir, const char *dir);

#endif /* INC_INSTUTIL_H */
