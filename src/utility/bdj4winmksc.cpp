/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 *   Excepting code excerpts from microsoft.
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlobj.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>

typedef struct {
  const wchar_t     *shortcut;
  const wchar_t     *targetpath;
  const wchar_t     *workingdir;
  const wchar_t     *args;
} scinfo_t;

static HRESULT winmkshortcut (scinfo_t *scinfo);

int
main (int argc, const char *argv [])
{
  scinfo_t    scinfo;
  int         rc;
  wchar_t     **wargv;
  int         wargc;


  wargv = CommandLineToArgvW (GetCommandLineW(), &wargc);

  if (wargc != 5 && wargc != 4) {
    fprintf (stderr, "ERR: invalid arguments %d\n", argc);
    return 1;
  }

  scinfo.shortcut = wargv [1];
  scinfo.targetpath = wargv [2];
  scinfo.workingdir = wargv [3];
  scinfo.args = NULL;
  if (argc == 5) {
    scinfo.args = wargv [4];
  }

  rc = winmkshortcut (&scinfo);
  return rc;
}

/* code from microsoft, modified */

static HRESULT
winmkshortcut (scinfo_t *scinfo)
{
  HRESULT       hres;
  IShellLink    * psl = NULL;
  IPersistFile  * ppf = NULL;

  hres = CoInitialize (NULL);
  if (! SUCCEEDED (hres)) {
    fprintf (stderr, "ERR: coinitialize failed %04lx\n", hres);
    return hres;
  }

  hres = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
      IID_IShellLink, (LPVOID *) &psl);
  if (! SUCCEEDED (hres)) {
    fprintf (stderr, "ERR: cocreatinstance failed %04lx\n", hres);
    return hres;
  }

  psl->SetPath (scinfo->targetpath);
  psl->SetWorkingDirectory (scinfo->workingdir);
  if (scinfo->args != NULL) {
    psl->SetArguments (scinfo->args);
  }

  hres = psl->QueryInterface (IID_IPersistFile, (LPVOID *) &ppf);

  if (SUCCEEDED (hres)) {
    hres = ppf->Save (scinfo->shortcut, TRUE);
    ppf->Release ();
  } else {
    fprintf (stderr, "ERR: queryinterface failed %04lx\n", hres);
  }
  psl->Release ();

  if (! SUCCEEDED (hres)) {
    fprintf (stderr, "ERR: save failed %04lx\n", hres);
  }

  CoUninitialize ();

  return hres;
}
