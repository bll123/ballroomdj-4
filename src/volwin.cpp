/*
 * Copyright 2016 Brad Lanam Walnut Creek, CA US
 * This code is in the public domain.
 *
 * much of the original volume code from: https://gist.github.com/rdp/8363580
 */

#include "config.h"

#if _hdr_endpointvolume

#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <stdio.h>
#include <math.h>

#include "volume.h"

#define EXIT_ON_ERROR(hr)  \
    if (FAILED(hr)) { printf ("error %ld occurred\n", -hr); goto Exit; }

#define SAFE_RELEASE(punk)  \
    if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

extern "C" {

void
volumeDisconnect (void) {
  return;
}

int
volumeProcess (volaction_t action, char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  IAudioEndpointVolume  *g_pEndptVol = NULL;
  HRESULT               hr = S_OK;
  IMMDeviceEnumerator   *pEnumerator = NULL;
  IMMDevice             *pDevice = NULL;
  OSVERSIONINFO         VersionInfo;
  float                 currentVal;

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

  // Get default audio-rendering device.
  hr = pEnumerator->GetDefaultAudioEndpoint (eRender, eConsole, &pDevice);
  EXIT_ON_ERROR (hr)

  hr = pDevice->Activate (__uuidof (IAudioEndpointVolume),
      CLSCTX_ALL, NULL, (void**) &g_pEndptVol);
  EXIT_ON_ERROR(hr)

  if (action == VOL_SET) {
    float got = (float) *vol / 100.0; // needs to be within 1.0 to 0.0
    hr = g_pEndptVol->SetMasterVolumeLevelScalar (got, NULL);
    EXIT_ON_ERROR (hr)
  }
  hr = g_pEndptVol->GetMasterVolumeLevelScalar (&currentVal);
  EXIT_ON_ERROR (hr)
  *vol = (int) round(100 * currentVal);

Exit:
  SAFE_RELEASE (pEnumerator)
  SAFE_RELEASE (pDevice)
  SAFE_RELEASE (g_pEndptVol)
  CoUninitialize ();
  return *vol;
}

} /* extern C */


#endif
