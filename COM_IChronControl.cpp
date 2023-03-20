#include "stdafx.h"

#include <strsafe.h>
#include <filesystem>
#include <oleauto.h>
#include "COM_Guid.h"

#include "COM_ClassFactory.h"
#include "COM_IChronControl.h"

#include "ConfigData.h"
#include "EngineThread.h"
#include "Engine.h"

using EM = engine_messages::Engine::Messages;

IChronControl::IChronControl() {

  ITypeLib* pTypeLib = nullptr;
  m_pTypeInfo = nullptr;
  m_cRef = 1;

  pfc::string8 path;
#ifdef _WIN64
  path << core_api::pathInProfile("user-components-x64\\");
  path << core_api::get_my_file_name();
#else
  path << core_api::pathInProfile("user-components\\");
  path << core_api::get_my_file_name();
#endif

  extract_native_path(path, path);
  std::filesystem::path os_src;

  try {
    os_src = std::filesystem::u8path(path.c_str());
    os_src.append("foo_chronflow.tlb");

    if (!std::filesystem::exists(os_src)) {
      throw;
    }

  } catch (...) {
    FB2K_console_formatter() << AppNameInternal << " failed to initialize chron COM object\n";
    return;
  }

  HRESULT hrlib = LoadTypeLibEx(os_src.c_str(), REGKIND_REGISTER, &pTypeLib);

  if SUCCEEDED (hrlib) {

    pTypeLib->GetTypeInfoOfGuid(CLSID_Chron_Control, &m_pTypeInfo);

    if (!m_pTypeInfo) {
      pTypeLib->GetTypeInfo(0, &m_pTypeInfo);
    }

    pTypeLib->Release();
  }
  LockModule(TRUE);
}

VOID SafeRelease(PVOID ppObj) {
  try {
    IUnknown** ppunk = static_cast<IUnknown**>(ppObj);
    if (*ppunk) {
      (*ppunk)->Release();
      *ppunk = nullptr;
    }
  } catch (...) {
  }
}

void LockModule(BOOL bLock) {
  if (bLock) {
    InterlockedIncrement(&g_lLocks);
  } else {
    InterlockedDecrement(&g_lLocks);
  }
}

IChronControl::~IChronControl()
{
  SafeRelease(&m_pTypeInfo);
  LockModule(FALSE);
}

STDMETHODIMP_(ULONG) IChronControl::AddRef()
{
  return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) IChronControl::Release()
{
  if (InterlockedDecrement(&m_cRef) == 0) {
    delete this;
    return 0;
  }
  return m_cRef;
}

STDMETHODIMP IChronControl::QueryInterface(REFIID riid, void** ppvObject) {
#pragma warning(push)
#pragma warning(disable : 4838)
  static const QITAB qit[] = {
      QITABENT(IChronControl, IDispatch),
      {0},
  };
#pragma warning(pop)
  return QISearch(this, qit, riid, ppvObject);
}

STDMETHODIMP IChronControl::GetTypeInfoCount(UINT* pctinfo) {
  *pctinfo = 1;
  return S_OK;
}

STDMETHODIMP IChronControl::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {
  if (m_pTypeInfo) {
    m_pTypeInfo->AddRef();
    *ppTInfo = m_pTypeInfo;
    return S_OK;
  }
  return E_NOTIMPL;
}

struct TEmethod {
  LONG id;
  LPCWSTR name;
};

size_t teBSearch(TEmethod* method, int nSize, int* pMap, LPOLESTR bs) {

  int nMin = 0;
  int nMax = nSize - 1;

  while (nMin <= nMax) {
    int nIndex = (nMin + nMax) / 2;
    int nCC = lstrcmpi(bs, method[pMap[nIndex]].name);
    if (nCC < 0) {
      nMax = nIndex - 1;
      continue;
    }
    if (nCC > 0) {
      nMin = nIndex + 1;
      continue;
    }
    return pMap[nIndex];
  }
  return pfc::infinite_size;
}

HRESULT teGetDispId(TEmethod* method, int nCount, int* pMap, LPOLESTR bs, DISPID* rgDispId)
{
  if (pMap) {
    size_t nIndex = teBSearch(method, nCount, pMap, bs);
    if (nIndex != ~0) {
      *rgDispId = method[nIndex].id;
      return S_OK;
    }
  } else {
    for (int i = 0; method[i].name; i++) {
      if (lstrcmpi(bs, method[i].name) == 0) {
        *rgDispId = method[i].id;
        return S_OK;
      }
    }
  }
  return DISP_E_UNKNOWNNAME;
}

int* g_map;

TEmethod methodTSC[] = {

    {2001, L"SetPanelColor"},
    {2003, L"SetTextColor"},
    {0, nullptr}
};

STDMETHODIMP IChronControl::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                                            LCID lcid, DISPID* rgDispId) {
  return teGetDispId(methodTSC, _countof(methodTSC), g_map, *rgszNames, rgDispId);
}

