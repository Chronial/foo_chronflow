#pragma once

class RenderThread {
	Renderer renderer;
public:
	bool attachToMainWindow();
	bool initMultisampling();
	void unAttachFromMainWindow();

	bool getPositionOnPoint(int x, int y, CollectionPos& out);
	void reDraw();
	void onWindowResize(int newWidth, int newHeight);

	bool shareLists(HGLRC shareWith); // frees RC in RenderThread, shared, retakes RC in RenderThread

private:
	void startRenderThread();
	static unsigned int WINAPI runRenderThread(void* lpParameter);
	void stopRenderThread();
	void renderThreadProc();
	HANDLE renderThread;
}

class Renderer
{
	friend class SwapBufferTimer;
public:
	Renderer(AppInstance* instance);
public:
	~Renderer(void);
public:
	bool attachGlWindow();
	void initGlState();
	int getPixelFormat();
public:
	void destroyGlWindow(void); //clean up all the GL stuff
	void resizeGlScene(int width, int height);
	void resetViewport();
public:
	bool positionOnPoint(int x, int y, CollectionPos& out);
public:
	void drawFrame(bool selectionPass = false);

	bool shareLists(HGLRC shareWith);

	void glPushOrthoMatrix();
	void glPopOrthoMatrix();

public:
	bool initMultisampling();

private:
	void loadExtensions(void);
	bool isExtensionSupported(const char *name);
	bool isWglExtensionSupported(const char *name);

	void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);
private:
	static const PIXELFORMATDESCRIPTOR pixelFormatDescriptor;

	AppInstance* appInstance;
	int winWidth;
	int winHeight;
	bool showFog;

	int pixelFormat;
	bool multisampleEnabled;


	pfc::array_t<double> getMirrorClipPlane();
	void drawMirrorPass();
	void drawMirrorOverlay();
	void drawCovers(bool showTarget = false);
};



class LockedRenderingContext {
	CRITICAL_SECTION lockCS;
	HDC hDC;
	HGLRC hRC;
	int lockCount;
	DWORD lockingThread;

public:
	LockedRenderingContext()
		: hDC(0), hRC(0), lockCount(0) {
		InitializeCriticalSectionAndSpinCount(&lockCS, 0x80000400);
	}
	~LockedRenderingContext(){
		DeleteCriticalSection(&lockCS);
	}
	void setRC(HDC hDC, HGLRC hRC){
		EnterCriticalSection(&lockCS);
		this->hDC = hDC;
		this->hRC = hRC;
		LeaveCriticalSection(&lockCS);
	}

	HDC getHDC(){
		PFC_ASSERT(lockingThread == GetCurrentThreadId());
		return hDC;
	}

	HGLRC getHRC(){
		PFC_ASSERT(lockingThread == GetCurrentThreadId());
		return hRC;
	}
	void lock(){
		EnterCriticalSection(&lockCS);
		if(lockCount == 0){
			lockingThread = GetCurrentThreadId();
		}
		lockCount++;
	}
	void take(){
		EnterCriticalSection(&lockCS);
		if(lockCount == 0){
			lockingThread = GetCurrentThreadId();
			if (!wglMakeCurrent(hDC, hRC)){
				pfc::string8 temp;
				temp << format_win32_error(GetLastError());

			}
		}
		lockCount++;
	}
	void release(){
		lockCount--;
		if(lockCount == 0){
			lockingThread = 0;
			wglMakeCurrent(NULL, NULL);
		}
		LeaveCriticalSection(&lockCS);
	}
	void unlock(){
		lockCount--;
		if(lockCount == 0){
			lockingThread = 0;
		}
		LeaveCriticalSection(&lockCS);
	}
	BOOL explicitRelease(){
		PFC_ASSERT(lockCount == 0 || lockingThread != GetCurrentThreadId());
		return wglMakeCurrent(NULL, NULL);
	}
	void swapBuffers(){
		take();
		SwapBuffers(hDC);
		release();
	}
};

class ScopeRCLock {
	LockedRenderingContext* lockedRC;

public:
	ScopeRCLock(LockedRenderingContext& lockedRC)
		: lockedRC(&lockedRC){
			lockedRC.take();
	}
	~ScopeRCLock(){
		lockedRC->release();
	}
};