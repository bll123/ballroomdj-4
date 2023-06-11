/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

/* need ntohs/ntohl */
#if _hdr_arpa_inet
# include <arpa/inet.h>
#endif

#include <bento4/Ap4.h>

extern "C" {

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

#define MP4_A9_SYMBOL "©"
#define MP4_A9_SYMBOL_LEN (strlen (MP4_A9_SYMBOL))

static int atibdj4AddMP4Tag (AP4_File *file, int tagkey, const char *tag, const char *base, const char *desc, const char *val);
static void atibdj4RemoveMP4Tag (AP4_File *file, const char *tag, const char *base, const char *desc);

void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  AP4_ByteStream*     input     = NULL;
  AP4_File*           file      = NULL;
  AP4_Result          result    = AP4_SUCCESS;
  const AP4_MetaData  *metadata;
  AP4_List<AP4_MetaData::Entry>::Item*  item;

  result = AP4_FileByteStream::Create (ffn,
      AP4_FileByteStream::STREAM_MODE_READ, input);
  file = new AP4_File (*input);

  metadata = file->GetMetaData ();
  item = metadata->GetEntries ().FirstItem ();
  while (item) {
    AP4_MetaData::Entry*  entry;
    char                  *key_name;
    AP4_Atom::Type        atom_type;
    bool                  shownamespace;
    char                  fullname [100];
    char                  tvalue [300];
    const char            *tagname;
    int                   type;
    const char            *value;
    char                  pbuff [300];

    shownamespace = true;
    entry = item->GetData ();
    key_name = (char *) entry->m_Key.GetName ().GetChars ();
    atom_type = AP4_Atom::TypeFromString (key_name);

    /* there's no need to search the list for a custom tag */
    if (AP4_MetaData::KeyInfos [0].four_cc != AP4_ATOM_TYPE_dddd) {
      for (unsigned int i = 0; i < AP4_MetaData::KeyInfos.ItemCount (); ++i) {
        if (AP4_MetaData::KeyInfos [i].four_cc == atom_type) {
          char  disp [20];
          int   j;

          memset (disp, '\0', sizeof (disp));
          j = 0;
          if (*key_name == '\xa9') {
            /* if present, translate the \xa9 to utf8 copyright symbol */
            strlcat (disp, "\xc2\xa9", sizeof (disp));
            j += 2;
          } else {
            *disp = *key_name;
            ++j;
          }
          for (int i = 1; i < 4; ++i) {
            disp [j] = key_name [i];
            ++j;
          }
          key_name = mdstrdup (disp);
          shownamespace = false;
          break;
        } /* found atom */
      } /* for each atom type */
    } /* if not a custom type */

    if (shownamespace) {
      /* this is the same display format that mutagen-inspect uses */
      snprintf (fullname, sizeof (fullname),
          "----:%s:%s", entry->m_Key.GetNamespace ().GetChars (), key_name);
    } else {
      strlcpy (fullname, key_name, sizeof (fullname));
    }

    tagname = atidata->tagLookup (tagtype, fullname);
    if (tagname == NULL) {
      tagname = fullname;
    }

    type = entry->m_Value->GetType ();
    {
      AP4_DataBuffer  buffer;
      int16_t         tshort;
      int32_t         tlong;
      struct {
        int32_t       tlong;
        int16_t       tshort;
        char          padding [6];  // track number is 8 bytes
      } tdual;
      long            valuea;
      long            valueb;

      if (type == AP4_MetaData::Value::TYPE_BINARY) {
        if (! AP4_FAILED (entry->m_Value->ToBytes (buffer))) {
          /* weird, they put the larger number into the shorter field */
          if (buffer.GetDataSize () > sizeof (tdual)) {
            fprintf (stderr, "fail: mp4 binary field too long to parse\n");
          } else {
            memcpy (&tdual, buffer.GetData (), buffer.GetDataSize ());
            valuea = ntohl (tdual.tlong);
            valueb = ntohs (tdual.tshort);
            snprintf (tvalue, sizeof (tvalue), "%ld/%ld", valuea, valueb);
          }
        }
      } else if (type == AP4_MetaData::Value::TYPE_INT_08_BE) {
        if (! AP4_FAILED (entry->m_Value->ToBytes (buffer))) {
          memcpy (&tvalue, buffer.GetData (), buffer.GetDataSize ());
          valuea = *tvalue;
          snprintf (tvalue, sizeof (tvalue), "%ld", valuea);
        }
      } else if (type == AP4_MetaData::Value::TYPE_INT_16_BE) {
        if (! AP4_FAILED (entry->m_Value->ToBytes (buffer))) {
          memcpy (&tshort, buffer.GetData (), buffer.GetDataSize ());
          valuea = ntohs (tshort);
          snprintf (tvalue, sizeof (tvalue), "%ld", valuea);
        }
      } else if (type == AP4_MetaData::Value::TYPE_INT_32_BE) {
        if (! AP4_FAILED (entry->m_Value->ToBytes (buffer))) {
          memcpy (&tlong, buffer.GetData (), buffer.GetDataSize ());
          valuea = ntohl (tlong);
          snprintf (tvalue, sizeof (tvalue), "%ld", valuea);
        }
      }
      value = tvalue;
    }

    if (type == AP4_MetaData::Value::TYPE_STRING_UTF_8) {
      strlcpy (tvalue, entry->m_Value->ToString ().GetChars (), sizeof (tvalue));
      value = tvalue;
    }
    if (type == AP4_MetaData::Value::TYPE_STRING_UTF_16) {
      fprintf (stderr, "fail: %s %s utf-16 not handled\n", tagname, fullname);
    }
    if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
      value = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
          value, pbuff, sizeof (pbuff));
    }
    if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
      value = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
          value, pbuff, sizeof (pbuff));
    }

    slistSetStr (tagdata, tagname, value);

    if (! shownamespace) {
      mdfree (key_name);
    }

    item = item->GetNext();
  }

  delete file;
  delete input;
}

