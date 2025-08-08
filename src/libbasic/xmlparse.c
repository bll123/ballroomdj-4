/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "filedata.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "xmlparse.h"

typedef struct xmlparse {
  xmlDocPtr           doc;
  xmlXPathContextPtr  xpathCtx;
  char                *tdata;
} xmlparse_t;

static void xmlParseXMLErrorHandler (void *udata, xmlErrorPtr xmlerr);
static void xmlParseRegisterNamespaces (xmlparse_t *xmlparse);

NODISCARD
xmlparse_t *
xmlParseInitFile (const char *fname, int nsflag)
{
  xmlparse_t    *xmlparse = NULL;
  char          *data;
  size_t        datalen;

  logMsg (LOG_DBG, LOG_INFO, "init-file: %s", fname);
  data = filedataReadAll (fname, &datalen);
  if (data == NULL) {
    logMsg (LOG_DBG, LOG_INFO, "unable to read data file");
    return xmlparse;
  }

  xmlparse = xmlParseInitData (data, datalen, nsflag);
  dataFree (data);

  return xmlparse;
}

NODISCARD
xmlparse_t *
xmlParseInitData (const char *data, size_t datalen, int nsflag)
{
  xmlparse_t    *xmlparse;
  char          *p;

  xmlparse = mdmalloc (sizeof (xmlparse_t));
  xmlparse->doc = NULL;
  xmlparse->xpathCtx = NULL;
  xmlparse->tdata = NULL;

  xmlInitParser ();
  xmlSetStructuredErrorFunc (NULL,
      (xmlStructuredErrorFunc) xmlParseXMLErrorHandler);

  xmlparse->tdata = mdmalloc (datalen + 1);
  memcpy (xmlparse->tdata, data, datalen);
  xmlparse->tdata [datalen] = '\0';

  if (nsflag == XMLPARSE_NONS) {
    logMsg (LOG_DBG, LOG_INFO, "no-namespace");
    /* libxml2 doesn't have any way to set the default namespace for xpath */
    /* which makes it a pain to use when a namespace is set */
    /* clear out the namespace */

    p = strstr (xmlparse->tdata, "xmlns");
    if (p != NULL) {
      char    *pe;
      size_t  len;

      pe = strstr (p, ">");
      len = pe - p;
      memset (p, ' ', len);
    }
  }

  xmlparse->doc = xmlParseMemory (xmlparse->tdata, datalen);
  if (xmlparse->doc == NULL) {
    logMsg (LOG_DBG, LOG_INFO, "unable to parse xml doc");
    return xmlparse;
  }
  mdextalloc (xmlparse->doc);

  xmlparse->xpathCtx = xmlXPathNewContext (xmlparse->doc);
  if (xmlparse->xpathCtx == NULL) {
    logMsg (LOG_DBG, LOG_INFO, "unable to create xpath context");
    return xmlparse;
  }
  mdextalloc (xmlparse->xpathCtx);

  if (nsflag == XMLPARSE_USENS) {
    logMsg (LOG_DBG, LOG_INFO, "use-namespace");

    xmlParseRegisterNamespaces (xmlparse);
  }

  return xmlparse;
}

void
xmlParseFree (xmlparse_t *xmlparse)
{
  if (xmlparse == NULL) {
    return;
  }

  if (xmlparse->xpathCtx != NULL) {
    mdextfree (xmlparse->xpathCtx);
    xmlXPathFreeContext (xmlparse->xpathCtx);
  }
  if (xmlparse->doc != NULL) {
    mdextfree (xmlparse->doc);
    xmlFreeDoc (xmlparse->doc);
  }
  dataFree (xmlparse->tdata);

  mdfree (xmlparse);
  xmlCleanupParser ();
}

void
xmlParseGetItem (xmlparse_t *xmlparse, const char *xpath,
    char *buff, size_t sz)
{
  xmlXPathObjectPtr   xpathObj = NULL;
  xmlNodeSetPtr       nodes = NULL;
  xmlNodePtr          cur = NULL;
  xmlChar             *xval = NULL;

  if (xmlparse == NULL ||
      xpath == NULL ||
      xmlparse->doc == NULL ||
      xmlparse->xpathCtx == NULL) {
    return;
  }

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpath, xmlparse->xpathCtx);
  mdextalloc (xpathObj);
  nodes = xpathObj->nodesetval;
  if (nodes->nodeNr > 0) {
    cur = nodes->nodeTab [0];
    xval = xmlNodeGetContent (cur);
    mdextalloc (xval);
    if (xval != NULL) {
      stpecpy (buff, buff + sz, (char *) xval);
      mdextfree (xval);
      xmlFree (xval);
    }
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
}

