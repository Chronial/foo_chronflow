#pragma once

class Renderer
{
public:
	Renderer(AppInstance* instance);
public:
	~Renderer(void);
public:
	bool setupGlWindow(); //complete initialization of all GL stuff
	PIXELFORMATDESCRIPTOR getPixelFormat();
public:
	void destroyGlWindow(void); //clean up all the GL stuff
	void resizeGlScene(int width, int height);
	void resetViewport();
public:
	bool positionOnPoint(int x, int y, CollectionPos& out);
public:
	void drawFrame(void);
	void drawEmptyFrame();
	void swapBuffers();

	bool setRenderingContext();
	bool releaseRenderingContext();

	bool shareLists(HGLRC shareWith);

	void glPushOrthoMatrix();
	void glPopOrthoMatrix();
private:
	void loadExtensions(void);
	bool isExtensionSupported(const char *name);

	void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);
private:
	AppInstance* appInstance;
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	int winWidth;
	int winHeight;
	bool showFog;

	pfc::array_staticsize_t<double> getMirrorClipPlane();
	void drawMirrorPass();
	void drawMirrorOverlay();
	void drawCovers(bool showTarget = false);
};
