#include "ScriptedCoverPositions.h"

#include <comutil.h>

#include "base.h"
#include "SafeArrayHelper.h"

using namespace pfc;

CPScriptCompiler::CPScriptCompiler()
{
	scriptObj.SetLanguage(L"JScript");
}

static inline bool GetIDispatchProperty (const LPDISPATCH _lpDispatch, LPWSTR _Name, VARIANT& _ExVariant)
{
   bool Ret = false;
   DISPID dispid = 0;
   OLECHAR FAR* szMember = _Name;

   HRESULT Hr = _lpDispatch->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid);

   if(Hr == S_OK)
   {
      //VARIANT pVarResult;

      DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
      EXCEPINFO excepinfo;
      UINT nArgErr;

      Ret = true;

      try
      {
         Hr = _lpDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
            DISPATCH_PROPERTYGET,
         &dispparamsNoArgs, &_ExVariant, &excepinfo, &nArgErr);
      }
      catch(...)
      {
         /*CString TString;
         DebugString += "catch<BR>rn";
         TString.Format("nArgErr = %lu", nArgErr);
         DebugString += TString + "<BR>rn";
         TString.Format("wCode = %lu", excepinfo.wCode);
         DebugString += TString + "<BR>rn";*/
         Ret = false;
      }
      /*CComVariant ExVariant(pVarResult);

      _ExVariant = ExVariant;*/

   }

   //delete szMember;
   return Ret;
};

const wchar_t* CPScriptFuncInfos::knownFunctions[CPScriptFuncInfos::funcCount]= {L"eyePos", L"lookAt", L"upVector", L"coverPosition", L"coverRotation",
		L"coverAlign", L"coverSizeLimits", L"drawCovers",
		L"aspectBehaviour",
		L"showMirrorPlane", L"mirrorPoint", L"mirrorNormal"};

const bit_array_range CPScriptFuncInfos::neededFunctions(0, 10);
const bit_array_range CPScriptFuncInfos::mirrorFunctions(10, 2);

