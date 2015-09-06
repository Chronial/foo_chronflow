#pragma once

// IDropSource implementation for drag&drop.
class TrackDropSource : public IDropSource {
private:
	pfc::refcounter m_refcount;
	HWND m_hWnd;

	TrackDropSource(HWND hWnd);

public:
	static pfc::com_ptr_t<IDropSource> g_create(HWND hWnd);

	/////////////////////////////////////////////////////////
	// IUnknown methods

	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	/////////////////////////////////////////////////////////
	// IDropSource methods

	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
};
