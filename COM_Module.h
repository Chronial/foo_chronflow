#pragma once

#include "COM_Guid.h"

inline CComModule _Module;

inline ULONG g_srvLockCount = 0;
inline ULONG g_srvObjCount = 0;

STDAPI DllCanUnloadNow(void)
{
  return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
  return _Module.RegisterServer(TRUE);
}

STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer(TRUE);
}
