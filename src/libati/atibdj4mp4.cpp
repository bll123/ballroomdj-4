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
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

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
          valuea = ntohs (tlong);
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
  return -1;
}

} /* extern C */
