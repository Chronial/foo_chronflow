#pragma once
#include "CoverConfig.h"
#include "Helpers.h"
#include "ScriptObject.h"
#include "config.h"
#include "glStructs.h"

struct fovAspectBehaviour {
  float x;
  float y;
};

struct CoverPosInfo {  // When changing this you have to update CompiledCPInfo::version
  glVectorf position;

  struct {
    float a;
    glVectorf axis;
  } rotation;

  struct {
    float x;
    float y;
  } alignment;

  struct {
    float w;
    float h;
  } sizeLim;

  static CoverPosInfo interpolate(const CoverPosInfo& a, const CoverPosInfo& b,
                                  float bWeight) {
    CoverPosInfo out;

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

 private:
  static inline float interpolF(float a, float b, float bWeight) {
    return a * (1 - bWeight) + b * bWeight;
  }
};

class CompiledCPInfo {
  static const int tableRes = 20;  // if you change this, you have to change version

  friend class CPScriptCompiler;

 public:
  static const int version = 1;

  bool showMirrorPlane;
  glVectord mirrorNormal;  // guaranteed to have length 1
  glVectord mirrorCenter;

  glVectord cameraPos;
  glVectord lookAt;
  glVectord upVector;

  int firstCover;
  int lastCover;

  fovAspectBehaviour aspectBehaviour;

  // this has to be the last Member!
  pfc::array_t<CoverPosInfo> coverPosInfos;

  CoverPosInfo getCoverPosInfo(float coverIdx) {
    float idx = coverIdx - firstCover;
    idx *= tableRes;
    int iPart = static_cast<int>(idx);
    float fPart = idx - iPart;

    return CoverPosInfo::interpolate(coverPosInfos[iPart], coverPosInfos[iPart + 1],
                                     fPart);
  }

  void serialize(stream_writer* p_stream, abort_callback& p_abort) {
    p_stream->write_lendian_t(version, p_abort);

    p_stream->write_object((void*)this, offsetof(CompiledCPInfo, coverPosInfos), p_abort);

    t_size c = coverPosInfos.get_size();
    p_stream->write_lendian_t(c, p_abort);
    p_stream->write_object((void*)coverPosInfos.get_ptr(), sizeof(CoverPosInfo) * c,
                           p_abort);
  }
  static void unserialize(CompiledCPInfo& out, stream_reader* p_stream,
                          abort_callback& p_abort) {
    int fileVer;
    p_stream->read_lendian_t(fileVer, p_abort);
    PFC_ASSERT(fileVer == version);
    p_stream->read_object((void*)&out, offsetof(CompiledCPInfo, coverPosInfos), p_abort);

    t_size c;
    p_stream->read_lendian_t(c, p_abort);
    auto tmp = make_unique<CoverPosInfo[]>(c);
    p_stream->read_object((void*)tmp.get(), sizeof(CoverPosInfo) * c, p_abort);
    out.coverPosInfos.set_data_fromptr(tmp.get(), c);
  }
};

struct CPScriptFuncInfos {
  static const t_size funcCount = 12;
  static const wchar_t* knownFunctions[funcCount];

  static const bit_array_range neededFunctions;
  static const bit_array_range mirrorFunctions;
};

class CPScriptCompiler {
 public:
  CPScriptCompiler();
  bool compileScript(const char*, CompiledCPInfo& out, pfc::string_base& message);

 private:
  CScriptObject scriptObj;
  bool scShowMirrorPlane(bool& res, pfc::string_base& message);
  bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res,
                            pfc::string_base& message);
  bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res,
                            pfc::string_base& message, double param);
  bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res,
                            pfc::string_base& message, LPSAFEARRAY sa);
};

class cfg_compiledCPInfoPtr : public cfg_var {
 private:
  shared_ptr<CompiledCPInfo> data_;

