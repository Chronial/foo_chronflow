#include "TrackDropSource.h"

TrackDropSource::TrackDropSource(HWND p_hWnd) : m_refcount(1), m_hWnd(p_hWnd) {}

pfc::com_ptr_t<IDropSource> TrackDropSource::g_create(HWND hWnd) {
  pfc::com_ptr_t<IDropSource> temp;
  temp.attach(new TrackDropSource(hWnd));
  return temp;
}

/////////////////////////////////////////////////////////
// IUnknown methods

// We support the IUnknown and IDropSource interfaces.
STDMETHODIMP TrackDropSource::QueryInterface(REFIID iid, void** ppvObject) {
  if (ppvObject == nullptr) {
    return E_INVALIDARG;
  } else if (iid == IID_IUnknown) {
    AddRef();
    *ppvObject = (IUnknown*)this;
    return S_OK;
  } else if (iid == IID_IDropSource) {
    AddRef();
    *ppvObject = (IDropSource*)this;
    return S_OK;
  } else
    return E_NOINTERFACE;
}

// Increase reference count.
STDMETHODIMP_(ULONG) TrackDropSource::AddRef() {
  return ++m_refcount;
}

// Decrease reference count.
// Delete object, if reference count reaches zero.
STDMETHODIMP_(ULONG) TrackDropSource::Release() {
  LONG rv = --m_refcount;
  if (rv == 0)
    delete this;
  return rv;
}

// IDropSource methods

// Determine whether the drag operation should be continued.
// Return S_OK to continue, DRAGDROP_S_CANCEL to cancel the operation,
// or DRAGDROP_S_DROP to perform a drop.
STDMETHODIMP TrackDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {
  // Cancel if escape was pressed.
  if (fEscapePressed) {
    return DRAGDROP_S_CANCEL;

    // Cancel if right mouse button was pressed.
  } else if (grfKeyState & MK_RBUTTON) {
    return DRAGDROP_S_CANCEL;

    // Drop or cancel if left mouse button was released.
  } else if (!(grfKeyState & MK_LBUTTON)) {
    DWORD pts = GetMessagePos();
    POINT pt = {static_cast<short> LOWORD(pts), static_cast<short> HIWORD(pts)};
    HWND pwnd = WindowFromPoint(pt);
    // If the mouse button was released over our window, cancel the operation.
    if (pwnd == m_hWnd || GetParent(pwnd) == m_hWnd) {
      return DRAGDROP_S_CANCEL;
      // Perform a drop otherwise.
    } else {
      return DRAGDROP_S_DROP;
    }
  }

  // Continue otherwise.
  else
    return S_OK;
}

// Provide visual feedback (through mouse pointer) about the state of the
// drag operation.
STDMETHODIMP TrackDropSource::GiveFeedback(DWORD /*dwEffect*/) {
  return DRAGDROP_S_USEDEFAULTCURSORS;
}