size_t GetIntFromVariant(VARIANT* pv) {

  if (pv) {
    if (pv->vt == (VT_VARIANT | VT_BYREF)) {
      return GetIntFromVariant(pv->pvarVal);
    }
    if (pv->vt == VT_I4) {
      return pv->lVal;
    }
    if (pv->vt == VT_UI4) {
      return pv->ulVal;
    }
    if (pv->vt == VT_R8) {
      return gsl::narrow_cast<size_t>(/*static_cast<LONGLONG>*/ /*(*/ pv->dblVal) /*)*/;
    }

    VARIANT vo;
    VariantInit(&vo);

    if SUCCEEDED (VariantChangeType(&vo, pv, 0, VT_I4)) {
      return vo.lVal;
    }
    if SUCCEEDED (VariantChangeType(&vo, pv, 0, VT_UI4)) {
      return vo.ulVal;
    }
    if SUCCEEDED (VariantChangeType(&vo, pv, 0, VT_I8)) {
      return gsl::narrow_cast<size_t>(vo.llVal);
    }
  }
  return 0;
}

VOID teVariantChangeType(__out VARIANTARG* pvargDest, __in const VARIANTARG* pvarSrc,
                         __in VARTYPE vt) {
  VariantInit(pvargDest);
  if FAILED(VariantChangeType(pvargDest, pvarSrc, 0, vt)) {
    pvargDest->llVal = 0;
  }
}

size_t BSTR_RefColor(BSTR bstr) {

  std::wstring ws(bstr, SysStringLen(bstr));
  const bool bhex = !ws.compare(0, 2, L"0x");

  int radix = 10;
  if (bhex) {
    auto cxitems = std::count_if(ws.begin() + 2, ws.end(), [](auto c) {
      return std::isxdigit((unsigned char)c);
      });

    if (cxitems == 6/*ws.length() - 2*/) {
      radix = 16;
    }
  }

  errno = 0;
  wchar_t* end;
  size_t ival = wcstol(ws.c_str(), &end, radix);

  if (errno) {
    ival = _wtol(bstr);
  }
  return ival;
}

STDMETHODIMP IChronControl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                     WORD wFlags, DISPPARAMS* pDispParams,
                                     VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                                     UINT* puArgErr) {
  size_t ndxArg;
  if (pDispParams) {
    ndxArg = pDispParams->cArgs - 1;
  }
  else {
    return DISP_E_BADPARAMCOUNT;
  }

  VARIANT v;
  VariantInit(&v);

  HRESULT hr = S_OK;

  switch (dispIdMember) {

    // SET PANEL COLOR

    case 2001:
    case 2003 : {

      BOOL bArg = true;

      if (pDispParams->cArgs == 2) {

        if (pDispParams->rgvarg[0].vt == (VT_VARIANT | VT_ARRAY)) {
          LPSAFEARRAY pSafeArray = pDispParams->rgvarg[0].parray;
          VARTYPE itemType;
          if (SUCCEEDED(SafeArrayGetVartype(pSafeArray, &itemType))) {
            if (itemType == VT_VARIANT) {
              if (SafeArrayGetDim(pSafeArray) == 1) {
                LONG lBound;
                LONG uBound;
                if (SUCCEEDED(SafeArrayGetLBound(pSafeArray, 1, &lBound)) &&
                    SUCCEEDED(SafeArrayGetUBound(pSafeArray, 1, &uBound))) {

                  VARIANT vrefresh;
                  VariantInit(&vrefresh);
                  long ai = 0;
 
                  if (SUCCEEDED(::SafeArrayGetElement(pSafeArray, &ai, &vrefresh))) {

                    size_t irefresh = GetIntFromVariant(&vrefresh);
                    bArg = irefresh;
                    
                    VariantClear(&vrefresh);
                  }
                }
              }
            }
          }
        }
      }

      VARIANT vcolor;
      VariantInit(&vcolor);
      teVariantChangeType(&vcolor, &pDispParams->rgvarg[ndxArg], VT_BSTR);

      if (!vcolor.bstrVal) {
        return DISP_E_BADPARAMCOUNT;
      }
      
      COLORREF colref = BSTR_RefColor(vcolor.bstrVal);
      VariantClear(&vcolor);

      // SET TITLE COLOR
      if (dispIdMember == 2001) {
        coverflow::configData->PanelBgCustom = true;
        coverflow::configData->PanelBg = colref;
      // SET TITLE COLOR
      } else if (dispIdMember == 2003) {
        coverflow::configData->TitleColorCustom = true;
        coverflow::configData->TitleColor = colref;
      }

      engine::EngineThread::forEach([bArg](engine::EngineThread& t) {
          t.send<EM::TextFormatChangedMessage>();

          if (bArg) {
            t.send<EM::RedrawMessage>();
          }
      });
            
      break;
    }
    default:
      hr = DISP_E_MEMBERNOTFOUND;
      break;
  }
  return hr;
}
