#include "cover_positions_compiler.h"

#include <atlsafe.h>
#include <comdef.h>
#include <comutil.h>

//#import "msscript.ocx" no_namespace
#include "lib/msscript.h"
#include "utils.h"

CoverPosInfo CoverPosInfo::interpolate(const CoverPosInfo& a, const CoverPosInfo& b,
                                       float bWeight) {
  CoverPosInfo out{};

  out.position.x = interpolF(a.position.x, b.position.x, bWeight);
  out.position.y = interpolF(a.position.y, b.position.y, bWeight);
  out.position.z = interpolF(a.position.z, b.position.z, bWeight);

  out.rotation.a = interpolF(a.rotation.a, b.rotation.a, bWeight);
  out.rotation.axis.x = interpolF(a.rotation.axis.x, b.rotation.axis.x, bWeight);
  out.rotation.axis.y = interpolF(a.rotation.axis.y, b.rotation.axis.y, bWeight);
  out.rotation.axis.z = interpolF(a.rotation.axis.z, b.rotation.axis.z, bWeight);

  out.alignment.x = interpolF(a.alignment.x, b.alignment.x, bWeight);
  out.alignment.y = interpolF(a.alignment.y, b.alignment.y, bWeight);

  out.sizeLim.w = interpolF(a.sizeLim.w, b.sizeLim.w, bWeight);
  out.sizeLim.h = interpolF(a.sizeLim.h, b.sizeLim.h, bWeight);

  return out;
}

script_error script_error::from_com_error(const _com_error& e) {
  auto desc = e.Description();
  if (desc.GetBSTR() != nullptr) {
    return script_error(PFC_string_formatter() << "COM Error: " << Tu(desc.GetBSTR()));
  } else {
    return script_error(PFC_string_formatter() << "COM Error: " << Tu(e.ErrorMessage()));
  }
}

namespace {
class CScriptObject {
public:
  CScriptObject(const wchar_t* code) {
    try {
      _com_util::CheckError(script_control.CreateInstance(__uuidof(ScriptControl)));
      script_control->PutAllowUI(VARIANT_FALSE);
      script_control->PutLanguage(L"JScript");
      _com_util::CheckError(script_control->AddCode(code));
    } catch (_com_error& e) {
      rethrow_error(e);
    }
  };

public:
  [[noreturn]] void rethrow_error(const _com_error& e) {
    try {
      IScriptErrorPtr pError;
      if (script_control != nullptr)
        pError = script_control->GetError();
      if (pError == nullptr || pError->GetText().GetBSTR() == nullptr) {
        throw script_error::from_com_error(e);
      }
      throw script_error(PFC_string_formatter()
                         << "Error: " << Tu(pError->GetDescription()) << ", "
                         << Tu(pError->GetText()) << "; in line " << pError->GetLine());
    } catch (_com_error& e) {
      throw script_error::from_com_error(e);
    }
  };

  template <typename Ret, typename... _Types>
  Ret call(const char* func, _Types ... _Args) {
    std::tuple tuple = std::make_tuple(_Args...);
    ULONG length = sizeof...(_Args);
    CComSafeArray<VARIANT> params{length};

    long i = 0;
    for_each(tuple, [&](auto p)
    {
      CComVariant v(p);
      if (S_OK != params.SetAt(i, v))
        throw std::bad_alloc{};
      i++;
    });

    try {
      return Ret(script_control->Run(uT(func), params.GetSafeArrayPtr()));
    } catch (_com_error& e) {
      if (e.Error() == DISP_E_UNKNOWNNAME) {
        throw script_error(
            PFC_string_formatter() << "Error: Function " << func << "() not found.");
      } else {
        rethrow_error(e);
      }
    }
  };

