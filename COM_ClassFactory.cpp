#include "COM_ClassFactory.h"
#include "COM_ICoverflowControl.h"

void LockModule(BOOL bLock)
{
  if (bLock) {
    InterlockedIncrement(&g_lLocks);
  } else {
    InterlockedDecrement(&g_lLocks);
  }
}

CoverflowClassFactory::CoverflowClassFactory()
{
  m_refCount = 0;

  g_srvObjCount++;
}

CoverflowClassFactory::~CoverflowClassFactory()
{
  g_srvObjCount--;
}

STDMETHODIMP_(ULONG) CoverflowClassFactory::AddRef()
{
  LockModule(TRUE);
  return 2;
}

STDMETHODIMP_(ULONG) CoverflowClassFactory::Release()
{
  LockModule(FALSE);
  return 1;
}

STDMETHODIMP CoverflowClassFactory::QueryInterface(REFIID riid, LPVOID *ppAny)
{
#pragma warning(push)
#pragma warning(disable : 4838)
  static const QITAB qit[] = {
      QITABENT(CoverflowClassFactory, IClassFactory),
      {0},
  };
#pragma warning(pop)
  return QISearch(this, qit, riid, ppAny);
}

STDMETHODIMP CoverflowClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid,
                                               void** ppvObj) {
  _Module.DllRegisterServer(TRUE);

  HRESULT hr;

  *ppvObj = nullptr;

  if (pUnkOuter != nullptr) {
    return CLASS_E_NOAGGREGATION;
  }

  ICoverflowControl* pControl = new ICoverflowControl();


  if (pControl == NULL) {
    return E_UNEXPECTED /*E_OUTOFMEMORY*/;
  }

  hr = pControl->QueryInterface(riid, ppvObj);

  if (SUCCEEDED(hr)) {
    pControl->Release();
  }
  else {
    delete pControl;
  }
  return hr;
}

HRESULT __stdcall CoverflowClassFactory::LockServer(BOOL fLock)
{
  if (fLock)

    g_srvLockCount++;

  else

    g_srvLockCount--;

  return S_OK;
}
