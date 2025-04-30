/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
  xmlSetStructuredErrorFunc (NULL, xmlParseXMLErrorHandler);

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
  } else {
    logMsg (LOG_DBG, LOG_INFO, "use-namespace");
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

ilist_t *
xmlParseGetList (xmlparse_t *xmlparse, const char *xpath, const char *attr [])
{
  xmlXPathObjectPtr   xpathObj = NULL;
  xmlNodeSetPtr       nodes = NULL;
  ilist_t             *list = NULL;
  int32_t             ncount;
  int32_t             rcount = 0;

  if (xmlparse == NULL ||
      xpath == NULL ||
      xmlparse->doc == NULL ||
      xmlparse->xpathCtx == NULL) {
    return NULL;
  }

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
  ilistSetSize (list, ncount);

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

    if (xval != NULL && *xval) {
      ilistSetStr (list, rcount, XMLPARSE_VAL, (const char *) xval);
      ilistSetStr (list, rcount, XMLPARSE_NM, (const char *) cur->name);
      ++rcount;
    }
    if ((xval == NULL || ! *xval) && attr != NULL) {
      const char  **tattr = attr;

      while (*tattr != NULL) {
        xmlChar   *axval = NULL;

        axval = xmlGetProp (cur, (xmlChar *) *tattr);
        if (axval != NULL) {
          mdextalloc (axval);
          ilistSetStr (list, rcount, XMLPARSE_VAL, (const char *) axval);
          ilistSetStr (list, rcount, XMLPARSE_NM, *tattr);
          ++rcount;
          mdextfree (axval);
          xmlFree (axval);
        }
        ++tattr;
      }
    }
    mdextfree (xval);
    xmlFree (xval);
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
  // xmlMemoryDump ();

  return list;
}

static void
xmlParseXMLErrorHandler (void *udata, xmlErrorPtr xmlerr)
{
  logMsg (LOG_DBG, LOG_INFO, "line:%d col:%d err:%d %s",
      xmlerr->line, xmlerr->int2, xmlerr->code, xmlerr->message);
  return;
}
