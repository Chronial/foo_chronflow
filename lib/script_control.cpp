///////////////////////////////////////////////////////////////////////////////
//	File:		ScriptObject.cpp
//	Version:	1.0
//
//	Author:		Ernest Laurentin
//	E-mail:		elaurentin@sympatico.ca
//
//	This class implements MSScript control
//	It can interface script from resource file or text file
//
//	This code may be used in compiled form in any way you desire. This
//	file may be redistributed unmodified by any means PROVIDING it is
//	not sold for profit without the authors written consent, and
//	providing that this notice and the authors name and all copyright
//	notices remains intact.
//
//	An email letting me know how you are using it would be nice as well.
//
//	This file is provided "as is" with no expressed or implied warranty.
//	The author accepts no liability for any damage/loss of business that
//	this c++ class may cause.
//
//	Version history
//
//	1.0	- Initial release.
//	1.1 - Bug fixes for VC7 and Unicode
///////////////////////////////////////////////////////////////////////////////
#include "script_control.h"

#include <comutil.h>

#ifndef ASSERT
#define ASSERT PFC_ASSERT
#endif

#ifndef TRACE
#define TRACE __noop
#endif


///////////////////////////////////////////////////////////////////////////////
// Construction
CScriptObject::CScriptObject() {
  CommonConstruct();  // will throw exception if failed
}

