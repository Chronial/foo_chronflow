#pragma once

#include <string>
#include <vector>

//#import "msscript.ocx" no_namespace
#include "msscript.h"

struct CScriptError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class CScriptObject {
 public:
  CScriptObject(const wchar_t* code);

 public:
  [[noreturn]] void RethrowError(const _com_error& e);
  VARIANT RunProcedure(LPCTSTR szProcName, SAFEARRAY*& saParameters);
  std::vector<std::wstring> GetMethodNames();

 private:
  IScriptControlPtr m_pScript;  // The one and only script control
};

class CSafeArrayHelper {
 public:
  CSafeArrayHelper();
  ~CSafeArrayHelper();

  bool Create(VARTYPE vt, UINT cDims, UINT lBound, UINT cCount);
  bool Destroy();
  UINT GetDimension();

  bool Attach(LPSAFEARRAY psa);
  bool AttachFromVariant(VARIANT* pVariant);
  LPSAFEARRAY Detach();
  LPSAFEARRAY GetArray();
  bool AccessData(void FAR* FAR* pvData);
  bool UnaccessData();
  bool Lock();
  bool Unlock();
  bool PutElement(long lIndices, void FAR* vData);
  bool GetElement(long lIndices, void FAR* vData);
  VARIANT GetAsVariant();

 protected:
  LPSAFEARRAY m_pSA;

 private:
};
