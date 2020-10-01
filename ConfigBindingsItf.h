#pragma once
#include "ConfigCoverConfigs.h"

namespace coverflow {

class IBinding {
 public:
  virtual ~IBinding() = default;
  virtual bool HasChanged() const = 0;
  virtual int GetActionFlag(bool changefilter = false) const = 0;
  virtual void FlowToControl(HWND wndCtrlParent = nullptr) {};
  virtual void FlowToVar(HWND wndCtrlParent = nullptr) {};
};

class IFlow {
 public:
  virtual ~IFlow() = default;
  virtual const int& GetCtrlID() = 0;
  virtual void GetFVal(pfc::string8* dstVal) {};
  virtual void GetFVal(bool* dstVal) {};
  virtual void GetFVal(int* dstVal) {};
  virtual void GetFVal(double* dstVal) {};
  virtual void GetFVal(t_size* dstVal) {};
  virtual void GetFVal(CoverConfigMap* dstVal) {};
  virtual void GetFVal(LOGFONT* dstVal) {};
};
} // namespace coverflow
