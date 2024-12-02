/*
 * Copyright 2016 Brad Lanam Walnut Creek, CA US
 * Copyright 2021-2024 Brad Lanam Pleasant Hill, CA US
 *
 * This code is in the public domain.
 *
 * much of the original volume code from: https://gist.github.com/rdp/8363580
 *
 */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

/* initguid.h is needed for the link to PKEY_Device_FriendlyName to work */
/* had to move it to the beginning of the include list 2023-7-31 */
#include <initguid.h>
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

#include "mdebug.h"
#include "osutils.h"
#include "volsink.h"
#include "volume.h"

#define ERROR_EXIT(hr)  \
    if (FAILED(hr)) { printf ("error %ld occurred\n", -hr); goto Exit; }
#define SAFE_RELEASE(data)  \
    if ((data) != NULL) { (data)->Release(); (data) = NULL; }

extern "C" {

enum {
  DEFNM_MAX_SZ = 300,
};

typedef struct {
  char                *defsinkname;
  bool                changed;
  IMMDeviceEnumerator *pEnumerator;
} volwin_t;

static bool   ginit = false;

void
voliDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Windows";
  ret [c++] = NULL;
}

void
voliDisconnect (void)
{
  return;
}

void
voliCleanup (void **udata)
{
  volwin_t    *volwin;

  if (udata == NULL || *udata == NULL) {
    return;
  }

  volwin = (volwin_t *) *udata;
  if (ginit) {
    SAFE_RELEASE (volwin->pEnumerator)
    CoUninitialize ();
    ginit = false;
  }
  mdfree (volwin->defsinkname);
  mdfree (volwin);
  *udata = NULL;
  return;
}

