#pragma once

#include <list>
#include <xstring>

//#import "msscript.ocx" no_namespace
#include "msscript.h"

#define LANGUAGE_DEFAULT _T("VBScript")
#define RT_SCRIPT _T("SCRIPT")
#define LANGUAGE_NAME_LEN 40
#define ERROR_DESC_LEN 256

class CScriptObject {
 public:
  CScriptObject();
  ~CScriptObject();

 public:
  LPCTSTR GetLanguage();
  void SetLanguage(LPCTSTR szLanguage);
  int GetMethodsCount() const;
  LPCTSTR GetNameAt(int index);
  void Reset();
  bool LoadScript(LPCTSTR szFilename);
  bool LoadScriptResource(LPCTSTR lpName, LPCTSTR lpType, HINSTANCE hInstance);
  bool AddScript(LPCTSTR szCode);
  LPCTSTR GetErrorString();
  bool ExecuteStatement(LPCTSTR szStatement);
  bool RunProcedure(LPCTSTR szProcName, SAFEARRAY** saParameters, VARIANT* varRet);

 protected:
  void CommonConstruct();
  bool GetMethodsName();
  LPCTSTR GetScriptFunction(LPCTSTR name);

  IScriptControlPtr m_pScript;                // The one and only script control
  std::list<std::wstring> m_FunctionList;     // Function list
  TCHAR m_szLanguage[LANGUAGE_NAME_LEN + 1];  // Current language
  TCHAR m_szError[ERROR_DESC_LEN + 1];        // Description error
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
