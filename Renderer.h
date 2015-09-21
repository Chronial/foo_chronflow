#pragma once
#include "Helpers.h"

#include "DbAlbumCollection.h"
#include "TextDisplay.h"
#include "ScriptedCoverPositions.h"

class AppInstance;
class DisplayPosition;

enum VSyncMode {
	VSYNC_SLEEP_ONLY = 1,
	VSYNC_AND_SLEEP = 2,
	VSYNC_ONLY = 3,
};

class Renderer
{
public:
	Renderer(AppInstance* instance, DisplayPosition* displayPos);
	~Renderer(void);

	bool attachGlWindow();
	void initGlState();
	int getPixelFormat();

	void destroyGlWindow(void); //clean up all the GL stuff
	void resizeGlScene(int width, int height);
	void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);

	bool offsetOnPoint(int x, int y, int& out);

	void drawFrame();

	void freeRC();
	bool takeRC();
	bool shareLists(HGLRC shareWith);
	void swapBuffers();

	void ensureVSync(bool enableVSync);

	void glPushOrthoMatrix();
	void glPopOrthoMatrix();

public:
	bool initMultisampling();

	TextDisplay textDisplay;
	FpsCounter fpsCounter;
	ScriptedCoverPositions coverPos;
	DisplayPosition* displayPos;

private:
	void loadExtensions(void);
	bool isExtensionSupported(const char *name);
	bool isWglExtensionSupported(const char *name);


	void getFrustrumSize(double &right, double &top, double &zNear, double &zFar);
	void setProjectionMatrixJittered(double xoff, double yoff);
private:
	static const PIXELFORMATDESCRIPTOR pixelFormatDescriptor;

	AppInstance* appInstance;
	int winWidth;
	int winHeight;

	int pixelFormat;
	bool multisampleEnabled;
	bool vSyncEnabled;

	HDC hDC;
	HGLRC hRC;

	void drawBg();
	void drawGui();
	void drawScene(bool selectionPass);
	void drawSceneAA();

	struct aaJitter {
		float x;
		float y;
	};
	static const aaJitter* getAAJitter (int passes);


	pfc::array_t<double> getMirrorClipPlane();
	void drawMirrorPass();
	void drawMirrorOverlay();
	void drawCovers(bool showTarget = false);
};

