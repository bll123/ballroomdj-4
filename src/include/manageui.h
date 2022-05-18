#ifndef INC_MANAGEUI_H
#define INC_MANAGEUI_H

#include "manageui.h"
#include "nlist.h"
#include "playlist.h"
#include "uiutils.h"

typedef struct manageseq manageseq_t;

/* managepl.c */

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