bool CPScriptCompiler::compileScript(const char * script, CompiledCPInfo& out, pfc::string_base& message){
	try {
		scriptObj.Reset();
		if (!scriptObj.AddScript(pfc::stringcvt::string_wide_from_utf8(script))){
			message = pfc::stringcvt::string_utf8_from_wide(scriptObj.GetErrorString());
			return false;
		}

		bit_array_bittable foundFunctions(CPScriptFuncInfos::funcCount);
		bit_array_not notFoundFunctions(foundFunctions);
		int mCount = scriptObj.GetMethodsCount();
		for (int i = 0; i < mCount; i++){
			const wchar_t* mName = scriptObj.GetNameAt(i);
			for (int j = 0; j < CPScriptFuncInfos::funcCount; j++){
				if (!wcscmp(mName, CPScriptFuncInfos::knownFunctions[j])){
					foundFunctions.set(j, true);
					break;
				}
			}
		}
		bit_array_and missingFunctions(CPScriptFuncInfos::neededFunctions, notFoundFunctions);
		t_size missing = missingFunctions.find(true, 0, CPScriptFuncInfos::funcCount);
		if (missing == CPScriptFuncInfos::funcCount){
			if (!scShowMirrorPlane(out.showMirrorPlane, message))
				return false;
			if (out.showMirrorPlane){
				bit_array_and missingMirrors(CPScriptFuncInfos::mirrorFunctions, notFoundFunctions);
				missing = missingMirrors.find(true, 0, CPScriptFuncInfos::funcCount);
			}
		}
		if (missing < CPScriptFuncInfos::funcCount){
			message = "Error: Required Function \"";
			message << pfc::stringcvt::string_utf8_from_wide(CPScriptFuncInfos::knownFunctions[missing]);
			message << "\" is not defined.";
			return false;
		}
		
		pfc::list_t<double> ret;
		if (scCallDArrayFunction(L"eyePos", ret, message)){
			if (ret.get_count() != 3){
				message = "Error: eyePos() did not return an Array with 3 elements";
				return false;
			} else {
				out.cameraPos.x = ret.get_item(0);
				out.cameraPos.y = ret.get_item(1);
				out.cameraPos.z = ret.get_item(2);
			}
		} else {
			return false;
		}

		if (scCallDArrayFunction(L"lookAt", ret, message)){
			if (ret.get_count() != 3){
				message = "Error: lookAt() did not return an Array with 3 elements";
				return false;
			} else {
				out.lookAt.x = ret.get_item(0);
				out.lookAt.y = ret.get_item(1);
				out.lookAt.z = ret.get_item(2);
			}
		} else {
			return false;
		}

		if (scCallDArrayFunction(L"upVector", ret, message)){
			if (ret.get_count() != 3){
				message = "Error: upVector() did not return an Array with 3 elements";
				return false;
			} else {
				out.upVector.x = ret.get_item(0);
				out.upVector.y = ret.get_item(1);
				out.upVector.z = ret.get_item(2);
			}
		} else {
			return false;
		}

		if (scCallDArrayFunction(L"aspectBehaviour", ret, message)){
			if (ret.get_count() != 2){
				message = "Error: aspectBehaviour() did not return an Array with 2 elements";
				return false;
			} else {
				out.aspectBehaviour.x = (float)ret.get_item(0);
				out.aspectBehaviour.y = (float)ret.get_item(1);
				out.aspectBehaviour.x /= out.aspectBehaviour.x + out.aspectBehaviour.y;
				out.aspectBehaviour.y /= out.aspectBehaviour.x + out.aspectBehaviour.y;
			}
		} else {
			return false;
		}


		if (out.showMirrorPlane){
			if (scCallDArrayFunction(L"mirrorPoint", ret, message)){
				if (ret.get_count() != 3){
					message = "Error: mirrorPoint() did not return an Array with 3 elements";
					return false;
				} else {
					out.mirrorCenter.x = ret.get_item(0);
					out.mirrorCenter.y = ret.get_item(1);
					out.mirrorCenter.z = ret.get_item(2);
				}
			} else {
				return false;
			}
			if (scCallDArrayFunction(L"mirrorNormal", ret, message)){
				if (ret.get_count() != 3){
					message = "Error: mirrorNormal() did not return an Array with 3 elements";
					return false;
				} else {
					out.mirrorNormal.x = ret.get_item(0);
					out.mirrorNormal.y = ret.get_item(1);
					out.mirrorNormal.z = ret.get_item(2);
					out.mirrorNormal = out.mirrorNormal.normalize();
				}
			} else {
				return false;
			}
		}

		if (scCallDArrayFunction(L"drawCovers", ret, message)){
			if (ret.get_count() != 2){
				message = "Error: drawCovers() did not return an Array with 2 elements";
				return false;
			} else {
				out.firstCover = static_cast<int>(ret.get_item(0));
				if (out.firstCover > -1)
					out.firstCover = -1;
				out.lastCover  = static_cast<int>(ret.get_item(1));
				if (out.lastCover < 1)
					out.lastCover = 1;

			}
		} else {
			return false;
		}
		
		int coverCount = out.lastCover - out.firstCover + 1;
		if (coverCount < 2){
			message = "Error: drawCovers() did return an interval that contained less than 2 elements";
			return false;
		}
		int tableSize = coverCount * out.tableRes + 1;
		out.coverPosInfos.set_size(tableSize);
		for (int i = 0; i < tableSize; i++){
			double coverId = static_cast<double>(i) / out.tableRes + out.firstCover;
			if (scCallDArrayFunction(L"coverPosition", ret, message, coverId)){
				if (ret.get_count() != 3){
					message = "Error: coverPosition() did not return an Array with 3 elements";
					return false;
				} else {
					out.coverPosInfos[i].position.x = (float)ret.get_item(0);
					out.coverPosInfos[i].position.y = (float)ret.get_item(1);
					out.coverPosInfos[i].position.z = (float)ret.get_item(2);
				}
			} else {
				return false;
			}
			if (scCallDArrayFunction(L"coverRotation", ret, message, coverId)){
				if (ret.get_count() != 4){
					message = "Error: coverRotation() did not return an Array with 4 elements";
					return false;
				} else {
					out.coverPosInfos[i].rotation.a = (float)deg2rad(ret.get_item(0));
					out.coverPosInfos[i].rotation.axis.x = (float)ret.get_item(1);
					out.coverPosInfos[i].rotation.axis.y = (float)ret.get_item(2);
					out.coverPosInfos[i].rotation.axis.z = (float)ret.get_item(3);
				}
			} else {
				return false;
			}
			if (scCallDArrayFunction(L"coverAlign", ret, message, coverId)){
				if (ret.get_count() != 2){
					message = "Error: coverAlign() did not return an Array with 4 elements";
					return false;
				} else {
					out.coverPosInfos[i].alignment.x = (float)ret.get_item(0);
					out.coverPosInfos[i].alignment.y = (float)ret.get_item(1);
				}
			} else {
				return false;
			}
			if (scCallDArrayFunction(L"coverSizeLimits", ret, message, coverId)){
				if (ret.get_count() != 2){
					message = "Error: coverSizeLimits() did not return an Array with 4 elements";
					return false;
				} else {
					out.coverPosInfos[i].sizeLim.w = (float)ret.get_item(0);
					out.coverPosInfos[i].sizeLim.h = (float)ret.get_item(1);
				}
			} else {
				return false;
			}
		}
		return true;
	} catch (_com_error& e){
		message = "Com Exception: ";
		message << pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage());
		return false;
	} catch (...) {
		message = "Unknown exception during compiling";
		return false;
	}
}

