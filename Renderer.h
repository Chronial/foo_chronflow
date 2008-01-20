#pragma once

class Renderer
{
public:
	Renderer(DisplayPosition* disp);
public:
	~Renderer(void);
public:
	bool setupGlWindow(HWND hWnd); //complete initialization of all GL stuff
public:
	void destroyGlWindow(void); //clean up all the GL stuff
	void resizeGlScene(int width, int height);
public:
	bool positionOnPoint(int x, int y, CollectionPos* out);
public:
	void drawFrame(void);
	void drawEmptyFrame();
	void swapBuffers();
private:
	void loadExtensions(void);
	bool isExtensionSupported(const char *name);

	void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);
private:
	DisplayPosition* displayPosition;
private:
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	int winWidth;
	int winHeight;

	void drawFloor();
	void drawCovers(bool showTarget = false);
};
