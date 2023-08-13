#pragma once
#include <memory>
#include <vector>
#include "ConfigBindingsItf.h"
#include "ConfigCoverConfigs.h" //opt

// credits to kbuffington @github
// class Binding is based on his implementation
// project name: foo_enhanced_playcount
// https://github.com/kbuffington/foo_enhanced_playcount

namespace coverflow {
class Binding {
 public:
  Binding(int& var, int actionflag, HWND hwnd, int controlId);
  Binding(bool& var, int actionflag, HWND hwnd, int controlId);
  Binding(t_size& var, int actionflag, HWND hwnd, int controlId);
  Binding(double& var, int actionflag, HWND hwnd, int controlId);
  Binding(pfc::string8& var, int actionflag, HWND hwnd, int controlId);
  Binding(LOGFONT& var, int actionflag, HWND hwnd, int controlId);
  Binding(cfg_coverConfigs& var, int actionflag, HWND hwnd, int controlId, pfc::string8 selected);

  Binding(Binding&& source);
  Binding& operator=(Binding&& source);
  ~Binding();

  bool HasChanged() const;
  int GetActionFlag(bool changefilter = false) const;
  void FlowToControl(HWND wndCtrlParent = nullptr);
  void FlowToVar(HWND wndCtrlParent = nullptr);

 private:
  std::unique_ptr<IBinding> binding_;
 public:
  IFlow* flowval_;
};

class BindingCollection {
 public:
  BindingCollection() {}
  ~BindingCollection() {}

  bool HasChanged() const;
  std::pair<int, bool> HasChangedExt() const;  //<actionflag, haschanged>
  void FlowToControl(HWND wndControlParent = nullptr);
  void FlowToVar(HWND wndControlParent = nullptr);

  template <typename... T>
  void Bind(T&&... args) {
    bindings_.emplace_back(std::forward<T>(args)...);
  }

  template <typename T>
  bool GetFVal(T&& arg, HWND hwndTab, UINT id) {
    auto&& binding =
        std::find_if(bindings_.begin(), bindings_.end(), [&](Binding& b) {
          return b.flowval_->GetCtrlID() == static_cast<int>(id);
        });
    if (binding != bindings_.end()) {
      binding->flowval_->GetFVal(arg);
      return true;
    }
    else
      return false;
  }

  void Clear() {
    bindings_.clear();
  }

 private:
  std::vector<Binding> bindings_;
};
} // namespace coverflow