int
atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  AP4_ByteStream    *input = NULL;
  AP4_File          *file = NULL;
  AP4_ByteStream    *outfile = NULL;
  AP4_Result        result = AP4_SUCCESS;
  slistidx_t        iteridx;
  const char        *tag;
  char              outfn [MAXPATHLEN];
  int               rc;

  result = AP4_FileByteStream::Create (ffn,
      AP4_FileByteStream::STREAM_MODE_READ, input);
  file = new AP4_File (*input);

  slistStartIterator (dellist, &iteridx);
  while ((tag = slistIterateKey (dellist, &iteridx)) != NULL) {
    int                 tagkey;
    const tagaudiotag_t *audiotag;

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: del: %s", tag);
    tagkey = atidata->tagCheck (atidata->writetags, tagtype, tag, AF_REWRITE_NONE);
    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    atibdj4RemoveMP4Tag (file, audiotag->tag, audiotag->base, audiotag->desc);
  }
  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    int                 tagkey;
    const tagaudiotag_t *audiotag;

    tagkey = atidata->tagCheck (atidata->writetags, tagtype, tag, AF_REWRITE_NONE);
    if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_TRACKTOTAL ||
        tagkey == TAG_DISCNUMBER || tagkey == TAG_DISCTOTAL) {
      /* these will be re-worked later */
      continue;
    }
    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    atibdj4RemoveMP4Tag (file, audiotag->tag, audiotag->base, audiotag->desc);
    atibdj4AddMP4Tag (file, tagkey, audiotag->tag, audiotag->base, audiotag->desc,
        slistGetStr (updatelist, tag));
  }

  snprintf (outfn, sizeof (outfn), "%s.m4a-tmp", ffn);
  result = AP4_FileByteStream::Create (outfn,
      AP4_FileByteStream::STREAM_MODE_WRITE, outfile);
  if (AP4_FAILED(result)) {
    fprintf(stderr, "ERROR: cannot open output file for writing\n");
  } else {
    AP4_FileCopier::Write(*file, *outfile);
    delete outfile;
  }

  delete file;
  delete input;

  rc = atiReplaceFile (ffn, outfn);

  return rc;
}

static int
atibdj4AddMP4Tag (AP4_File *file, int tagkey, const char *tag, const char *base, const char *desc, const char *val)
{
  AP4_MetaData::Value *vobj = NULL;
  AP4_MetaData::Entry *entry = NULL;
  AP4_Result          result;

  if (base == NULL) {
    long    nval;
    char    ttag [5];
    int     i, j, len;

    if (tagkey == TAG_BPM) {
      nval = atol (val);
      vobj = new AP4_IntegerMetaDataValue (AP4_MetaData::Value::TYPE_INT_16_BE, nval);
    } else {
      vobj = new AP4_StringMetaDataValue (val);
    }

    /* convert the four character display tag to the internal tag value */
    /* i is the index into the display tag */
    /* j is the index into the internal tag name */
    memset (ttag, '\0', sizeof (ttag));
    i = 0;
    j = 0;
    if (strncmp (tag, MP4_A9_SYMBOL, MP4_A9_SYMBOL_LEN) == 0) {
      ttag [j++] = 0xa9;
      i += MP4_A9_SYMBOL_LEN;
    }
    len = strlen (tag);
    for ( ; i < len; ++i) {
      ttag [j++] = tag [i];
    }
    entry = new AP4_MetaData::Entry (ttag, "meta", vobj);
  } else {
    vobj = new AP4_StringMetaDataValue (val);
    entry = new AP4_MetaData::Entry (desc, base, vobj);
  }

  result = entry->AddToFile (*file, 0);
  if (AP4_FAILED (result)) {
    fprintf(stderr, "ERROR: cannot add entry %s (%d:%s)\n",
        tag, result, AP4_ResultText (result));
  }

  delete entry;
  return 0;
}

static void
atibdj4RemoveMP4Tag (AP4_File *file, const char *tag, const char *base, const char *desc)
{
  AP4_String      *ap4key;
  AP4_String          *key_namespace = NULL;
  AP4_String          *key_name = NULL;
  //AP4_Ordinal       key_index = 0;
  AP4_MetaData::Entry *entry;


  ap4key = new AP4_String (tag);
  if (base != NULL) {
    key_namespace = new AP4_String (base);
    key_name = new AP4_String (desc);
  } else {
    key_namespace = new AP4_String ("meta");
    key_name = new AP4_String (tag);
  }

  /* for the moment, ignore key-index, but it will need to be parsed later */
  entry = new AP4_MetaData::Entry (key_name->GetChars(), key_namespace->GetChars(), NULL);
  entry->RemoveFromFile (*file, 0);

  delete ap4key;
  delete key_namespace;
  delete key_name;
  delete entry;
}


} /* extern C */
