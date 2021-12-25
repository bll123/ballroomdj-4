#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _hdr_bsd_string
# include <bsd/string.h>
#endif

#include "bdjopt.h"
#include "list.h"
#include "sysvars.h"
#include "list.h"
#include "fileutil.h"
#include "bdjstring.h"
#include "datafile.h"

static bdjoptkey_t  bdjoptGetIdx (char *);
static void         bdjoptLoad (char *);

static list_t   *bdjopt = NULL;

static bdjoptdef_t bdjoptdefs [OPT_MAX] =
{
  [OPT_ACOUSTID_CLIENT] =
    { OPTTYPE_GLOBAL, "ACOUSTID_CLIENT", VALUE_DATA },
  [OPT_ALLOWEDIT] =
    { OPTTYPE_PROFILE, "ALLOWEDIT", VALUE_DATA },
  [OPT_ARCHIVEDIR] =
    { OPTTYPE_MACHINE, "ARCHIVEDIR", VALUE_DATA },
  [OPT_audioid_match] =
    { OPTTYPE_GLOBAL, "audioid_match", VALUE_DATA },
  [OPT_AUDIOSINK] =
    { OPTTYPE_MACHINE, "AUDIOSINK", VALUE_DATA },
  [OPT_AUTOORGANIZE] =
    { OPTTYPE_GLOBAL, "AUTOORGANIZE", VALUE_DATA },
  [OPT_AUTOSTARTUP] =
    { OPTTYPE_PROFILE, "AUTOSTARTUP", VALUE_DATA },
  [OPT_bdj_analogclock] =
    { OPTTYPE_PROFILE, "bdj_analogclock", VALUE_DATA },
  [OPT_bdj_dual] =
    { OPTTYPE_PROFILE, "bdj_dual", VALUE_DATA },
  [OPT_bdj_dualmarks] =
    { OPTTYPE_PROFILE, "bdj_dualmarks", VALUE_DATA },
  [OPT_bdjedit_embedplayer] =
    { OPTTYPE_PROFILE, "bdjedit_embedplayer", VALUE_DATA },
  [OPT_bdjeditfieldlist] =
    { OPTTYPE_GLOBAL, "bdjeditfieldlist", VALUE_DATA },
  [OPT_bdjedit_hidefn] =
    { OPTTYPE_PROFILE, "bdjedit_hidefn", VALUE_DATA },
  [OPT_bdjedit_playswitch] =
    { OPTTYPE_PROFILE, "bdjedit_playswitch", VALUE_DATA },
  [OPT_bdjedit_saveadv] =
    { OPTTYPE_PROFILE, "bdjedit_saveadv", VALUE_DATA },
  [OPT_bdj_hidemarquee] =
    { OPTTYPE_PROFILE, "bdj_hidemarquee", VALUE_DATA },
  [OPT_bdj_hidemarqueeonstart] =
    { OPTTYPE_PROFILE, "bdj_hidemarqueeonstart", VALUE_DATA },
  [OPT_bdj_hidemgmt] =
    { OPTTYPE_PROFILE, "bdj_hidemgmt", VALUE_DATA },
  [OPT_bdj_hidemqcont] =
    { OPTTYPE_PROFILE, "bdj_hidemqcont", VALUE_DATA },
  [OPT_bdj_histembed] =
    { OPTTYPE_PROFILE, "bdj_histembed", VALUE_DATA },
  [OPT_bdj_imagedisp] =
    { OPTTYPE_PROFILE, "bdj_imagedisp", VALUE_DATA },
  [OPT_bdj_insrequest] =
    { OPTTYPE_PROFILE, "bdj_insrequest", VALUE_DATA },
  [OPT_bdj_lastexpcddir] =
    { OPTTYPE_MACHINE, "bdj_lastexpcddir", VALUE_DATA },
  [OPT_bdj_marqueefullscreen] =
    { OPTTYPE_PROFILE, "bdj_marqueefullscreen", VALUE_DATA },
  [OPT_bdjmm_dancecount] =
    { OPTTYPE_PROFILE, "bdjmm_dancecount", VALUE_DATA },
  [OPT_bdjmm_embed] =
    { OPTTYPE_PROFILE, "bdjmm_embed", VALUE_DATA },
  [OPT_bdjmm_hidemainmenu] =
    { OPTTYPE_PROFILE, "bdjmm_hidemainmenu", VALUE_DATA },
  [OPT_bdjmm_lastexportbdj] =
    { OPTTYPE_MACHINE, "bdjmm_lastexportbdj", VALUE_DATA },
  [OPT_bdjmm_lastexportdir] =
    { OPTTYPE_MACHINE, "bdjmm_lastexportdir", VALUE_DATA },
  [OPT_bdjmm_lastimportbdj] =
    { OPTTYPE_MACHINE, "bdjmm_lastimportbdj", VALUE_DATA },
  [OPT_bdjmm_lastimportfn] =
    { OPTTYPE_MACHINE, "bdjmm_lastimportfn", VALUE_DATA },
  [OPT_bdjmm_marksel] =
    { OPTTYPE_PROFILE, "bdjmm_marksel", VALUE_DATA },
  [OPT_bdjmm_pbcont] =
    { OPTTYPE_PROFILE, "bdjmm_pbcont", VALUE_DATA },
  [OPT_bdjmm_pbmode] =
    { OPTTYPE_PROFILE, "bdjmm_pbmode", VALUE_DATA },
  [OPT_bdjmm_playswitch] =
    { OPTTYPE_PROFILE, "bdjmm_playswitch", VALUE_DATA },
  [OPT_bdjmm_searchfieldlist] =
    { OPTTYPE_GLOBAL, "bdjmm_searchfieldlist", VALUE_DATA },
  [OPT_bdjsl_playswitch] =
    { OPTTYPE_PROFILE, "bdjsl_playswitch", VALUE_DATA },
  [OPT_bdj_tooltipdisp] =
    { OPTTYPE_GLOBAL, "bdj_tooltipdisp", VALUE_DATA },
  [OPT_bpm_selection] =
    { OPTTYPE_GLOBAL, "bpm_selection", VALUE_DATA },
  [OPT_CBFONTSIZE] =
    { OPTTYPE_MACH_PROF, "CBFONTSIZE", VALUE_DATA },
  [OPT_CHANGESPACE] =
    { OPTTYPE_GLOBAL, "CHANGESPACE", VALUE_DATA },
  [OPT_CLPATHFMT] =
    { OPTTYPE_GLOBAL, "CLPATHFMT", VALUE_DATA },
  [OPT_CLVAPATHFMT] =
    { OPTTYPE_GLOBAL, "CLVAPATHFMT", VALUE_DATA },
  [OPT_DEBUGLVL] =
    { OPTTYPE_GLOBAL, "DEBUGLVL", VALUE_DATA },
  [OPT_DEBUGON] =
    { OPTTYPE_GLOBAL, "DEBUGON", VALUE_DATA },
  [OPT_DEFAULTVOLUME] =
    { OPTTYPE_PROFILE, "DEFAULTVOLUME", VALUE_DATA },
  [OPT_DELETEDIR] =
    { OPTTYPE_MACHINE, "DELETEDIR", VALUE_DATA },
  [OPT_DONEMSG] =
    { OPTTYPE_PROFILE, "DONEMSG", VALUE_DATA },
  [OPT_ENABLEIMGPLAYER] =
    { OPTTYPE_PROFILE, "ENABLEIMGPLAYER", VALUE_DATA },
  [OPT_FADEINTIME] =
    { OPTTYPE_PROFILE, "FADEINTIME", VALUE_DATA },
  [OPT_FADEOUTTIME] =
    { OPTTYPE_PROFILE, "FADEOUTTIME", VALUE_DATA },
  [OPT_FADETYPE] =
    { OPTTYPE_PROFILE, "FADETYPE", VALUE_DATA },
  [OPT_FONTSIZE] =
    { OPTTYPE_MACH_PROF, "FONTSIZE", VALUE_DATA },
  [OPT_GAP] =
    { OPTTYPE_PROFILE, "GAP", VALUE_DATA },
  [OPT_HOST] =
    { OPTTYPE_MACHINE, "HOST", VALUE_DATA },
  [OPT_IMAGEDIR] =
    { OPTTYPE_MACHINE, "IMAGEDIR", VALUE_DATA },
  [OPT_INSTPASSWORD] =
    { OPTTYPE_MACHINE, "INSTPASSWORD", VALUE_DATA },
  [OPT_ITUNESSUPPORT] =
    { OPTTYPE_GLOBAL, "ITUNESSUPPORT", VALUE_DATA },
  [OPT_lastbackupdir] =
    { OPTTYPE_GLOBAL, "lastbackupdir", VALUE_DATA },
  [OPT_lastfmfromdir] =
    { OPTTYPE_GLOBAL, "lastfmfromdir", VALUE_DATA },
  [OPT_lastfmmusicdir] =
    { OPTTYPE_GLOBAL, "lastfmmusicdir", VALUE_DATA },
  [OPT_lastfmtodir] =
    { OPTTYPE_GLOBAL, "lastfmtodir", VALUE_DATA },
  [OPT_lastrafflegame] =
    { OPTTYPE_GLOBAL, "lastrafflegame", VALUE_DATA },
  [OPT_lastrestoredir] =
    { OPTTYPE_GLOBAL, "lastrestoredir", VALUE_DATA },
  [OPT_lastsort] =
    { OPTTYPE_GLOBAL, "lastsort", VALUE_DATA },
  [OPT_LISTINGFONTSIZE] =
    { OPTTYPE_MACH_PROF, "LISTINGFONTSIZE", VALUE_DATA },
  [OPT_LOADDANCEFROMGENRE] =
    { OPTTYPE_GLOBAL, "LOADDANCEFROMGENRE", VALUE_DATA },
  [OPT_MAXPLAYTIME] =
    { OPTTYPE_PROFILE, "MAXPLAYTIME", VALUE_DATA },
  [OPT_mmdisp] =
    { OPTTYPE_GLOBAL, "mmdisp", VALUE_DATA },
  [OPT_mmdisp_cc] =
    { OPTTYPE_GLOBAL, "mmdisp_cc", VALUE_DATA },
  [OPT_mmdisp_qp] =
    { OPTTYPE_GLOBAL, "mmdisp_qp", VALUE_DATA },
  [OPT_mmdisp_req] =
    { OPTTYPE_GLOBAL, "mmdisp_req", VALUE_DATA },
  [OPT_mmdisp_sl] =
    { OPTTYPE_GLOBAL, "mmdisp_sl", VALUE_DATA },
  [OPT_mmsldisp] =
    { OPTTYPE_GLOBAL, "mmsldisp", VALUE_DATA },
  [OPT_mmsltooltipdisp] =
    { OPTTYPE_GLOBAL, "mmsltooltipdisp", VALUE_DATA },
  [OPT_mmttdisp] =
    { OPTTYPE_GLOBAL, "mmttdisp", VALUE_DATA },
  [OPT_mmttdisp_cc] =
    { OPTTYPE_GLOBAL, "mmttdisp_cc", VALUE_DATA },
  [OPT_mmttdisp_qp] =
    { OPTTYPE_GLOBAL, "mmttdisp_qp", VALUE_DATA },
  [OPT_mmttdisp_req] =
    { OPTTYPE_GLOBAL, "mmttdisp_req", VALUE_DATA },
  [OPT_mmttdisp_sl] =
    { OPTTYPE_GLOBAL, "mmttdisp_sl", VALUE_DATA },
  [OPT_MOBILEMARQUEE] =
    { OPTTYPE_PROFILE, "MOBILEMARQUEE", VALUE_DATA },
  [OPT_MOBILEMQPORT] =
    { OPTTYPE_PROFILE, "MOBILEMQPORT", VALUE_DATA },
  [OPT_MOBILEMQTAG] =
    { OPTTYPE_PROFILE, "MOBILEMQTAG", VALUE_DATA },
  [OPT_MOBILEMQTITLE] =
    { OPTTYPE_PROFILE, "MOBILEMQTITLE", VALUE_DATA },
  [OPT_MQCLOCKFONTSIZE] =
    { OPTTYPE_PROFILE, "MQCLOCKFONTSIZE", VALUE_DATA },
  [OPT_MQDANCEFONT] =
    { OPTTYPE_PROFILE, "MQDANCEFONT", VALUE_DATA },
  [OPT_MQDANCEFONTMULT] =
    { OPTTYPE_PROFILE, "MQDANCEFONTMULT", VALUE_DATA },
  [OPT_MQFONT] =
    { OPTTYPE_PROFILE, "MQFONT", VALUE_DATA },
  [OPT_MQFULLSCREEN] =
    { OPTTYPE_PROFILE, "MQFULLSCREEN", VALUE_DATA },
  [OPT_mqheight] =
    { OPTTYPE_GLOBAL, "mqheight", VALUE_DATA },
  [OPT_MQQLEN] =
    { OPTTYPE_PROFILE, "MQQLEN", VALUE_DATA },
  [OPT_mqSize] =
    { OPTTYPE_GLOBAL, "mqSize", VALUE_DATA },
  [OPT_mqwidth] =
    { OPTTYPE_GLOBAL, "mqwidth", VALUE_DATA },
  [OPT_MTMPDIR] =
    { OPTTYPE_MACHINE, "MTMPDIR", VALUE_DATA },
  [OPT_MUSICDIR] =
    { OPTTYPE_MACHINE, "MUSICDIR", VALUE_DATA },
  [OPT_MUSICDIRDFLT] =
    { OPTTYPE_GLOBAL, "MUSICDIRDFLT", VALUE_DATA },
  [OPT_ORIGINALDIR] =
    { OPTTYPE_MACHINE, "ORIGINALDIR", VALUE_DATA },
  [OPT_PATHFMT] =
    { OPTTYPE_GLOBAL, "PATHFMT", VALUE_DATA },
  [OPT_PAUSEMSG] =
    { OPTTYPE_PROFILE, "PAUSEMSG", VALUE_DATA },
  [OPT_PLAYER] =
    { OPTTYPE_GLOBAL, "PLAYER", VALUE_DATA },
  [OPT_PLAYEROPTIONS] =
    { OPTTYPE_MACH_PROF, "PLAYEROPTIONS", VALUE_DATA },
  [OPT_PLAYERQLEN0] =
    { OPTTYPE_GLOBAL, "PLAYERQLEN0", VALUE_DATA },
  [OPT_PLAYERQLEN1] =
    { OPTTYPE_GLOBAL, "PLAYERQLEN1", VALUE_DATA },
  [OPT_PLAYERSHUTDOWNSCRIPT] =
    { OPTTYPE_MACH_PROF, "PLAYERSHUTDOWNSCRIPT", VALUE_DATA },
  [OPT_PLAYERSTARTSCRIPT] =
    { OPTTYPE_MACH_PROF, "PLAYERSTARTSCRIPT", VALUE_DATA },
  [OPT_PROFILENAME] =
    { OPTTYPE_PROFILE, "PROFILENAME", VALUE_DATA },
  [OPT_QUEUENAME0] =
    { OPTTYPE_PROFILE, "QUEUENAME0", VALUE_DATA },
  [OPT_QUEUENAME1] =
    { OPTTYPE_PROFILE, "QUEUENAME1", VALUE_DATA },
  [OPT_QUICKPLAYENABLED] =
    { OPTTYPE_PROFILE, "QUICKPLAYENABLED", VALUE_DATA },
  [OPT_QUICKPLAYSHOW] =
    { OPTTYPE_PROFILE, "QUICKPLAYSHOW", VALUE_DATA },
  [OPT_REMCONTROLHTML] =
    { OPTTYPE_GLOBAL, "REMCONTROLHTML", VALUE_DATA },
  [OPT_REMCONTROLPASS] =
    { OPTTYPE_PROFILE, "REMCONTROLPASS", VALUE_DATA },
  [OPT_REMCONTROLPORT] =
    { OPTTYPE_PROFILE, "REMCONTROLPORT", VALUE_DATA },
  [OPT_REMCONTROLSHOWDANCE] =
    { OPTTYPE_PROFILE, "REMCONTROLSHOWDANCE", VALUE_DATA },
  [OPT_REMCONTROLSHOWSONG] =
    { OPTTYPE_PROFILE, "REMCONTROLSHOWSONG", VALUE_DATA },
  [OPT_REMCONTROLUSER] =
    { OPTTYPE_PROFILE, "REMCONTROLUSER", VALUE_DATA },
  [OPT_REMOTECONTROL] =
    { OPTTYPE_PROFILE, "REMOTECONTROL", VALUE_DATA },
  [OPT_samesongkey] =
    { OPTTYPE_GLOBAL, "samesongkey", VALUE_DATA },
  [OPT_SERVERNAME] =
    { OPTTYPE_PROFILE, "SERVERNAME", VALUE_DATA },
  [OPT_SERVERPASS] =
    { OPTTYPE_PROFILE, "SERVERPASS", VALUE_DATA },
  [OPT_SERVERPORT] =
    { OPTTYPE_PROFILE, "SERVERPORT", VALUE_DATA },
  [OPT_SERVERTYPE] =
    { OPTTYPE_PROFILE, "SERVERTYPE", VALUE_DATA },
  [OPT_SERVERUSER] =
    { OPTTYPE_PROFILE, "SERVERUSER", VALUE_DATA },
  [OPT_SHOWALBUM] =
    { OPTTYPE_GLOBAL, "SHOWALBUM", VALUE_DATA },
  [OPT_SHOWBPM] =
    { OPTTYPE_GLOBAL, "SHOWBPM", VALUE_DATA },
  [OPT_SHOWCLASSICAL] =
    { OPTTYPE_GLOBAL, "SHOWCLASSICAL", VALUE_DATA },
  [OPT_SHOWSTATUS] =
    { OPTTYPE_GLOBAL, "SHOWSTATUS", VALUE_DATA },
  [OPT_SHUTDOWNSCRIPT] =
    { OPTTYPE_GLOBAL, "SHUTDOWNSCRIPT", VALUE_DATA },
  [OPT_SLOWDEVICE] =
    { OPTTYPE_GLOBAL, "SLOWDEVICE", VALUE_DATA },
  [OPT_slstatsdur] =
    { OPTTYPE_MACH_PROF, "slstatsdur", VALUE_DATA },
  [OPT_STARTMAXIMIZED] =
    { OPTTYPE_GLOBAL, "STARTMAXIMIZED", VALUE_DATA },
  [OPT_STARTUPSCRIPT] =
    { OPTTYPE_MACHINE, "STARTUPSCRIPT", VALUE_DATA },
  [OPT_UIBGCOLOR] =
    { OPTTYPE_PROFILE, "UIBGCOLOR", VALUE_DATA },
  [OPT_UIFIXEDFONT] =
    { OPTTYPE_MACH_PROF, "UIFIXEDFONT", VALUE_DATA },
  [OPT_UIFONT] =
    { OPTTYPE_MACH_PROF, "UIFONT", VALUE_DATA },
  [OPT_UITHEME] =
    { OPTTYPE_PROFILE, "UITHEME", VALUE_DATA },
  [OPT_VAPATHFMT] =
    { OPTTYPE_GLOBAL, "VAPATHFMT", VALUE_DATA },
  [OPT_VARIOUS] =
    { OPTTYPE_GLOBAL, "VARIOUS", VALUE_DATA },
  [OPT_version] =
    { OPTTYPE_GLOBAL, "version", VALUE_DATA },
  [OPT_WRITETAGS] =
    { OPTTYPE_GLOBAL, "WRITETAGS", VALUE_DATA }
};

