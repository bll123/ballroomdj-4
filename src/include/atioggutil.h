#ifndef INC_ATIOGGUTIL_H
#define INC_ATIOGGUTIL_H

#include "ati.h"
#include "slist.h"

void atioggProcessVorbisCommentCombined (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *kw);
void atioggProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *tag, const char *val);
const char * atioggParseVorbisComment (const char *kw, char *buff, size_t sz);
slist_t *atioggSplitVorbisComment (int tagkey, const char *tagname, const char *val);
int   atioggCheckCodec (const char *ffn, int filetype);
int   atioggWriteOggFile (const char *ffn, void *newvc, int filetype);

#endif /* INC_ATIOGGUTIL_H */