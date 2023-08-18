#pragma once

#include "COM_Guid.h"

#ifndef _COVERFLOWCLASSFACTORY
#define _COVERFLOWCLASSFACTORY

#include <windows.h>

inline LONG g_lLocks = 0;

inline void LockModule(BOOL bLock);

class CoverflowClassFactory : public IClassFactory {
 public:
  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppAny);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
  STDMETHODIMP LockServer(BOOL fLock);

  CoverflowClassFactory();
  virtual ~CoverflowClassFactory();

 private:
  ULONG m_refCount;
};
#endif
