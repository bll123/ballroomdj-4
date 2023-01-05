/*
 * Copyright 2016 Brad Lanam Walnut Creek, CA US
 * Copyright 2021-2023 Brad Lanam Pleasant Hill, CA US
 *
 * This code is in the public domain.
 *
 * much of the original volume code from: https://gist.github.com/rdp/8363580
 */

#include "config.h"

#if _hdr_endpointvolume

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <initguid.h>  // needed to link correctly
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

void
volumeDisconnect (void) {
  return;
}

void
volumeCleanup (void **udata) {
  return;
}

int
volumeProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  IAudioEndpointVolume  *g_pEndptVol = NULL;
  HRESULT               hr = S_OK;
  IMMDevice             *volDevice = NULL;
  OSVERSIONINFO         VersionInfo;
  float                 currentVal;
  wchar_t               *wdevnm;
  IMMDeviceEnumerator   *pEnumerator = NULL;
  IMMDevice             *defDevice = NULL;
  LPWSTR                defsinknm = NULL;

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  ZeroMemory (&VersionInfo, sizeof (OSVERSIONINFO));
  VersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  GetVersionEx (&VersionInfo);
  if (VersionInfo.dwMajorVersion <= 5) {
    return -1;
  }

  CoInitialize (NULL);

  /* Get enumerator for audio endpoint devices. */
  hr = CoCreateInstance (__uuidof (MMDeviceEnumerator),
      NULL, CLSCTX_INPROC_SERVER,
      __uuidof (IMMDeviceEnumerator),
      (void**)&pEnumerator);
  ERROR_EXIT (hr)

  /* Get default audio-rendering device. */
  hr = pEnumerator->GetDefaultAudioEndpoint (eRender, eConsole, &defDevice);
  ERROR_EXIT (hr)
  hr = defDevice->GetId (&defsinknm);
  ERROR_EXIT (hr)

  if (action == VOL_GETSINKLIST) {
    IMMDeviceCollection   *pCollection = NULL;
    UINT                  count;


    sinklist->defname = osFromWideChar (defsinknm);
    sinklist->count = 0;
    sinklist->sinklist = NULL;

    hr = pEnumerator->EnumAudioEndpoints(
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

      if (strcmp (sinklist->defname, sinklist->sinklist [i].name) == 0) {
        sinklist->sinklist [i].defaultFlag = true;
      }

      if (sinklist->sinklist [i].defaultFlag) {
        sinklist->defname = mdstrdup (sinklist->sinklist [i].name);
      }

      sinklist->sinklist [i].description = osFromWideChar (dispName.pwszVal);

      CoTaskMemFree (devid);
      devid = NULL;
      PropVariantClear (&dispName);
      SAFE_RELEASE (pProps)
      SAFE_RELEASE (pDevice)
    }
  }

  if (sinkname != NULL && *sinkname && action == VOL_SET_SYSTEM_DFLT) {
    /* future: like macos, change the system default to match the output sink */
    ;
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    if (sinkname == NULL || ! *sinkname) {
      wdevnm = (wchar_t *) defsinknm;
    } else {
      wdevnm = (wchar_t *) osToWideChar (sinkname);
    }
    hr = pEnumerator->GetDevice (wdevnm, &volDevice);
    ERROR_EXIT (hr)
    if (sinkname != NULL && *sinkname) {
      mdfree (wdevnm);
    }

    hr = volDevice->Activate (__uuidof (IAudioEndpointVolume),
        CLSCTX_ALL, NULL, (void**) &g_pEndptVol);

    if (action == VOL_SET) {
      float got = (float) *vol / 100.0; // needs to be within 1.0 to 0.0
      hr = g_pEndptVol->SetMasterVolumeLevelScalar (got, NULL);
      ERROR_EXIT (hr)
    }

    hr = g_pEndptVol->GetMasterVolumeLevelScalar (&currentVal);
    ERROR_EXIT (hr)

    *vol = (int) round (100 * currentVal);
  }

Exit:
  CoTaskMemFree (defsinknm);
  SAFE_RELEASE (defDevice)
  SAFE_RELEASE (pEnumerator)
  SAFE_RELEASE (g_pEndptVol)
  SAFE_RELEASE (volDevice)
  CoUninitialize ();
  if (vol != NULL) {
    return *vol;
  } else {
    return 0;
  }
}

} /* extern C */


#endif
