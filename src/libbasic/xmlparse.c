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

xmlparse_t *
xmlParseInit (const char *fname)
{
  xmlparse_t    *xmlparse;
  char          *data;
  size_t        datalen;
  char          *p;

  xmlparse = mdmalloc (sizeof (xmlparse_t));
  xmlparse->doc = NULL;
  xmlparse->xpathCtx = NULL;
  xmlparse->tdata = NULL;

  xmlInitParser ();

  data = filedataReadAll (fname, &datalen);
  if (data == NULL) {
    logMsg (LOG_DBG, LOG_INFO, "unable to read data file");
    return xmlparse;
  }

  /* libxml2 doesn't have any way to set the default namespace for xpath */
  /* which makes it a pain to use when a namespace is set */
  /* clear out the namespace */
  xmlparse->tdata = mdmalloc (datalen + 1);
  memcpy (xmlparse->tdata, data, datalen);
  xmlparse->tdata [datalen] = '\0';

  p = strstr (xmlparse->tdata, "xmlns");
  if (p != NULL) {
    char    *pe;
    size_t  len;

    pe = strstr (p, ">");
    len = pe - p;
    memset (p, ' ', len);
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

  if (xmlparse == NULL || xpath == NULL) {
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
xmlParseGetList (xmlparse_t *xmlparse, const char *xpath)
{
  xmlXPathObjectPtr   xpathObj = NULL;
  xmlNodeSetPtr       nodes = NULL;
  ilist_t             *list = NULL;
  int32_t             count;

  if (xmlparse == NULL || xpath == NULL) {
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

  count = nodes->nodeNr;
  ilistSetSize (list, count);

  for (int32_t i = 0; i < count; ++i)  {
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

    ilistSetStr (list, i, XMLPARSE_VAL, (const char *) xval);
    ilistSetStr (list, i, XMLPARSE_NM, (const char *) cur->name);
    mdextfree (xval);
    xmlFree (xval);
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
  // xmlMemoryDump ();

  return list;
}
