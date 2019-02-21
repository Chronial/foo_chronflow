#include "cover_positions_compiler.h"

#include <comutil.h>
#include <boost/range/adaptor/indexed.hpp>

#include "lib/script_control.h"

#include "utils.h"

namespace {

VARIANT getIDispatchProperty(LPDISPATCH dispatch, LPOLESTR propertyName) {
  DISPID dispid = 0;
  _com_util::CheckError(dispatch->GetIDsOfNames(
      IID_NULL, &propertyName, 1, LOCALE_SYSTEM_DEFAULT, &dispid));

  DISPPARAMS dispparamsNoArgs = {nullptr, nullptr, 0, 0};
  EXCEPINFO excepinfo;
  UINT nArgErr;
  VARIANT out;
  _com_util::CheckError(dispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                         DISPATCH_PROPERTYGET, &dispparamsNoArgs, &out,
                                         &excepinfo, &nArgErr));
  return out;
};

template <int vector_size>
std::array<double, vector_size> scoVectorFun(CScriptObject& scriptObj,
                                             const wchar_t* func, LPSAFEARRAY sa) {
  _variant_t varRet = scriptObj.RunProcedure(func, sa);
  if (varRet.vt != VT_DISPATCH) {
    throw script_error(PFC_string_formatter()
                       << "Error: " << Tu(func) << "() did not return an array.");
  }
  LPDISPATCH dispatch = varRet.pdispVal;
  int length;
  try {
    _variant_t lVar = getIDispatchProperty(dispatch, L"length");
    if (lVar.vt != VT_I4)
      lVar.ChangeType(VT_I4);
    length = static_cast<int>(lVar.lVal);
  } catch (...) {
    throw script_error(PFC_string_formatter()
                       << "Error: " << Tu(func) << "() did not return an array.");
  }
  if (length != vector_size) {
    throw script_error(PFC_string_formatter()
                       << "Error: " << Tu(func) << "()"
                       << " did not return an array with " << vector_size << " elements");
  }
  std::array<double, vector_size> res{};
  for (int i = 0; i < vector_size; i++) {
    try {
      _variant_t eVar = getIDispatchProperty(dispatch, std::to_wstring(i).data());
      eVar.ChangeType(VT_R8);
      res[i] = eVar.dblVal;
    } catch (...) {
      throw script_error(PFC_string_formatter()
                         << "Error: " << Tu(func) << "()"
                         << " did not return an array of numbers.");
    }
    if (_isnan(res[i])) {
      throw script_error(PFC_string_formatter()
                         << "Error: " << Tu(func) << "()"
                         << " returned an array with element at index " << i
                         << " beeing NAN.");
    } else if (res[i] > std::numeric_limits<double>::max() ||
               res[i] < -std::numeric_limits<double>::max()) {
      throw script_error(PFC_string_formatter()
                         << "Error: " << Tu(func) << "()"
                         << " returned an array with element at index " << i
                         << " beeing out of range for double values.");
    }
  }
  return res;
}

template <int vector_size>
std::array<double, vector_size> scoVectorFun(CScriptObject& scriptObj,
                                             const wchar_t* func, double param) {
  CSafeArrayHelper sfHelper;
  sfHelper.Create(VT_VARIANT, 1, 0, 1);
  _variant_t var;
  var.vt = VT_R8;
  var.dblVal = param;
  sfHelper.PutElement(0, reinterpret_cast<void*>(&var));
  LPSAFEARRAY sa = sfHelper.GetArray();
  return scoVectorFun<vector_size>(scriptObj, func, sa);
}

template <int vector_size>
std::array<double, vector_size> scoVectorFun(CScriptObject& scriptObj,
                                             const wchar_t* func) {
  CSafeArrayHelper sfHelper;
  sfHelper.Create(VT_VARIANT, 1, 0, 0);
  LPSAFEARRAY sa = sfHelper.GetArray();
  return scoVectorFun<vector_size>(scriptObj, func, sa);
}

bool scoShowMirrorPlane(CScriptObject& scriptObj) {
  CSafeArrayHelper sfHelper;
  sfHelper.Create(VT_VARIANT, 1, 0, 0);
  LPSAFEARRAY sa = sfHelper.GetArray();
  _variant_t varRet = scriptObj.RunProcedure(L"showMirrorPlane", sa);
  varRet.ChangeType(VT_BOOL);
  return varRet.boolVal != 0;
}

}  // namespace

CompiledCPInfo compileCPScript(const char* script) {
#pragma warning(push)
// 4244: conversion from 'double' to 'float', possible loss of data
#pragma warning(disable : 4244)
  try {
    CScriptObject scriptObj(uT(script));
    CompiledCPInfo out;

    std::tie(out.cameraPos.x, out.cameraPos.y, out.cameraPos.z) =
        array2tuple(scoVectorFun<3>(scriptObj, L"eyePos"));

    std::tie(out.lookAt.x, out.lookAt.y, out.lookAt.z) =
        array2tuple(scoVectorFun<3>(scriptObj, L"lookAt"));

    std::tie(out.upVector.x, out.upVector.y, out.upVector.z) =
        array2tuple(scoVectorFun<3>(scriptObj, L"upVector"));

    std::tie(out.aspectBehaviour.x, out.aspectBehaviour.y) =
        array2tuple(scoVectorFun<2>(scriptObj, L"aspectBehaviour"));
    out.aspectBehaviour.x /= out.aspectBehaviour.x + out.aspectBehaviour.y;
    out.aspectBehaviour.y /= out.aspectBehaviour.x + out.aspectBehaviour.y;

    out.showMirrorPlane = scoShowMirrorPlane(scriptObj);
    if (out.showMirrorPlane) {
      std::tie(out.mirrorCenter.x, out.mirrorCenter.y, out.mirrorCenter.z) =
          array2tuple(scoVectorFun<3>(scriptObj, L"mirrorPoint"));

      std::tie(out.mirrorNormal.x, out.mirrorNormal.y, out.mirrorNormal.z) =
          array2tuple(scoVectorFun<3>(scriptObj, L"mirrorNormal"));
      out.mirrorNormal = out.mirrorNormal.normalize();
    }

    auto coverRange = scoVectorFun<2>(scriptObj, L"drawCovers");
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
          array2tuple(scoVectorFun<3>(scriptObj, L"coverPosition", coverId));

      std::tie(
          pi.rotation.a, pi.rotation.axis.x, pi.rotation.axis.y, pi.rotation.axis.z) =
          array2tuple(scoVectorFun<4>(scriptObj, L"coverRotation", coverId));
      pi.rotation.a = deg2rad(pi.rotation.a);

      std::tie(pi.alignment.x, pi.alignment.y) =
          array2tuple(scoVectorFun<2>(scriptObj, L"coverAlign", coverId));

      std::tie(pi.sizeLim.w, pi.sizeLim.h) =
          array2tuple(scoVectorFun<2>(scriptObj, L"coverSizeLimits", coverId));
    }
    return out;
  } catch (CScriptError& e) {
    throw script_error(e.what());
  }
#pragma warning(pop)
}
