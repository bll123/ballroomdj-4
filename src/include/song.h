#ifndef INC_SONG
#define INC_SONG

#include "datafile.h"
#include "nlist.h"
#include "slist.h"
#include "songfav.h"

typedef struct song song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songParse (song_t *song, char *data, ssize_t didx);
char *    songGetStr (song_t *, nlistidx_t);
ssize_t   songGetNum (song_t *, nlistidx_t);
double    songGetDouble (song_t *, nlistidx_t);
slist_t * songGetList (song_t *song, nlistidx_t idx);
songfavoriteinfo_t  * songGetFavoriteData (song_t *);
void      songSetNum (song_t *, nlistidx_t, ssize_t val);
void      songSetDouble (song_t *, nlistidx_t, double val);
void      songSetStr (song_t *song, nlistidx_t tagidx, const char *str);
void      songSetList (song_t *song, nlistidx_t tagidx, const char *str);
void      songChangeFavorite (song_t *song);
bool      songAudioFileExists (song_t *song);
char *    songDisplayString (song_t *song, int tagidx);
slist_t * songTagList (song_t *song);
bool      songIsChanged (song_t *song);
bool      songHasSonglistChange (song_t *song);
void      songClearChanged (song_t *song);

#endif /* INC_SONG */
