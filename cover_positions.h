#pragma once
#include "lib/gl_structs.h"

#include "CoverConfig.h"
#include "Helpers.h"

class CompiledCPInfo;

struct fovAspectBehaviour {
  float x;
  float y;
};

class cfg_compiledCPInfoPtr : public cfg_var {
 private:
  shared_ptr<CompiledCPInfo> data_;

 protected:
  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) final;
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                    abort_callback& p_abort) final;

 public:
  shared_ptr<CompiledCPInfo> get() const { return std::atomic_load(&this->data_); }
  void set(shared_ptr<CompiledCPInfo> value) {
    std::atomic_store(&this->data_, std::move(value));
  }
  void reset() { std::atomic_store(&this->data_, {}); }

  inline explicit cfg_compiledCPInfoPtr(const GUID& p_guid) : cfg_var(p_guid) {}
};

class ScriptedCoverPositions {
 public:
  ScriptedCoverPositions();

  bool setScript(const char* script, pfc::string_base& errorMsg);
  const fovAspectBehaviour& getAspectBehaviour();
  const glVectord& getLookAt();
  const glVectord& getUpVector();
  bool isMirrorPlaneEnabled();
  const glVectord& getMirrorNormal();
  const glVectord& getMirrorCenter();
  const glVectord& getCameraPos();
  const int getFirstCover();
  const int getLastCover();
  double distanceToMirror(glVectord point);
  glQuad getCoverQuad(float coverId, float coverAspect);

 private:
  shared_ptr<CompiledCPInfo> cInfo;
};
