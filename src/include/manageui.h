/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MANAGEUI_H
#define INC_MANAGEUI_H

#include "conn.h"
#include "dispsel.h"
#include "msgparse.h"
#include "musicdb.h"
#include "nlist.h"
#include "playlist.h"
#include "procutil.h"
#include "song.h"
#include "uiwcont.h"
#include "uisongsel.h"

enum {
  MANAGE_PRELOAD,
  MANAGE_PRELOAD_FORCE,
  MANAGE_STD,
  MANAGE_CREATE,    // used for create-from-playlist
};

enum {
  MANAGE_DB_COUNT_SAVE = 10,
};

typedef struct {
  uiwcont_t         *window;
  dispsel_t         *dispsel;
  uiwcont_t         *statusMsg;
  uiwcont_t         *errorMsg;
  nlist_t           *options;
  const char        *pleasewaitmsg;
} manageinfo_t;

/* managepl.c */
typedef struct managepl managepl_t;

managepl_t *managePlaylistAlloc (manageinfo_t *minfo);
void managePlaylistFree (managepl_t *managepl);
void managePlaylistSetLoadCallback (managepl_t *managepl, callback_t *uicb);
void manageBuildUIPlaylist (managepl_t *managepl, uiwcont_t *vboxp);
uiwcont_t *managePlaylistMenu (managepl_t *managepl, uiwcont_t *menubar);
void managePlaylistSave (managepl_t *managepl);
void managePlaylistLoadCheck (managepl_t *managepl);
void managePlaylistLoadFile (managepl_t *managepl, const char *fn, int preloadflag);
bool managePlaylistNew (managepl_t *managepl, int preloadflag);

/* managepltree.c */
typedef struct managepltree managepltree_t;

managepltree_t *managePlaylistTreeAlloc (uiwcont_t *errorMsg);
void managePlaylistTreeFree (managepltree_t *managepltree);
void manageBuildUIPlaylistTree (managepltree_t *managepltree, uiwcont_t *vboxp);
void managePlaylistTreePrePopulate (managepltree_t *managepltree, playlist_t *pl);
void managePlaylistTreePopulate (managepltree_t *managepltree, playlist_t *pl);
bool managePlaylistTreeIsChanged (managepltree_t *managepltree);
void managePlaylistTreeUpdatePlaylist (managepltree_t *managepltree);

/* manageseq.c */
typedef struct manageseq manageseq_t;

manageseq_t *manageSequenceAlloc (manageinfo_t *minfo);
void manageSequenceFree (manageseq_t *manageseq);
void manageSequenceSetLoadCallback (manageseq_t *manageseq, callback_t *uicb);
void manageSequenceSetNewCallback (manageseq_t *manageseq, callback_t *uicb);
void manageBuildUISequence (manageseq_t *manageseq, uiwcont_t *vboxp);
uiwcont_t *manageSequenceMenu (manageseq_t *manageseq, uiwcont_t *menubar);
void manageSequenceSave (manageseq_t *manageseq);
void manageSequenceLoadCheck (manageseq_t *manageseq);
void manageSequenceLoadFile (manageseq_t *manageseq, const char *fn, int preloadflag);

/* managedb.c */
typedef struct managedb managedb_t;

managedb_t *manageDbAlloc (manageinfo_t *minfo, conn_t *conn, procutil_t **processes);
void  manageDbFree (managedb_t *managedb);
void  manageDbProcess (managedb_t *managedb);
void  manageBuildUIUpdateDatabase (managedb_t *managedb, uiwcont_t *vboxp);
bool  manageDbChg (void *udata);
void  manageDbProgressMsg (managedb_t *managedb, char *args);
void  manageDbStatusMsg (managedb_t *managedb, char *args);
void  manageDbFinish (managedb_t *managedb, int routefrom);
void  manageDbClose (managedb_t *managedb);
void  manageDbResetButtons (managedb_t *managedb);

/* managemisc.c */
bool manageCreatePlaylistCopy (uiwcont_t *errorMsg,
    const char *oname, const char *newname);
void manageDeletePlaylist (uiwcont_t *statusMsg, const char *name);
char * manageTrimName (const char *name);

/* managestats.c */
typedef struct managestats managestats_t;

managestats_t *manageStatsInit (conn_t *conn, musicdb_t *musicdb);
void  manageStatsFree (managestats_t *managestats);
void  manageStatsSetDatabase (managestats_t *managestats, musicdb_t *musicdb);
uiwcont_t *manageBuildUIStats (managestats_t *managestats);
void manageStatsProcessData (managestats_t *managestats, mp_musicqupdate_t *musicqupdate);

/* manageaudioid.c */
typedef struct manageaudioid manageaudioid_t;

manageaudioid_t *manageAudioIdAlloc (manageinfo_t *minfo);
void manageAudioIdFree (manageaudioid_t *maudioid);
uiwcont_t *manageAudioIdBuildUI (manageaudioid_t *maudioid, uisongsel_t *uisongsel);
uiwcont_t *manageAudioIDMenu (manageaudioid_t *maudioid, uiwcont_t *menubar);
void manageAudioIdMainLoop (manageaudioid_t *maudioid);
void manageAudioIdLoad (manageaudioid_t *maudioid, song_t *song, dbidx_t dbidx);
void manageAudioIdSetSaveCallback (manageaudioid_t *maudioid, callback_t *cb);
void manageAudioIdSavePosition (manageaudioid_t *maudioid);

#endif /* INC_MANAGEUI_H */