static list_t *bdjoptlookup = NULL;

void
bdjoptInit (void)
{
  char      path [MAXPATHLEN];
  listkey_t lkey;

  bdjoptlookup = vlistAlloc (KEY_STR, LIST_UNORDERED, stringCompare, NULL, NULL);
  vlistSetSize (bdjoptlookup, OPT_MAX);
  for (size_t i = 0; i < OPT_MAX; ++i) {
    lkey.name = bdjoptdefs[i].optname;
    vlistSetLong (bdjoptlookup, lkey, (long) i);
  }
  vlistSort (bdjoptlookup);

  bdjopt = vlistAlloc (KEY_LONG, LIST_ORDERED, stringCompare, NULL, free);

  /* global */
  strlcpy (path, "data/", MAXPATHLEN);
  strlcat (path, BDJ_CONFIG_BASEFN, MAXPATHLEN);
  bdjoptLoad (path);

  /* per machine */
  strlcpy (path, "data/", MAXPATHLEN);
  strlcat (path, sysvars [SV_HOSTNAME], MAXPATHLEN);
  strlcat (path, "/", MAXPATHLEN);
  strlcat (path, BDJ_CONFIG_BASEFN, MAXPATHLEN);
  bdjoptLoad (path);

  /* profile */
  strlcpy (path, "data/profiles/", MAXPATHLEN);
  strlcat (path, BDJ_CONFIG_BASEFN, MAXPATHLEN);
  bdjoptLoad (path);

  /* per machine per profile */
  strlcpy (path, "data/", MAXPATHLEN);
  strlcat (path, sysvars [SV_HOSTNAME], MAXPATHLEN);
  strlcat (path, "/profiles/", MAXPATHLEN);
  strlcat (path, BDJ_CONFIG_BASEFN, MAXPATHLEN);
  bdjoptLoad (path);
}