NODISCARD
ilist_t *
xmlParseGetList (xmlparse_t *xmlparse, const char *xpath,
    const xmlparseattr_t attr [])
{
  xmlXPathObjectPtr   xpathObj = NULL;
  xmlNodeSetPtr       nodes = NULL;
  ilist_t             *list = NULL;
  int32_t             ncount;
  int32_t             rcount = 0;
  char                first [80];

  if (xmlparse == NULL ||
      xpath == NULL ||
      xmlparse->doc == NULL ||
      xmlparse->xpathCtx == NULL) {
    return NULL;
  }

  *first = '\0';
  list = ilistAlloc ("xmlparse", LIST_ORDERED);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpath, xmlparse->xpathCtx);
  mdextalloc (xpathObj);
  nodes = xpathObj->nodesetval;
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    mdextfree (xpathObj);
    xmlXPathFreeObject (xpathObj);
    return list;
  }

  ncount = nodes->nodeNr;

  for (int32_t i = 0; i < ncount; ++i)  {
    xmlNodePtr    cur = NULL;
    xmlChar       *xval = NULL;

    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    cur = nodes->nodeTab [i];
    xval = xmlNodeGetContent (cur);
    if (xval == NULL) {
      continue;
    }
    mdextalloc (xval);

    if (strcmp (first, (const char *) cur->name) == 0) {
      ++rcount;
    }

    /* messy stuff, as there is more than one attribute */
    /* for some xpath items */
    if (attr != NULL) {
      const xmlparseattr_t    *attrp = attr;

      while (attrp->name != NULL) {
        if (! *xval) {
          xmlChar   *axval = NULL;

          if (attrp->attr == NULL) {
            ++attrp;
            continue;
          }

          axval = xmlGetProp (cur, (xmlChar *) attrp->attr);
          if (axval != NULL) {
            mdextalloc (axval);
            ilistSetStr (list, rcount, attrp->idx, (const char *) axval);
            // fprintf (stderr, "%d %d %s\n", rcount, attrp->idx, (const char *) axval);
            mdextfree (axval);
            xmlFree (axval);
          }
        } else if (strcmp (attrp->name, (const char *) cur->name) == 0) {
          ilistSetStr (list, rcount, attrp->idx, (const char *) xval);
          // fprintf (stderr, "%d %d %s\n", rcount, attrp->idx, (const char *) xval);
          break;
        }
        ++attrp;
      }

      if (! *first) {
        stpecpy (first, first + sizeof (first), (const char *) cur->name);
      }
    } else {
      ilistSetStr (list, rcount, XMLPARSE_VAL, (const char *) xval);
      ilistSetStr (list, rcount, XMLPARSE_NM, (const char *) cur->name);
      // fprintf (stderr, "%d %s %s\n", rcount, (const char *) cur->name, (const char *) xval);
      ++rcount;
    }
    mdextfree (xval);
    xmlFree (xval);
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
  // xmlMemoryDump ();

  return list;
}

/* internal routines */

static void
xmlParseRegisterNamespaces (xmlparse_t *xmlparse)
{
  char    *p;
  char    *tp;
  char    ns [40];
  char    uri [1024];

  /* xmlns:itunes="http://www.itunes.com/dtds/podcast-1.0.dtd" */
  p = xmlparse->tdata;
  p = strstr (p, "xmlns:");
  while (p != NULL) {
    p += 6;   // xmlns:
    tp = strstr (p, "=");
    if (tp != NULL) {
      *tp = '\0';
      stpecpy (ns, ns + sizeof (ns), p);
      *tp = '=';
      p = tp + 2;
      tp = strstr (p, "\"");
      if (tp != NULL) {
        *tp = '\0';
        stpecpy (uri, uri + sizeof (uri), p);
        *tp = '"';
        xmlXPathRegisterNs (xmlparse->xpathCtx, (xmlChar *) ns, (xmlChar *) uri);
        p = tp + 1;
      }
    }
    p = strstr (p, "xmlns:");
  }
}

static void
xmlParseXMLErrorHandler (void *udata, xmlErrorPtr xmlerr)
{
  logMsg (LOG_DBG, LOG_INFO, "line:%d col:%d err:%d %s",
      xmlerr->line, xmlerr->int2, xmlerr->code, xmlerr->message);
  return;
}
