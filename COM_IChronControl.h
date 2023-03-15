#pragma once

interface IChronControl : public IDispatch {

public:

  IChronControl();
 ~IChronControl();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();
  // IDispatch
  STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
  STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,
                             DISPID* rgDispId);
  STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                      DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                      UINT* puArgErr);

  HRESULT STDMETHODCALLTYPE SetPanelColor(
      /* [in] */ BSTR Expression,
      /* [in] */ SAFEARRAY** sf,
      /* [retval][out] */ VARIANT* pvarResult) {};
  STDMETHODIMP SetTextColor(
      /* [in] */ BSTR str,
      /* [in] */ SAFEARRAY** sf,
      /* [retval][out] */ VARIANT* pvarResult) {};

 private:

  LONG m_cRef;
  IDispatch* m_pObject;
  ITypeInfo* m_pTypeInfo;
};
