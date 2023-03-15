#pragma once

#include "COM_Guid.h"

#ifndef _CHRONCLASSFACTORY
#define _CHRONCLASSFACTORY

#include <windows.h>

inline LONG g_lLocks = 0;

inline void LockModule(BOOL bLock);

class ChronClassFactory : public IClassFactory {
 public:
  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppAny);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
  STDMETHODIMP LockServer(BOOL fLock);

  ChronClassFactory();
  virtual ~ChronClassFactory();

 private:
  ULONG m_refCount;
};
#endif
