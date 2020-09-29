#pragma once
#include "cover_fovAspectBehaviour.h"
#include "lib/gl_structs.h"

struct CoverPosInfo {
  // When changing this you have to update CompiledCPInfo::version
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
                                  float bWeight);

 private:
  static inline float interpolF(float a, float b, float bWeight) {
    return a * (1 - bWeight) + b * bWeight;
  }
};

class CompiledCPInfo {
 public:
  static const int tableRes = 20;  // if you change this, you have to change version
  static const int version = 2;

  bool showMirrorPlane{};
  glVectord mirrorNormal;  // guaranteed to have length 1
  glVectord mirrorCenter;

  glVectord cameraPos;
  glVectord lookAt;
  glVectord upVector;

  int firstCover{};
  int lastCover{};

  fovAspectBehaviour aspectBehaviour{};

  bool enableCoverTitle{};
  bool enableCoverPngAlpha{};

  // this has to be the last Member!
  pfc::array_t<CoverPosInfo> coverPosInfos;

  CoverPosInfo getCoverPosInfo(float coverIdx) {
    float idx = coverIdx - firstCover;
    idx *= tableRes;
    int iPart = static_cast<int>(idx);
    float fPart = idx - iPart;

    return CoverPosInfo::interpolate(
        coverPosInfos[iPart], coverPosInfos[iPart + 1], fPart);
  }

  void serialize(stream_writer* p_stream, abort_callback& p_abort) {
    p_stream->write_lendian_t(version, p_abort);

    p_stream->write_object(  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        static_cast<void*>(this), offsetof(CompiledCPInfo, coverPosInfos), p_abort);

    t_size c = coverPosInfos.get_size();
    p_stream->write_lendian_t(c, p_abort);
    p_stream->write_object(
        static_cast<void*>(coverPosInfos.get_ptr()), sizeof(CoverPosInfo) * c, p_abort);
  }

  static void unserialize(CompiledCPInfo& out, stream_reader* p_stream,
                          abort_callback& p_abort) {
    int fileVer;
    p_stream->read_lendian_t(fileVer, p_abort);
    PFC_ASSERT(fileVer == version);
    p_stream->read_object(  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        static_cast<void*>(&out), offsetof(CompiledCPInfo, coverPosInfos), p_abort);

    t_size c;
    p_stream->read_lendian_t(c, p_abort);
    auto tmp = make_unique<CoverPosInfo[]>(c);
    p_stream->read_object(
        static_cast<void*>(tmp.get()), sizeof(CoverPosInfo) * c, p_abort);
    out.coverPosInfos.set_data_fromptr(tmp.get(), c);
  }
};

struct script_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
  static script_error from_com_error(const class _com_error&);
};

CompiledCPInfo compileCPScript(const char*);
