#pragma once
#include "COM_Guid.h"

#include <xkeycheck.h>
#include "COM_ClassFactory.h"

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppAny)
{
  static ChronClassFactory pFactory;
  HRESULT hr;

  *ppAny = NULL;

  if (IsEqualCLSID(rclsid, CLSID_Chron_Control)) {

    hr = pFactory.QueryInterface(riid, ppAny);

  }
  else {
    FB2K_console_formatter() << "IChronControl COM not available";
    hr = CLASS_E_CLASSNOTAVAILABLE;
  }

  if (FAILED(hr)) {
    FB2K_console_formatter() << "IChronControl COM factory failed";
    hr = CLASS_E_CLASSNOTAVAILABLE;
  }

  return hr;
}

STDAPI DllCanUnloadNow()
{
  if (g_srvLockCount == 0 && g_objCount == 0) {
    return S_OK;
  } else {
    return S_FALSE;
  }
}
