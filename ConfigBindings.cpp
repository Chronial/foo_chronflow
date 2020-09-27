#include <algorithm>
#include "ConfigBindings.h"
#include "ConfigCoverConfigs.h"
//#include "EngineThread.h"
//#include "engine.h"
#include "ConfigData.h"

namespace coverflow {

//using engine::EngineThread;
//using EM = engine::Engine::Messages;

namespace {

class StringBinding : public IBinding, IFlow {
 public:
  StringBinding(pfc::string8& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  void GetFVal(pfc::string8* dstVal) final {
    const pfc::string8 res = pfc::string8(var_);
    dstVal->set_string(var_);
  }

  int GetActionFlag(bool changefilter = false) const final { return !changefilter? actionflag_ : !HasChanged()? 0 : actionflag_; }

  bool HasChanged() const final {
    pfc::string8_fast text;
    const HWND hwtest = uGetDlgItem(hwnd_, controlId_);
    uGetDlgItemText(hwnd_, controlId_, text);

    bool bres = false;
    bres = !var_.equals(text.toString());

    return bres;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;

    uSetDlgItemText(hwnd_, controlId_, var_.get_ptr());
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    pfc::string8_fast text;
    uGetDlgItemText(hwnd_, controlId_, text);
    var_ = text.get_ptr();
  }

  const int& GetCtrlID() final { return controlId_; }

 private:
  pfc::string8& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

class BoolBinding : public IBinding, IFlow {
 public:
  BoolBinding(bool& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  bool HasChanged() const final {
    bool const checked = IsDlgButtonChecked(hwnd_, controlId_) == BST_CHECKED;
    return checked != var_;
  }

  int GetActionFlag(bool changefilter = false) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    CheckDlgButton(hwnd_, controlId_, var_ ? BST_CHECKED : BST_UNCHECKED);
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    var_ = IsDlgButtonChecked(hwnd_, controlId_) == BST_CHECKED;
  }

  const int& GetCtrlID() final { return controlId_; }
  void GetFVal(bool* dstval) final { *dstval = &var_; }

 private:
  bool& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

class IntBinding : public IBinding, IFlow {
 public:
  IntBinding(int& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  bool HasChanged() const final {
    int i;
    i = uGetDlgItemInt(hwnd_, controlId_, nullptr, true);
    return (var_ != i);
  }

  int GetActionFlag(bool changefilter) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    uSetDlgItemInt(hwnd_, controlId_, var_, true);
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    var_ = uGetDlgItemInt(hwnd_, controlId_, nullptr, true);
  }

  const int& GetCtrlID() final { return controlId_; }
  void GetFVal(int* dstval) final { *dstval = var_; }

 private:
  int& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

class DoubleBinding : public IBinding, IFlow {
 public:
  DoubleBinding(double& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  const int& GetCtrlID() final { return controlId_; }

  bool HasChanged() const final {
    double i;
    pfc::string8 txtval;
    uGetDlgItemText(hwnd_, controlId_, txtval);
    i = atof(txtval.c_str());
    return (var_ != i);
  }

  int GetActionFlag(bool changefilter) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    uSetDlgItemText(hwnd_, controlId_, std::to_string(var_).c_str());
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    var_ = atof(uGetDlgItemText(hwnd_, controlId_).c_str());
  }

  void GetFVal(double* dstval) final { *dstval = var_; }

 private:
  double& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

class CoverConfigBinding : public IBinding, IFlow {
 public:
  CoverConfigBinding(cfg_coverConfigs& var, int actionflag, HWND hwnd, int controlId,
                     pfc::string8 selected)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId), selected_(selected) {
  }

  void GetFVal(CoverConfigMap* dstVal) final { dstVal->insert(var_.begin(), var_.end()); }

  bool HasChanged() const final {
    pfc::string8_fast text;

    uGetDlgItemText(hwnd_, controlId_, text);

    pfc::string txtSelected = uGetDlgItemText(hwnd_, IDC_SAVED_SELECT);

    bool bres;
    if (txtSelected.get_length()) {
      const CoverConfig& config = var_.at(txtSelected.c_str());
      std::string vartext = std::string(config.script.c_str());
      bres = uStringCompare(windows_lineendings(vartext).c_str(), text.get_ptr()) != 0;
    } else {
      //getting weird on_get_state() calls from framework
      bres = uStringCompare(txtSelected.get_ptr(), text.get_ptr()) != 0;
    }

    return bres;
  }

  int GetActionFlag(bool changefilter) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    HWND hw = uGetDlgItem(hwnd_, controlId_);

    const CoverConfig& config = var_.at(selected_.c_str());
    uSetDlgItemText(hwnd_, controlId_, windows_lineendings(config.script).c_str());
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;

    pfc::string8 txtSelected;
    uGetDlgItemText(hwnd_, IDC_SAVED_SELECT, txtSelected);

    pfc::string8_fast text;
    uGetDlgItemText(hwnd_, controlId_, text);

    const CoverConfig& config = var_.at(txtSelected.c_str());

    std::string txt = config.script;
    txt.assign(text.get_ptr());
    var_.at(txtSelected.c_str()).script = linux_lineendings(text.c_str());
    selected_ = txtSelected;
  }

  const int& GetCtrlID() { return controlId_; }

 private:
  cfg_coverConfigs& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
  pfc::string8 selected_;
};

class FontBinding : public IBinding, IFlow {
 public:
  FontBinding(LOGFONT& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  void GetFVal(LOGFONT* dstVal) final { dstVal = &var_; }

  bool HasChanged() const final {
    pfc::string8 text;

    pfc::string8 tmpstrBase64;
    uGetDlgItemText(hwnd_, controlId_, tmpstrBase64);

    LOGFONT lfctrl;
    std::string restbuf(sizeof(lfctrl), '\0');
    pfc::string8 restorebuf;
    restorebuf.set_string(restbuf.c_str(), restbuf.length());

    pfc::base64_decode(tmpstrBase64.c_str(), &lfctrl);
    int res = memcmp(&var_, &lfctrl, sizeof(lfctrl));

    return (res != 0);
  }

  int GetActionFlag(bool changefilter) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;

    std::ostringstream o_txt_stream;
    std::string buf(sizeof(var_), '\0');

    pfc::string8 pfcbuf = pfc::string8(buf.c_str());

    o_txt_stream.write((char*)&var_, sizeof(var_));
    pfc::string8 strBase64;
    pfc::base64_encode(strBase64, (void*)o_txt_stream.str().c_str(), sizeof(var_));

    uSetDlgItemText(hwnd_, controlId_, strBase64);
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;

    pfc::string8 tmpstr, tmpstrBase64;
    uGetDlgItemText(hwnd_, controlId_, tmpstrBase64);

    LOGFONT restoreFont;
    restoreFont = def_cfgTitleFont();
    std::string restbuf(sizeof(restoreFont), '\0');
    pfc::string8 restorebuf;
    restorebuf.set_string(restbuf.c_str(), restbuf.length());
    pfc::base64_decode(tmpstrBase64.c_str(), &restoreFont);

    var_ = restoreFont;
  }

  const int& GetCtrlID() final { return controlId_; }

 private:
  LOGFONT& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

class TsizeBinding : public IBinding, IFlow {
 public:
  TsizeBinding(t_size& var, int actionflag, HWND hwnd, int controlId)
    : var_(var), actionflag_(actionflag), hwnd_(hwnd), controlId_(controlId) {
  }

  void GetFVal(t_size* dstVal) final {
    const t_size res = t_size(var_);
    memcpy(dstVal, (const void*)var_, sizeof(var_));
  }

  bool HasChanged() const final {
    t_size tsval;
    if (uGetDlgItemText(hwnd_, controlId_) == "") {
      tsval = 0;
    } else
        tsval = std::stoull(uGetDlgItemText(hwnd_, controlId_).c_str(), nullptr, 0);
    return (var_ != tsval);
  }

  int GetActionFlag(bool changefilter) const final {
    return !changefilter ? actionflag_ : !HasChanged() ? 0 : actionflag_;
  }

  void FlowToControl(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;
    uSetDlgItemText(hwnd_, controlId_, std::to_string(var_).data());
  }

  void FlowToVar(HWND wndCtrlParent = nullptr) final {
    if (wndCtrlParent != nullptr && hwnd_ != wndCtrlParent)
      return;

    t_size tsval = std::stoull(uGetDlgItemText(hwnd_, controlId_).c_str(), nullptr, 0);
    var_ = tsval;
  }

  const int& GetCtrlID() final { return controlId_; }

 private:
  t_size& var_;
  int actionflag_;
  HWND hwnd_;
  int controlId_;
};

} // namespace

Binding::Binding(int& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<IntBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(IntBinding*)(binding_.get());
}

Binding::Binding(bool& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<BoolBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(BoolBinding*)(binding_.get());
}

Binding::Binding(pfc::string8& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<StringBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(StringBinding*)(binding_.get());
}

Binding::Binding(double& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<DoubleBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(DoubleBinding*)(binding_.get());
}

Binding::Binding(t_size& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<TsizeBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(TsizeBinding*)(binding_.get());
}

Binding::Binding(LOGFONT& var, int actionflag, HWND hwnd, int controlId)
  : binding_(std::make_unique<FontBinding>(var, actionflag, hwnd, controlId)) {
  flowval_ = (IFlow*)(FontBinding*)(binding_.get());
}

Binding::Binding(cfg_coverConfigs& var, int actionflag, HWND hwnd, int controlId, pfc::string8 selected)
  : binding_(std::make_unique<CoverConfigBinding>(var, actionflag, hwnd, controlId, selected)) {
  flowval_ = (IFlow*)(CoverConfigBinding*)(binding_.get());
}

Binding::Binding(Binding&&) = default;
Binding& Binding::operator=(Binding&&) = default;
Binding::~Binding() = default;

bool Binding::HasChanged() const { return binding_->HasChanged(); }
int Binding::GetActionFlag(bool changefilter) const { return binding_->GetActionFlag(changefilter); }

void Binding::FlowToControl(HWND wndCtrlParent) {
  binding_->FlowToControl(wndCtrlParent);
}

void Binding::FlowToVar(HWND wndCtrlParent) { binding_->FlowToVar(wndCtrlParent); }

bool BindingCollection::HasChanged() const {
  return std::any_of(std::begin(bindings_), std::end(bindings_),
              [](auto const& binding) { return binding.HasChanged(); });
}

//get changed status and actions to trigger
std::pair<int, bool> BindingCollection::HasChangedExt() const {

    bool bflag0 = std::any_of(std::begin(bindings_), std::end(bindings_), [](auto const& binding) {
          return binding.GetActionFlag(true) == (1 << 0);
        });
    bool bflag1 = std::any_of(std::begin(bindings_), std::end(bindings_), [](auto const& binding) {
          return binding.GetActionFlag(true) == (1 << 1);
        });

    int actionflag = bflag0 ? (1 << 0) : 0;
    actionflag = actionflag | (bflag1? 1 << 1 : actionflag);

    //return checked flags change status and general change status
    std::pair<int, bool> pres = { actionflag, HasChanged() };

    return pres;
}

void BindingCollection::FlowToControl(HWND wndCtrlParent) {
  for (auto&& binding : bindings_) {
    binding.FlowToControl(wndCtrlParent);
  }
}

void BindingCollection::FlowToVar(HWND wndCtrlParent) {

  for (auto&& binding : bindings_) {

    binding.FlowToVar(wndCtrlParent);

    //debugging example for controls
    //int debug_ctrl = (binding.flowval_)->GetCtrlID();
    //if (debug_ctrl == IDC_DISPLAY_CONFIG) {
    //  auto debugflow = binding.flowval_;
    //  CoverConfigMap scriptMap;
    //  bool bcheck = debugflow->GetFVal(&scriptMap);
    //}
  }
}
} // namespace coverflow
