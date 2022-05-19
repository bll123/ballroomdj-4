#ifndef INC_MANAGEUI_H
#define INC_MANAGEUI_H

#include "nlist.h"
#include "playlist.h"
#include "uiutils.h"

typedef struct manageseq manageseq_t;
typedef struct managepl managepl_t;
typedef struct managepltree managepltree_t;

/* managepl.c */
managepl_t *managePlaylistAlloc (UIWidget *window, nlist_t *options,
    UIWidget *statusMsg);
void managePlaylistFree (managepl_t *managepl);
void manageBuildUIPlaylist (managepl_t *managepl, UIWidget *vboxp);
uimenu_t *managePlaylistMenu (managepl_t *managepl, GtkWidget *menubar);
void managePlaylistSave (managepl_t *managepl);

/* managepltree.c */
managepltree_t *managePlaylistTreeAlloc (void);
void managePlaylistTreeFree (managepltree_t *managepltree);
void manageBuildUIPlaylistTree (managepltree_t *managepltree, UIWidget *vboxp);
void managePlaylistTreePopulate (managepltree_t *managepltree, playlist_t *pl);

/* manageseq.c */
manageseq_t *manageSequenceAlloc (UIWidget *window, nlist_t *options,
    UIWidget *statusMsg);
void manageSequenceFree (manageseq_t *manageseq);
void manageBuildUISequence (manageseq_t *manageseq, UIWidget *vboxp);
uimenu_t *manageSequenceMenu (manageseq_t *manageseq, GtkWidget *menubar);
void manageSequenceSave (manageseq_t *manageseq);

/* managemisc.c */
void manageSetStatusMsg (UIWidget *statusMsg, const char *msg);
void manageRenamePlaylistFiles (const char *oldname, const char *newname);
void manageCheckAndCreatePlaylist (const char *name,
    const char *suppfname, pltype_t pltype);
bool manageCreatePlaylistCopy (UIWidget *statusMsg,
    const char *oname, const char *newname);

#endif /* INC_MANAGEUI_H */