  template <int vector_size, typename... Types>
  std::array<double, vector_size> call_double_array(const char* func, Types ... Args) {
    _variant_t varRet = call<_variant_t>(func, Args...);
    try {
      CComPtr<IDispatch> dispatch(varRet);
      long length;
      _variant_t lVar;
      _com_util::CheckError(dispatch.GetPropertyByName(L"length", &lVar));
      length = long(lVar);
      if (length != vector_size)
        throw std::exception{};
      std::array<double, vector_size> res{};
      for (int i = 0; i < vector_size; i++) {
        _variant_t eVar;
        _com_util::CheckError(
            dispatch.GetPropertyByName(std::to_wstring(i).data(), &eVar));
        res[i] = double(eVar);
        if (!(res[i] < std::numeric_limits<float>::max() &&
              res[i] > -std::numeric_limits<float>::max())) {
          throw std::exception{};
        }
      }
      return res;
    } catch (...) {
      throw script_error(
          PFC_string_formatter() << "Error: " << func << "() did not return an array "
          << "of " << vector_size << " valid numbers.");
    }
  }

  template <int len, typename... Types>
  auto call_double_tuple(const char* func, Types ... Args) {
    return array2tuple(call_double_array<len>(func, Args...));
  }

private:
  IScriptControlPtr script_control;
};

} // namespace

CompiledCPInfo compileCPScript(const char* script) {
#pragma warning(push)
// 4244: conversion from 'double' to 'float', possible loss of data
#pragma warning(disable : 4244)
  CScriptObject scriptObj(uT(script));
  CompiledCPInfo out;

  std::tie(out.cameraPos.x, out.cameraPos.y, out.cameraPos.z) =
      scriptObj.call_double_tuple<3>("eyePos");

  std::tie(out.lookAt.x, out.lookAt.y, out.lookAt.z) =
      scriptObj.call_double_tuple<3>("lookAt");

  std::tie(out.upVector.x, out.upVector.y, out.upVector.z) =
      scriptObj.call_double_tuple<3>("upVector");

  std::tie(out.aspectBehaviour.x, out.aspectBehaviour.y) =
      scriptObj.call_double_tuple<2>("aspectBehaviour");
  out.aspectBehaviour.x /= out.aspectBehaviour.x + out.aspectBehaviour.y;
  out.aspectBehaviour.y /= out.aspectBehaviour.x + out.aspectBehaviour.y;

  out.showMirrorPlane = scriptObj.call<bool>("showMirrorPlane");
  if (out.showMirrorPlane) {
    std::tie(out.mirrorCenter.x, out.mirrorCenter.y, out.mirrorCenter.z) =
        scriptObj.call_double_tuple<3>("mirrorPoint");

    std::tie(out.mirrorNormal.x, out.mirrorNormal.y, out.mirrorNormal.z) =
        scriptObj.call_double_tuple<3>("mirrorNormal");
    out.mirrorNormal = out.mirrorNormal.normalize();
  }

  auto coverRange = scriptObj.call_double_array<2>("drawCovers");
  out.firstCover = std::min(-1, static_cast<int>(coverRange[0]));
  out.lastCover = std::max(1, static_cast<int>(coverRange[1]));
  int coverCount = out.lastCover - out.firstCover + 1;
  if (coverCount < 2) {
    throw script_error(
        "Error: drawCovers() did return an interval that contained less than 2 "
        "elements");
  }

  int tableSize = coverCount * CompiledCPInfo::tableRes + 1;
  out.coverPosInfos.set_size(tableSize);
  for (int i = 0; i < tableSize; i++) {
    auto& pi = out.coverPosInfos[i];
    double coverId = static_cast<double>(i) / CompiledCPInfo::tableRes + out.firstCover;

    std::tie(pi.position.x, pi.position.y, pi.position.z) =
        scriptObj.call_double_tuple<3>("coverPosition", coverId);

    std::tie(pi.rotation.a, pi.rotation.axis.x, pi.rotation.axis.y, pi.rotation.axis.z) =
        scriptObj.call_double_tuple<4>("coverRotation", coverId);
    pi.rotation.a = deg2rad(pi.rotation.a);

    std::tie(pi.alignment.x, pi.alignment.y) =
        scriptObj.call_double_tuple<2>("coverAlign", coverId);

    std::tie(pi.sizeLim.w, pi.sizeLim.h) =
        scriptObj.call_double_tuple<2>("coverSizeLimits", coverId);
  }
  return out;
#pragma warning(pop)
}

//} // namespace