 protected:
  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
    auto data = this->get();
    if (data) {
      p_stream->write_lendian_t(true, p_abort);
      data->serialize(p_stream, p_abort);
    } else {
      p_stream->write_lendian_t(false, p_abort);
    }
  }
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
    bool configNotEmpty;
    p_stream->read_lendian_t(configNotEmpty, p_abort);
    if (configNotEmpty) {
      auto data = make_shared<CompiledCPInfo>();
      CompiledCPInfo::unserialize(*data, p_stream, p_abort);
      this->set(data);
    } else {
      this->reset();
    }
  }

 public:
  shared_ptr<CompiledCPInfo> get() const { return std::atomic_load(&this->data_); }
  void set(shared_ptr<CompiledCPInfo> value) {
    std::atomic_store(&this->data_, std::move(value));
  }
  void reset() { std::atomic_store(&this->data_, {}); }

  inline cfg_compiledCPInfoPtr(const GUID& p_guid) : cfg_var(p_guid) {}
};

extern cfg_compiledCPInfoPtr sessionCompiledCPInfo;

class ScriptedCoverPositions {
 public:
  ScriptedCoverPositions() {
    cInfo = sessionCompiledCPInfo.get();
    if (!cInfo) {
      const CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
      pfc::string8 errorMsg;
      if (!config || !setScript(config->script, errorMsg)) {
        if (!setScript(defaultCoverConfig, errorMsg)) {
          popup_message::g_show(errorMsg, "JScript Compile Error",
                                popup_message::icon_error);
          throw new pfc::exception(errorMsg);
        }
      }
    }
  }

  bool setScript(const char* script, pfc::string_base& errorMsg) {
    try {
      auto compiled = make_shared<CompiledCPInfo>();
      CPScriptCompiler compiler;
      if (compiler.compileScript(script, *compiled, errorMsg)) {
        sessionCompiledCPInfo.set(compiled);
        cInfo = std::move(compiled);
        return true;
      } else {
        return false;
      }
    } catch (_com_error) {
      errorMsg =
          "Windows Script Control not installed. Download it from "
          "<http://www.microsoft.com/downloads/"
          "details.aspx?FamilyId=D7E31492-2595-49E6-8C02-1426FEC693AC>.";
      return false;
    }
  }
  inline const fovAspectBehaviour& getAspectBehaviour() { return cInfo->aspectBehaviour; }

  inline const glVectord& getLookAt() { return cInfo->lookAt; }

  inline const glVectord& getUpVector() { return cInfo->upVector; }

  inline bool isMirrorPlaneEnabled() { return cInfo->showMirrorPlane; }

  inline const glVectord& getMirrorNormal() { return cInfo->mirrorNormal; }
  inline const glVectord& getMirrorCenter() { return cInfo->mirrorCenter; }
  inline const glVectord& getCameraPos() { return cInfo->cameraPos; }
  inline const int getFirstCover() { return cInfo->firstCover; }
  inline const int getLastCover() { return cInfo->lastCover; }

  double distanceToMirror(glVectord point) {
    return abs((cInfo->mirrorCenter - point) * cInfo->mirrorNormal);
  }

  glQuad getCoverQuad(float coverId, float coverAspect) {
    CoverPosInfo cPos = cInfo->getCoverPosInfo(coverId);

    double sizeLimAspect = cPos.sizeLim.w / cPos.sizeLim.h;

    float w;
    float h;
    if (coverAspect > sizeLimAspect) {
      w = cPos.sizeLim.w;
      h = w / coverAspect;
    } else {
      h = cPos.sizeLim.h;
      w = h * coverAspect;
    }

    glQuad out;

    // out.topLeft.x = -w/2 -align.x * w/2;
    out.topLeft.x = (-1 - cPos.alignment.x) * w / 2;
    out.bottomLeft.x = out.topLeft.x;
    out.topRight.x = (1 - cPos.alignment.x) * w / 2;
    out.bottomRight.x = out.topRight.x;

    out.topLeft.y = (1 - cPos.alignment.y) * h / 2;
    out.topRight.y = out.topLeft.y;
    out.bottomLeft.y = (-1 - cPos.alignment.y) * h / 2;
    out.bottomRight.y = out.bottomLeft.y;

    out.topLeft.z = 0;
    out.topRight.z = 0;
    out.bottomLeft.z = 0;
    out.bottomRight.z = 0;

    out.rotate(cPos.rotation.a, cPos.rotation.axis);

    out.topLeft = cPos.position + out.topLeft;
    out.topRight = cPos.position + out.topRight;
    out.bottomLeft = cPos.position + out.bottomLeft;
    out.bottomRight = cPos.position + out.bottomRight;

    return out;
  }

 private:
  shared_ptr<CompiledCPInfo> cInfo;
};
