#pragma once
#include "TextureCache.h"
#include "DisplayPosition.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"
#include "BlockingQueue.h"
#include "Renderer.h"

class AppInstance;
class DbReloadWorker;

class RTMessage {
public:
	virtual ~RTMessage(){};
};

template <typename T>
class RTAnswerMessage : public RTMessage {
private:
	std::mutex mtx;
	std::condition_variable condition;
	bool isAnswered;
	T answer;

public:
	RTAnswerMessage() : isAnswered(false) {}

	T getAnswer(){
		std::unique_lock<std::mutex> lock(this->mtx);
		this->condition.wait(lock, [=] { return isAnswered; });
		return answer;
	}
	void setAnswer(T const& value) {
		std::unique_lock<std::mutex> lock(this->mtx);
		answer = value;
		isAnswered = true;
		this->condition.notify_one();
	}
};

class RTPaintMessage : public RTMessage {};
class RTRedrawMessage : public RTMessage {};
class RTStopThreadMessage : public RTMessage {};
class RTInitDoneMessage : public RTAnswerMessage <bool> {};
class RTTextFormatChangedMessage : public RTMessage {};
class RTDeviceModeMessage : public RTMessage {};
class RTWindowResizeMessage : public RTMessage {
public:
	RTWindowResizeMessage(int width, int height) : width(width), height(height) {};
	const int width;
	const int height;
};
class RTTargetChangedMessage : public RTMessage {};

class RTGetPosAtCoordsMessage : public RTAnswerMessage <shared_ptr<CollectionPos>> {
public:
	RTGetPosAtCoordsMessage(int x, int y) : x(x), y(y) {};
	const int x;
	const int y;
};

class RTChangeCPScriptMessage : public RTMessage {
public:
	RTChangeCPScriptMessage(const pfc::string_base &script) : script(script) {};
	const pfc::string8 script;
};

class RTCollectionReloadedMessage : public RTMessage {};


class RTWindowHideMessage : public RTMessage {};
class RTWindowShowMessage : public RTMessage {};


class RenderThread {
	DisplayPosition displayPos;
	TextureCache texCache;
	Renderer renderer;
	AppInstance* appInstance;
public:
	RenderThread(AppInstance* appInstance);
	~RenderThread();

	void send(shared_ptr<RTMessage> msg);
private:
	int timerResolution;
	bool timerInPeriod;
	int refreshRate;
	double afterLastSwap;

	void updateRefreshRate();

	static unsigned int WINAPI runRenderThread(void* lpParameter);
	void threadProc();
	void onPaint();
	bool doPaint;

	HANDLE renderThread;

	BlockingQueue<shared_ptr<RTMessage>> messageQueue;
};