void
bdjoptFree (void)
{
  if (bdjopt != NULL) {
    free (bdjopt);
  }
}

/* internal routines */

static bdjoptkey_t
bdjoptGetIdx (char *keynm)
{
  listkey_t   lkey;
  bdjoptkey_t idx;

  lkey.name = keynm;
  idx = (bdjoptkey_t) vlistGetLong (bdjoptlookup, lkey);
  return idx;
}

static void
bdjoptLoad (char *path)
{
  char          *data;
  parseinfo_t   *pi;
  char          **strdata;
  size_t        count;
  listkey_t     lkey;


  if (lsysvars [SVL_BDJIDX] != 0L) {
    char      temp [10];
    snprintf (temp, sizeof (temp), "-%ld\n", lsysvars [SVL_BDJIDX]);
    strlcat (path, temp, MAXPATHLEN);
  }
  strlcat (path, BDJ_CONFIG_EXT, MAXPATHLEN);

  data = fileReadAll (path);
  pi = parseInit ();
  count = parseKeyValue (pi, data);
  strdata = parseGetData (pi);
  for (size_t i = 0; i < count; i += 2) {
    bdjoptkey_t idx = bdjoptGetIdx (strdata [i]);
    if (idx >= OPT_MAX) {
      /* unused option key, ignore for now */
      continue;
    }
    lkey.key = idx;

    switch (bdjoptdefs [idx].valuetype) {
      case VALUE_DOUBLE:
      {
        vlistSetDouble (bdjopt, lkey, atof (strdata [i+1]));
        break;
      }
      case VALUE_LONG:
      {
        vlistSetLong (bdjopt, lkey, atol (strdata [i+1]));
        break;
      }
      case VALUE_DATA:
      {
        vlistSetData (bdjopt, lkey, strdup (strdata [i+1]));
        break;
      }
    }
  }
  parseFree (pi);
}
