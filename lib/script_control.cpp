#include "script_control.h"

#include <comutil.h>

#define Tu(x) (pfc::stringcvt::string_utf8_from_os(x).get_ptr())

CScriptObject::CScriptObject(const wchar_t* code) {
  try {
    _com_util::CheckError(m_pScript.CreateInstance(__uuidof(ScriptControl)));
    m_pScript->PutAllowUI(VARIANT_FALSE);
    m_pScript->PutLanguage(L"JScript");
    m_pScript->AddCode(code);
  } catch (_com_error& e) {
    RethrowError(e);
  }
}

pfc::string8 comErrorDesc(const _com_error& e) {
  auto desc = e.Description();
  if (desc.GetBSTR() != nullptr) {
    return Tu(desc.GetBSTR());
  } else {
    return Tu(e.ErrorMessage());
  }
}

[[noreturn]] void CScriptObject::RethrowError(const _com_error& e) {
  try {
    IScriptErrorPtr pError;
    if (m_pScript != nullptr)
      pError = m_pScript->GetError();
    if (pError == nullptr || pError->GetText().GetBSTR() == nullptr) {
      throw CScriptError(PFC_string_formatter() << "COM Error: " << comErrorDesc(e));
    }
    throw CScriptError(PFC_string_formatter()
                       << "Error: " << Tu(pError->GetDescription()) << ", "
                       << Tu(pError->GetText()) << "; in line " << pError->GetLine());
  } catch (_com_error& e) {
    throw CScriptError(PFC_string_formatter() << "FATAL COM Error: " << comErrorDesc(e));
  }
}

std::vector<std::wstring> CScriptObject::GetMethodNames() {
  try {
    std::vector<std::wstring> methods{};
    IScriptProcedureCollectionPtr pIProcedures = m_pScript->GetProcedures();
    IScriptProcedurePtr pIProcPtr{};
    long count = pIProcedures->GetCount();
    for (int i = 1; i <= count; i++) {
      pIProcPtr = pIProcedures->GetItem(_variant_t(i));
      methods.emplace_back(pIProcPtr->GetName().GetBSTR());
    }
    return methods;
  } catch (_com_error& e) {
    RethrowError(e);
  }
}

VARIANT CScriptObject::RunProcedure(LPCTSTR szProcName, SAFEARRAY*& saParameters) {
  try {
    return m_pScript->Run(szProcName, &saParameters);
  } catch (_com_error& e) {
    if (e.Error() == DISP_E_UNKNOWNNAME) {
      throw CScriptError(PFC_string_formatter()
                         << "Error: Function " << Tu(szProcName) << "() not found.");
    } else {
      RethrowError(e);
    }
  }
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
    SAFEARRAYBOUND rgsabound[1] = {{0}};
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