int
voliProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  HRESULT               hr = S_OK;
  OSVERSIONINFO         VersionInfo;
  IMMDevice             *defDevice = NULL;
  LPWSTR                defsinknm = NULL;
  volwin_t              *volwin = NULL;
  char                  *tdefsinknm = NULL;


  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  ZeroMemory (&VersionInfo, sizeof (OSVERSIONINFO));
  VersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  GetVersionEx (&VersionInfo);
  if (VersionInfo.dwMajorVersion <= 5) {
    return -1;
  }

  if (udata != NULL && *udata == NULL) {
    volwin = (volwin_t *) mdmalloc (sizeof (*volwin));
    volwin->defsinkname = mdstrdup ("");
    volwin->changed = false;
    volwin->pEnumerator = NULL;
    *udata = volwin;
  } else {
    volwin = (volwin_t *) *udata;
    volwin->changed = false;
  }

  if (! ginit) {
    CoInitialize (NULL);

    /* Get enumerator for audio endpoint devices. */
    hr = CoCreateInstance (__uuidof (MMDeviceEnumerator),
        NULL, CLSCTX_INPROC_SERVER,
        __uuidof (IMMDeviceEnumerator),
        (void**) &volwin->pEnumerator);
    ERROR_EXIT (hr)
    ginit = true;
  }

  /* Get default audio-rendering device. */
  hr = volwin->pEnumerator->GetDefaultAudioEndpoint (eRender, eConsole, &defDevice);
  ERROR_EXIT (hr)
  hr = defDevice->GetId (&defsinknm);
  SAFE_RELEASE (defDevice)
  ERROR_EXIT (hr)

  tdefsinknm = osFromWideChar (defsinknm);

  if (! *volwin->defsinkname) {
    volwin->defsinkname = mdstrdup (tdefsinknm);
    volwin->changed = false;
  } else {
    /* leave the changed flag set until the user fetches it */
    if (strcmp (volwin->defsinkname, tdefsinknm) != 0) {
      dataFree (volwin->defsinkname);
      volwin->defsinkname = mdstrdup (tdefsinknm);
      volwin->changed = true;
    }
  }

  if (action == VOL_CHK_SINK) {
    bool    orig;

    /* leave the changed flag set until the user fetches it */
    orig = volwin->changed;
    volwin->changed = false;

    dataFree (tdefsinknm);
    CoTaskMemFree (defsinknm);

    return orig;
  }

  if (action == VOL_GETSINKLIST) {
    IMMDeviceCollection   *pCollection = NULL;
    UINT                  count;


    dataFree (sinklist->defname);
    /* for windows, the default device name is an empty string */
    sinklist->defname = mdstrdup ("");
    sinklist->count = 0;
    sinklist->sinklist = NULL;

    hr = volwin->pEnumerator->EnumAudioEndpoints (
        eRender, DEVICE_STATE_ACTIVE, &pCollection);
    ERROR_EXIT (hr)

    hr = pCollection->GetCount (&count);
    ERROR_EXIT (hr)

    sinklist->count = count;
    sinklist->sinklist = (volsinkitem_t *) mdrealloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));

    for (UINT i = 0; i < count; ++i) {
      LPWSTR      devid = NULL;
      IMMDevice   *pDevice = NULL;
      IPropertyStore *pProps = NULL;
      PROPVARIANT dispName;

      sinklist->sinklist [i].defaultFlag = false;
      sinklist->sinklist [i].idxNumber = i;
      sinklist->sinklist [i].name = NULL;
      sinklist->sinklist [i].description = NULL;

      hr = pCollection->Item (i, &pDevice);
      ERROR_EXIT (hr)

      hr = pDevice->GetId (&devid);
      ERROR_EXIT (hr)

      hr = pDevice->OpenPropertyStore (STGM_READ, &pProps);
      ERROR_EXIT (hr)

      PropVariantInit (&dispName);
      // display name
      hr = pProps->GetValue (PKEY_Device_FriendlyName, &dispName);
      ERROR_EXIT (hr)

      sinklist->sinklist [i].name = osFromWideChar (devid);

      if (strcmp (tdefsinknm, sinklist->sinklist [i].name) == 0) {
        sinklist->sinklist [i].defaultFlag = true;
      }

      sinklist->sinklist [i].description = osFromWideChar (dispName.pwszVal);

      CoTaskMemFree (devid);
      devid = NULL;
      PropVariantClear (&dispName);
      SAFE_RELEASE (pProps)
      SAFE_RELEASE (pDevice)
    }

    dataFree (tdefsinknm);
    CoTaskMemFree (defsinknm);
    return 0;
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    wchar_t               *wdevnm;
    IMMDevice             *volDevice = NULL;
    IAudioEndpointVolume  *g_pEndptVol = NULL;
    float                 currentVal;

    if (sinkname == NULL || ! *sinkname) {
      wdevnm = (wchar_t *) defsinknm;
fprintf (stderr, "   get/set sinkname: %s\n", tdefsinknm);
    } else {
fprintf (stderr, "   get/set sinkname: %s\n", sinkname);
      wdevnm = (wchar_t *) osToWideChar (sinkname);
    }

    hr = volwin->pEnumerator->GetDevice (wdevnm, &volDevice);
    ERROR_EXIT (hr)
    if (sinkname != NULL && *sinkname) {
      mdfree (wdevnm);
    }

    hr = volDevice->Activate (__uuidof (IAudioEndpointVolume),
        CLSCTX_ALL, NULL, (void**) &g_pEndptVol);

    if (action == VOL_SET) {
      float svol = (float) *vol / 100.0; // needs to be within 1.0 to 0.0
      hr = g_pEndptVol->SetMasterVolumeLevelScalar (svol, NULL);
      ERROR_EXIT (hr)
    }

    hr = g_pEndptVol->GetMasterVolumeLevelScalar (&currentVal);
    ERROR_EXIT (hr)

    *vol = (int) round (100 * currentVal);
  }

Exit:
  dataFree (tdefsinknm);
  CoTaskMemFree (defsinknm);

  if (vol != NULL) {
    return *vol;
  } else {
    return 0;
  }
}

} /* extern C */
