#include "COM_ClassFactory.h"
#include "COM_IChronControl.h"

void LockModule(BOOL bLock)
{
  if (bLock) {
    InterlockedIncrement(&g_lLocks);
  } else {
    InterlockedDecrement(&g_lLocks);
  }
}

ChronClassFactory::ChronClassFactory()
{
  m_refCount = 0;

  g_srvObjCount++;
}

ChronClassFactory::~ChronClassFactory()
{
  g_srvObjCount--;
}

STDMETHODIMP_(ULONG) ChronClassFactory::AddRef()
{
  LockModule(TRUE);
  return 2;
}

STDMETHODIMP_(ULONG) ChronClassFactory::Release()
{
  LockModule(FALSE);
  return 1;
}

STDMETHODIMP ChronClassFactory::QueryInterface(REFIID riid, LPVOID *ppAny)
{
#pragma warning(push)
#pragma warning(disable : 4838)
  static const QITAB qit[] = {
      QITABENT(ChronClassFactory, IClassFactory),
      {0},
  };
#pragma warning(pop)
  return QISearch(this, qit, riid, ppAny);
}

STDMETHODIMP ChronClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid,
                                               void** ppvObj) {
  _Module.DllRegisterServer(TRUE);

  HRESULT hr;

  *ppvObj = nullptr;

  if (pUnkOuter != nullptr) {
    return CLASS_E_NOAGGREGATION;
  }

  IChronControl* pControl = new IChronControl();

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

HRESULT __stdcall ChronClassFactory::LockServer(BOOL fLock)
{
  if (fLock)

    g_srvLockCount++;

  else

    g_srvLockCount--;

  return S_OK;
}