CScriptObject::~CScriptObject() {
  // Destroy object- and release
  m_pScript = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Members

///////////////////////////////////////////////////////////////////////////////
// CommonConstruct
void CScriptObject::CommonConstruct() {
  HRESULT hr = m_pScript.CreateInstance(__uuidof(ScriptControl));
  _com_util::CheckError(hr);  // will throw an exception if failed

  // will not come here if exception
  _tcscpy_s(m_szLanguage, LANGUAGE_DEFAULT);
  m_pScript->PutAllowUI(VARIANT_FALSE);
  m_pScript->PutLanguage(_bstr_t(m_szLanguage));
}

///////////////////////////////////////////////////////////////////////////////
// GetLanguage : Get current script language
LPCTSTR CScriptObject::GetLanguage() {
  return m_szLanguage;
}

///////////////////////////////////////////////////////////////////////////////
// SetLanguage : Set current script language
void CScriptObject::SetLanguage(LPCTSTR szLanguage) {
  _tcscpy_s(m_szLanguage, szLanguage);

  if (m_pScript != NULL) {
    m_pScript->PutLanguage(_bstr_t(szLanguage));
    m_pScript->Reset();
    m_FunctionList.clear();
  }
}

///////////////////////////////////////////////////////////////////////////////
// GetMethodsCount
int CScriptObject::GetMethodsCount() const {
  return m_FunctionList.size();
}

///////////////////////////////////////////////////////////////////////////////
// GetNameAt : Get method name at specified index
LPCTSTR CScriptObject::GetNameAt(int index) {
  if (index >= 0 && index < (int)m_FunctionList.size()) {
    auto iter = m_FunctionList.begin();
    while (index > 0) {
      iter++;
      index--;
    }
    return (*iter).c_str();
  }
  return TEXT("");
}

///////////////////////////////////////////////////////////////////////////////
// Reset : Reset script control object
void CScriptObject::Reset() {
  if (m_pScript != NULL) {
    m_pScript->Reset();

    // empty list...
    m_FunctionList.clear();
  }
}

///////////////////////////////////////////////////////////////////////////////
// GetErrorString : Get Script error string
LPCTSTR CScriptObject::GetErrorString() {
  m_szError[0] = 0;
  if (m_pScript != NULL) {
    try {
      IScriptErrorPtr pError = m_pScript->GetError();
      if (pError != NULL) {
        _bstr_t desc = _bstr_t("Error: ") + pError->GetDescription() + _bstr_t(", ");
        desc += pError->GetText() + _bstr_t("; in line ");
        desc += _bstr_t(pError->GetLine());
        int count = __min(desc.length(), ERROR_DESC_LEN);  // string may be truncated...
        _tcsncpy_s(m_szError, (LPCTSTR)desc, count);
        m_szError[count] = 0;
        pError->Clear();
      }
    } catch (_com_error& e) {
      (void)(e);
      TRACE((LPSTR)e.Description());
      TRACE((LPSTR) "\n");
    }
  }
  return m_szError;
}

///////////////////////////////////////////////////////////////////////////////
// GetMethodsName: Get methods name list
bool CScriptObject::GetMethodsName() {
  bool bResult = false;
  if (m_pScript != NULL) {
    IScriptProcedureCollectionPtr pIProcedures = m_pScript->GetProcedures();

    // empty list...
    m_FunctionList.clear();

    try {
      long count = pIProcedures->GetCount();
      for (long index = 1; index <= count; index++) {
        IScriptProcedurePtr pIProcPtr = pIProcedures->GetItem(_variant_t(index));
        _bstr_t name = pIProcPtr->GetName();
        m_FunctionList.insert(m_FunctionList.end(), (LPCTSTR)name);
        pIProcPtr = NULL;
      }

      bResult = true;
    } catch (...) {
      // Just catch the exception, call GetErrorString()
      // to retreive last error
    }

    pIProcedures = NULL;
  }
  return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// GetScriptFunction
//		Get Script function name, this is useful for script with case sensitive
//		function name.
LPCTSTR CScriptObject::GetScriptFunction(LPCTSTR szName) {
  auto iter = m_FunctionList.begin();
  while (iter != m_FunctionList.end()) {
    if ((*iter).compare(szName) == 0)
      return (*iter).c_str();
    iter++;
  }
  return TEXT("");
}

//////////////////////////////////////////////////////////////////////////
// LoadScriptResource:
//		Load a Script resource.  This function loads and insert all functions
//		and procedures to the component.  The script resource may contain comments
//		as well.  Global variables may also be defined. You may want to see the
//		script resource as a Module file in Visual Basic.
//////////////////////////////////////////////////////////////////////////
bool CScriptObject::LoadScriptResource(LPCTSTR lpName, LPCTSTR lpType,
                                       HINSTANCE hInstance) {
  try {
    if (m_pScript) {
      HRSRC res = ::FindResource(hInstance, lpName, lpType);
      ASSERT(res != NULL);
      BYTE* pbytes = (BYTE*)LockResource(LoadResource(hInstance, res));
      ASSERT(pbytes != NULL);
      _bstr_t strCode = (LPCSTR)(pbytes);
      m_pScript->AddCode(strCode);
      GetMethodsName();
      return true;
    }
  } catch (...) {
    // Just catch the exception, call GetErrorString()
    // to retreive last error
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// LoadScript
//		Load a Script File.  This function loads and insert all functions and
//		procedures to the component.  The script file may contain comments as
//well. 		Global variables may also be define. You may want to see the script file 		as a
//Module file in Visual Basic. 		The script file is probably a simple text file (ASCII
//format)
///////////////////////////////////////////////////////////////////////////////
bool CScriptObject::LoadScript(LPCTSTR szFilename) {
  HANDLE hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD dwSize = GetFileSize(hFile, NULL);
    if (0xFFFFFFFF != dwSize) {
      BYTE* pbytes = (BYTE*)GlobalAlloc(GPTR, dwSize + 1);
      if (pbytes != NULL) {
        DWORD dwRead = 0;
        bool bResult = false;
        if (ReadFile(hFile, pbytes, dwSize, &dwRead, NULL)) {
          try {
            if (m_pScript) {
              _bstr_t strCode = (LPCSTR)(pbytes);
              m_pScript->AddCode(strCode);
              GetMethodsName();
              bResult = true;
            }
          } catch (...) {
            // Just catch the exception, call GetErrorString()
            // to retreive last error
          }

          GlobalFree((HGLOBAL)pbytes);
          CloseHandle(hFile);
          return bResult;
        }

        GlobalFree((HGLOBAL)pbytes);
      }
    }

    CloseHandle(hFile);
    return false;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
// AddScript
//		Use this function to add a script function, useful for internal use
//		not global script (resource file).
///////////////////////////////////////////////////////////////////////////////
bool CScriptObject::AddScript(LPCTSTR szCode) {
  try {
    if (m_pScript != NULL) {
      m_pScript->AddRef();
      _bstr_t strCode = szCode;
      m_pScript->AddCode(strCode);
      GetMethodsName();
      m_pScript->Release();
      return true;
    }
  } catch (...) {
    // Just catch the exception, call GetErrorString()
    // to retreive last error
    m_pScript->Release();
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// ExecuteStatement
//		Use this function to execute a "Sub routine" - no arguments
///////////////////////////////////////////////////////////////////////////////
bool CScriptObject::ExecuteStatement(LPCTSTR szStatement) {
  try {
    if (m_pScript != NULL) {
      m_pScript->AddRef();
      m_pScript->ExecuteStatement(_bstr_t(szStatement));
      m_pScript->Release();
      return true;
    }
  } catch (...) {
    // Just catch the exception, call GetErrorString()
    // to retreive last error
    m_pScript->Release();
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
// RunProcedure
//		Use this function to run a "Procedure" or "Function" - with arguments
///////////////////////////////////////////////////////////////////////////////
bool CScriptObject::RunProcedure(LPCTSTR szProcName, SAFEARRAY** saParameters,
                                 VARIANT* varRet) {
  // required argument
  ASSERT(saParameters != NULL);
  ASSERT(varRet != NULL);

  try {
    if (m_pScript != NULL) {
      m_pScript->AddRef();
      bool bResult = false;
      _bstr_t szFunc = GetScriptFunction(szProcName);
      if (szFunc.length() > 0) {
        *varRet = m_pScript->Run(szFunc, saParameters);
        bResult = true;
      }
      m_pScript->Release();
      return bResult;
    }
  } catch (...) {
    // Just catch the exception, call GetErrorString()
    // to retreive last error
    m_pScript->Release();
  }
  return false;
}



///////////////////////////////////////////////////////////////////////////////
//	File:		SafeArrayHelper.cpp
//	Version:	1.0
//
//	Author:		Ernest Laurentin
//	E-mail:		elaurentin@sympatico.ca
//
//	Version history
//
//	1.0	- Initial release.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Construction
CSafeArrayHelper::CSafeArrayHelper() : m_pSA(NULL) {}

CSafeArrayHelper::~CSafeArrayHelper() {
  Destroy();
}

///////////////////////////////////////////////////////////////////////////////
// Members

///////////////////////////////////////////////////////////////////////////////
// Create : Creates a SafeArray object
bool CSafeArrayHelper::Create(VARTYPE vt, UINT cDims, UINT lBound, UINT cCount) {
  Destroy();

  if (cDims == 1)  // this is somewhat faster...
    m_pSA = SafeArrayCreateVector(vt, lBound, cCount);
  else {
    SAFEARRAYBOUND rgsabound[1] = {0};
    rgsabound[0].lLbound = lBound;
    rgsabound[0].cElements = cCount;
    m_pSA = SafeArrayCreate(vt, cDims, rgsabound);
  }
  return (m_pSA != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Destroy : Destroy the safeArray object (can't destroy it object is locked)
bool CSafeArrayHelper::Destroy() {
  if (NULL != m_pSA) {
    // Maybe the object is locked!
    if (SUCCEEDED(SafeArrayDestroy(m_pSA)))
      m_pSA = NULL;
  }
  return (m_pSA == NULL);
}

///////////////////////////////////////////////////////////////////////////////
// GetArray : return Safearray pointer
LPSAFEARRAY CSafeArrayHelper::GetArray() {
  return m_pSA;
}

///////////////////////////////////////////////////////////////////////////////
// GetDimension : Get Dimemsion of the SafeArray
UINT CSafeArrayHelper::GetDimension() {
  UINT uDim = 0;
  if (NULL != m_pSA)
    uDim = SafeArrayGetDim(m_pSA);
  return uDim;
}

///////////////////////////////////////////////////////////////////////////////
// Attach : Attach an existing SafeArray to this object
bool CSafeArrayHelper::Attach(LPSAFEARRAY psa) {
  if (NULL != psa)  // prevent 'destroy' for null pointer
  {
    Destroy();
    m_pSA = psa;
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Detach: Detach this SafeArray
LPSAFEARRAY CSafeArrayHelper::Detach() {
  if (NULL == m_pSA)
    return NULL;

  LPSAFEARRAY psa = m_pSA;
  m_pSA = NULL;
  return psa;
}

///////////////////////////////////////////////////////////////////////////////
// AttachFromVariant: Attach a Variant SafeArray
bool CSafeArrayHelper::AttachFromVariant(VARIANT* pVariant) {
  if (NULL != pVariant) {
    if (pVariant->vt & VT_ARRAY) {
      LPSAFEARRAY psa = pVariant->parray;
      if (pVariant->vt & VT_BYREF)  // VB use this...
        psa = *pVariant->pparray;
      return Attach(psa);
    }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// GetAsVariant
// Description: Return a variant object of this SafeArray.  This function will
//				try to represent the data based on features or size of
// elements
VARIANT CSafeArrayHelper::GetAsVariant() {
  VARIANT var;
  VariantClear(&var);
  if (NULL != m_pSA) {
    var.vt = VT_ARRAY | VT_BYREF;
    var.pparray = &m_pSA;
    USHORT fFeatures = m_pSA->fFeatures;
    LONG cbElements = m_pSA->cbElements;
    if (fFeatures & FADF_BSTR)
      var.vt |= VT_BSTR;
    else if (fFeatures & FADF_UNKNOWN)
      var.vt |= VT_UNKNOWN;
    else if (fFeatures & FADF_DISPATCH)
      var.vt |= VT_DISPATCH;
    else if (fFeatures & FADF_VARIANT)
      var.vt |= VT_VARIANT;
    else if (fFeatures & FADF_RECORD)
      var.vt |= VT_RECORD;
    else if (fFeatures & FADF_HAVEVARTYPE) {
      // these are just guess based on size of element, may not be accurated...
      if (sizeof(char) == cbElements)  // 1 byte
        var.vt |= VT_UI1;
      else if (sizeof(short) == cbElements)  // 2 bytes
        var.vt |= VT_UI2;
      else if (sizeof(long) == cbElements)  // 4 bytes -- or float (VT_R4)
        var.vt |= VT_UI4;
      else if (sizeof(double) == cbElements)  // 8 bytes -- or Currency (VT_CY)
        var.vt |= VT_R8;
    }
  }
  return var;
}

///////////////////////////////////////////////////////////////////////////////
// AccessData : Get Access the SafeArray data
bool CSafeArrayHelper::AccessData(void FAR* FAR* pvData) {
  if (NULL != m_pSA && NULL != pvData) {
    return (SafeArrayAccessData(m_pSA, pvData) == S_OK);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// UnaccessData : Unaccess the safeArray data pointer
bool CSafeArrayHelper::UnaccessData() {
  if (NULL != m_pSA) {
    return (SafeArrayUnaccessData(m_pSA) == S_OK);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Lock : Lock the SafeArray
bool CSafeArrayHelper::Lock() {
  if (NULL != m_pSA) {
    return (SafeArrayLock(m_pSA) == S_OK);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Unlock : Unlock the SafeArray
bool CSafeArrayHelper::Unlock() {
  if (NULL != m_pSA) {
    return (SafeArrayUnlock(m_pSA) == S_OK);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// PutElement : Set element data
bool CSafeArrayHelper::PutElement(long lIndices, void FAR* vData) {
  if (NULL != m_pSA) {
    return (SafeArrayPutElement(m_pSA, &lIndices, vData) == S_OK);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// GetElement : Get element data
bool CSafeArrayHelper::GetElement(long lIndices, void FAR* vData) {
  if (NULL != m_pSA) {
    return (SafeArrayGetElement(m_pSA, &lIndices, vData) == S_OK);
  }
  return false;
}
