#include <stdlib.h>
#include <libintl.h>

#include "tagdef.h"
#include "bdjstring.h"

void
tagdefInit (void)
{
  tagdefs [TAG_KEY_AFMODTIME].label = _("Audio File Date");
  tagdefs [TAG_KEY_ALBUM].label = _("Album");
  tagdefs [TAG_KEY_ALBUMARTIST].label = _("Album Artist");
  tagdefs [TAG_KEY_ARTIST].label = _("Artist");
  tagdefs [TAG_KEY_COMPOSER].label = _("Composer");
  tagdefs [TAG_KEY_CONDUCTOR].label = _("Conductor");
  tagdefs [TAG_KEY_DANCE].label = _("Dance");
  tagdefs [TAG_KEY_DANCELEVEL].label = _("Level");
  tagdefs [TAG_KEY_DANCERATING].label = _("Dance Rating");
  tagdefs [TAG_KEY_DANCERATING].shortLabel = _("Rating");
  tagdefs [TAG_KEY_DATE].label = _("Date");
  tagdefs [TAG_KEY_DBADDDATE].label = _("Date Added");
  tagdefs [TAG_KEY_DISCNUMBER].label = _("Disc");
  tagdefs [TAG_KEY_DISCTOTAL].label = _("Total Discs");
  tagdefs [TAG_KEY_DISPLAYIMG].label = _("Image");
  tagdefs [TAG_KEY_DURATION].label = _("Duration");
  tagdefs [TAG_KEY_DURATION_STR].label = _("Duration");
  tagdefs [TAG_KEY_GENRE].label = _("Genre");
  tagdefs [TAG_KEY_KEYWORD].label = _("Keyword");
  tagdefs [TAG_KEY_MQDISPLAY].label = _("Marquee Display");
  tagdefs [TAG_KEY_NOMAXPLAYTIME].label = _("No Maximum Play Time");
  tagdefs [TAG_KEY_NOMAXPLAYTIME].shortLabel = _("No Max");
  tagdefs [TAG_KEY_NOTES].label = _("Notes");
  tagdefs [TAG_KEY_SONGEND].label = _("Song End");
  tagdefs [TAG_KEY_SONGSTART].label = _("Song Start");
  tagdefs [TAG_KEY_SPEEDADJUSTMENT].label = _("Speed Adjustment");
  tagdefs [TAG_KEY_SPEEDADJUSTMENT].shortLabel = _("Spd. Adj.");
  tagdefs [TAG_KEY_STATUS].label = _("Status");
  tagdefs [TAG_KEY_TAGS].label = _("Tags");
  tagdefs [TAG_KEY_TITLE].label = _("Title");
  tagdefs [TAG_KEY_TRACKNUMBER].label = _("Track Number");
  tagdefs [TAG_KEY_TRACKNUMBER].shortLabel = _("Track");
  tagdefs [TAG_KEY_TRACKTOTAL].label = _("Total Tracks");
  tagdefs [TAG_KEY_VARIOUSARTISTS].label = _("Various Artists");
  tagdefs [TAG_KEY_VOLUMEADJUSTPERC].label = _("Adjustment");
  tagdefs [TAG_KEY_VOLUMEADJUSTPERC].shortLabel = _("Vol. Adj.");
}
