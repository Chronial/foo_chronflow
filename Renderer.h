#pragma once
#include "Helpers.h"

#include "DbAlbumCollection.h"
#include "TextDisplay.h"
#include "ScriptedCoverPositions.h"

class AppInstance;
class TextureCache;
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

	void initGlState();

	void resizeGlScene(int width, int height);
	void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);

	bool offsetOnPoint(int x, int y, int& out);

	void drawFrame();

	void swapBuffers();

	void ensureVSync(bool enableVSync);

	void glPushOrthoMatrix();
	void glPopOrthoMatrix();

	TextDisplay textDisplay;
	FpsCounter fpsCounter;
	ScriptedCoverPositions coverPos;
	DisplayPosition *const displayPos;
	TextureCache* texCache;

private:
	void loadExtensions(void);
	bool isExtensionSupported(const char *name);
	bool isWglExtensionSupported(const char *name);

	void getFrustrumSize(double &right, double &top, double &zNear, double &zFar);

	AppInstance* appInstance;
	int winWidth;
	int winHeight;

	bool vSyncEnabled;

	void drawBg();
	void drawGui();
	void drawScene(bool selectionPass);

	pfc::array_t<double> getMirrorClipPlane();
	void drawMirrorPass();
	void drawMirrorOverlay();
	void drawCovers(bool showTarget = false);
};

