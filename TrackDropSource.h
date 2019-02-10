#pragma once

// IDropSource implementation for drag&drop.
class TrackDropSource final : public IDropSource {
 private:
  pfc::refcounter m_refcount;
  HWND m_hWnd;

  explicit TrackDropSource(HWND hWnd);

 public:
  static pfc::com_ptr_t<IDropSource> g_create(HWND hWnd);

  /////////////////////////////////////////////////////////
  // IUnknown methods

  STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  /////////////////////////////////////////////////////////
  // IDropSource methods

  STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState) final;
  STDMETHOD(GiveFeedback)(DWORD dwEffect) final;
};
