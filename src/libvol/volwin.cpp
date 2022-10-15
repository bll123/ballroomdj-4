/*
 * Copyright 2016 Brad Lanam Walnut Creek, CA US
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

#include "osutils.h"
#include "volsink.h"
#include "volume.h"

#define EXIT_ON_ERROR(hr)  \
    if (FAILED(hr)) { printf ("error %ld occurred\n", -hr); goto Exit; }
#define RETURN_ON_ERROR(hr)  \
    if (FAILED(hr)) { printf ("error %ld occurred\n", -hr); goto SinkListExit; }
#define SAFE_RELEASE(data)  \
    if ((data) != NULL) { (data)->Release(); (data) = NULL; }

extern "C" {

void
volumeDisconnect (void) {
  return;
}

int
volumeProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  IAudioEndpointVolume  *g_pEndptVol = NULL;
  HRESULT               hr = S_OK;
  IMMDevice             *volDevice = NULL;
  OSVERSIONINFO         VersionInfo;
  float                 currentVal;
  wchar_t               *wdevnm;
  IMMDeviceEnumerator   *pEnumerator = NULL;

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

  // Get enumerator for audio endpoint devices.
  hr = CoCreateInstance (__uuidof (MMDeviceEnumerator),
      NULL, CLSCTX_INPROC_SERVER,
      __uuidof (IMMDeviceEnumerator),
      (void**)&pEnumerator);
  EXIT_ON_ERROR (hr)

  if (action == VOL_GETSINKLIST) {
    IMMDeviceCollection   *pCollection = NULL;
    IMMDevice             *defDevice = NULL;
    UINT                  count;
    LPWSTR                defid = NULL;


    sinklist->defname = NULL;
    sinklist->count = 0;
    sinklist->sinklist = NULL;

    hr = pEnumerator->EnumAudioEndpoints(
        eRender, DEVICE_STATE_ACTIVE, &pCollection);
    RETURN_ON_ERROR (hr)

    hr = pCollection->GetCount (&count);
    RETURN_ON_ERROR (hr)

    sinklist->count = count;
    sinklist->sinklist = (volsinkitem_t *) realloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));

    // Get default audio-rendering device.
    hr = pEnumerator->GetDefaultAudioEndpoint (eRender, eConsole, &defDevice);
    RETURN_ON_ERROR (hr)
    hr = defDevice->GetId (&defid);
    RETURN_ON_ERROR (hr)

    sinklist->defname = osFromWideChar (defid);

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
      RETURN_ON_ERROR (hr)

      hr = pDevice->GetId (&devid);
      RETURN_ON_ERROR (hr)

      hr = pDevice->OpenPropertyStore (STGM_READ, &pProps);
      RETURN_ON_ERROR (hr)

      PropVariantInit (&dispName);
      // display name
      hr = pProps->GetValue (PKEY_Device_FriendlyName, &dispName);
      RETURN_ON_ERROR (hr)

      sinklist->sinklist [i].name = osFromWideChar (devid);

      if (strcmp (sinklist->defname, sinklist->sinklist [i].name) == 0) {
        sinklist->sinklist [i].defaultFlag = true;
      }

      if (sinklist->sinklist [i].defaultFlag) {
        sinklist->defname = strdup (sinklist->sinklist [i].name);
      }

      sinklist->sinklist [i].description = osFromWideChar (dispName.pwszVal);

      CoTaskMemFree (devid);
      devid = NULL;
      PropVariantClear (&dispName);
      SAFE_RELEASE (pProps)
      SAFE_RELEASE (pDevice)
    }

SinkListExit:
    CoTaskMemFree (defid);
    SAFE_RELEASE (defDevice)
    SAFE_RELEASE (g_pEndptVol)
    SAFE_RELEASE (volDevice)
    SAFE_RELEASE (pEnumerator)
    CoUninitialize ();
    return 0;
  }

  wdevnm = (wchar_t *) osToWideChar (sinkname);
  hr = pEnumerator->GetDevice (wdevnm, &volDevice);
  EXIT_ON_ERROR (hr)
  free (wdevnm);

  hr = volDevice->Activate (__uuidof (IAudioEndpointVolume),
      CLSCTX_ALL, NULL, (void**) &g_pEndptVol);
  EXIT_ON_ERROR (hr)

  if (action == VOL_SET) {
    float got = (float) *vol / 100.0; // needs to be within 1.0 to 0.0
    hr = g_pEndptVol->SetMasterVolumeLevelScalar (got, NULL);
    EXIT_ON_ERROR (hr)
  }
  hr = g_pEndptVol->GetMasterVolumeLevelScalar (&currentVal);
  EXIT_ON_ERROR (hr)

  *vol = (int) round (100 * currentVal);

Exit:
  SAFE_RELEASE (g_pEndptVol)
  SAFE_RELEASE (volDevice)
  SAFE_RELEASE (pEnumerator)
  CoUninitialize ();
  return *vol;
}

} /* extern C */


#endif
