#pragma once

class MouseFlicker
{
public:
	MouseFlicker(AppInstance* instance);
public:
	~MouseFlicker(void);
public:
	void mouseDown(HWND hWnd, int x, int y);
public:
	void mouseUp(int x, int y);
public:
	void mouseMove(int x, int y);
public:
	void lostCapture(int x, int y);
private:
	double getPosDistance(int dX, int dY); // parameters in Window-coordinates

	double win2pos(int x); // translates window pixel-difference to colPos difference

private:
	AppInstance* appInstance;
	bool isMouseDown;
	int lastX;
	double lastTime;
	int dX;
	double dTime;
};