bool CPScriptCompiler::scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message){
	CSafeArrayHelper sfHelper;
	sfHelper.Create(VT_VARIANT, 1, 0, 0);
	LPSAFEARRAY sa = sfHelper.GetArray();
	return scCallDArrayFunction(func, res, message, sa);
}
bool CPScriptCompiler::scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message, double param){
	CSafeArrayHelper sfHelper;
	sfHelper.Create(VT_VARIANT, 1, 0, 1);
	_variant_t var;
	var.vt = VT_R8;
	var.dblVal = param;
	sfHelper.PutElement(0, (void*)&var);
	LPSAFEARRAY sa = sfHelper.GetArray();
	return scCallDArrayFunction(func, res, message, sa);
}
bool CPScriptCompiler::scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message, LPSAFEARRAY sa){
	try {
		_variant_t varRet;
		int length;
		if (scriptObj.RunProcedure(func, &sa, &varRet)){
			if (varRet.vt == VT_DISPATCH){
				LPDISPATCH lpDispatch = varRet.pdispVal;
				_variant_t lVar;
				if(GetIDispatchProperty(lpDispatch, L"length", lVar))
				{
					PFC_ASSERT(lVar.vt == VT_I4);
					if(lVar.vt != VT_I4)
						lVar.ChangeType(VT_I4);
					length = (int) lVar.lVal;
					res.remove_all();
					_variant_t eVar;
					wchar_t id[32];
					for (int i = 0; i < length; i++){
						swprintf_s(id, L"%d", i);
						GetIDispatchProperty(lpDispatch, id, eVar);
						try {
							eVar.ChangeType(VT_R8);
							if (_isnan(eVar.dblVal)){
								message.reset();
								message << "Error: The function \"" << pfc::stringcvt::string_utf8_from_wide(func)
									<< "\" returned an array with element at index " << i << " beeing NAN.";
								return false;
							} else if (eVar.dblVal > std::numeric_limits<double>::max() ||
								eVar.dblVal < -std::numeric_limits<double>::max()){
								message.reset();
								message << "Error: The function \"" << pfc::stringcvt::string_utf8_from_wide(func)
									<< "\" returned an array with element at index " << i << " beeing out of range for double values.";
								return false;
							}
							res.add_item(eVar.dblVal);
						} catch (...) {
							message.reset();
							message << "Error: The function \"" << pfc::stringcvt::string_utf8_from_wide(func)
								<< "\" did not return an array of numbers.";
							return false;
						}
					}
					return true;
				} else {
					message = "Unknown Internal Error";
				}
			} else {
				message.reset();
				message << "Error: The function \"" << pfc::stringcvt::string_utf8_from_wide(func)
						<< "\" did not return an array.";
			}
		} else {
			message = pfc::stringcvt::string_utf8_from_wide(scriptObj.GetErrorString());
		}
	} catch(_com_error& e) {
		message = pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage());
	} catch(...) {
		message = pfc::stringcvt::string_utf8_from_wide(scriptObj.GetErrorString());
	}
	return false;
}

bool CPScriptCompiler::scShowMirrorPlane(bool& res, pfc::string_base& message){
	try {
		CSafeArrayHelper sfHelper;
		sfHelper.Create(VT_VARIANT, 1, 0, 0);
		LPSAFEARRAY sa = sfHelper.GetArray();
		_variant_t varRet;
		if (scriptObj.RunProcedure(L"showMirrorPlane", &sa, &varRet)){
			varRet.ChangeType(VT_BOOL);
			if (varRet.boolVal){
				res = true;
			} else {
				res = false;
			}
			return true;
		} else {
			message = pfc::stringcvt::string_utf8_from_wide(scriptObj.GetErrorString());
		}
	} catch(_com_error& e) {
		message = pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage());
	} catch(...) {
		message = pfc::stringcvt::string_utf8_from_wide(scriptObj.GetErrorString());
	}
	return false;
}
