#ifndef _INC_TAGDEF_H
#define _INC_TAGDEF_H

typedef enum {
  AIDD_YES,
  AIDD_OPT,
  AIDD_NO
} audioiddispflag_t;

typedef enum {
  ET_SCALE,
  ET_CHECKBUTTON,
  ET_COMBOBOX,
  ET_ENTRY,
  ET_NA,
  ET_DISABLED_ENTRY
} edittype_t;

typedef enum {
  ANCHOR_WEST,
  ANCHOR_EAST,
  ANCHOR_BOTH
} anchor_t;

typedef struct {
  char                *name;
  char                *label;
  char                *shortLabel;
  unsigned int        defaultEditOrder;
  unsigned int        editIndex;
  unsigned int        editWidth;
  anchor_t            listingAnchor;
  unsigned int        listingWeight;
  anchor_t            songlistAnchor;
  unsigned int        songlistWeight;
  audioiddispflag_t   audioiddispflag;
  edittype_t          editType;
  unsigned int        isBdjTag : 1;
  unsigned int        isNormTag : 1;
  unsigned int        albumEdit : 1;
  unsigned int        allEdit : 1;
  unsigned int        isEditable : 1;
  unsigned int        listingDisplay : 1;
  unsigned int        songListDisplay : 1;
  unsigned int        textSearchable : 1;
} tagdef_t;

#endif /* _INC_TAGDEF_H */
