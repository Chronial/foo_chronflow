#include "script_control.h"

#include <comutil.h>

#define Tu(x) (pfc::stringcvt::string_utf8_from_os(x).get_ptr())

CScriptObject::CScriptObject(const wchar_t* code) {
  try {
    _com_util::CheckError(m_pScript.CreateInstance(__uuidof(ScriptControl)));
    m_pScript->PutAllowUI(VARIANT_FALSE);
    m_pScript->PutLanguage(L"JScript");
    _com_util::CheckError(m_pScript->AddCode(code));
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

VARIANT CScriptObject::RunProcedure(LPCTSTR szProcName, SAFEARRAY* saParameters) {
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

CSafeArrayHelper::CSafeArrayHelper(UINT size)
    : array(SafeArrayCreateVector(VT_VARIANT, 0, size)) {
  if (array == nullptr)
    throw std::bad_alloc{"SafeArrayCreateVector() failed"};
}

CSafeArrayHelper::~CSafeArrayHelper() {
  SafeArrayDestroy(array);
}

LPSAFEARRAY CSafeArrayHelper::GetArray() const {
  return array;
}

void CSafeArrayHelper::PutElement(long idx, _variant_t& vData) {
  _com_util::CheckError(SafeArrayPutElement(array, &idx, static_cast<void*>(&vData)));
}
