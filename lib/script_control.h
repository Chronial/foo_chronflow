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
  VARIANT RunProcedure(LPCTSTR szProcName, SAFEARRAY* saParameters);

 private:
  IScriptControlPtr m_pScript;  // The one and only script control
};

class CSafeArrayHelper {
 public:
  CSafeArrayHelper(UINT size);
  CSafeArrayHelper(const CSafeArrayHelper&) = delete;
  CSafeArrayHelper& operator=(const CSafeArrayHelper&) = delete;
  CSafeArrayHelper(CSafeArrayHelper&&) = delete;
  CSafeArrayHelper& operator=(CSafeArrayHelper&&) = delete;
  ~CSafeArrayHelper();

  void PutElement(long idx, _variant_t& vData);
  LPSAFEARRAY GetArray() const;

 private:
  LPSAFEARRAY array;
};
