#pragma once
#include "DisplayPosition.h"
#include "DbAlbumCollection.h"
#include "BlockingQueue.h"
#include "CriticalSection.h"
#include "Renderer.h"

class AppInstance;

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
class RTAttachMessage : public RTAnswerMessage <bool> {};
class RTUnattachMessage : public RTAnswerMessage <bool> {};
class RTMultiSamplingMessage : public RTAnswerMessage <bool> {};
class RTTextFormatChangedMessage : public RTMessage {};
class RTDeviceModeMessage : public RTMessage {};
class RTWindowResizeMessage : public RTMessage {
	RTWindowResizeMessage(){};
public:
	RTWindowResizeMessage(int width, int height) : width(width), height(height) {};
	int width;
	int height;
};
class RTTargetChangedMessage : public RTMessage {};

class RTGetPosAtCoordsMessage : public RTAnswerMessage <shared_ptr<CollectionPos>> {
	RTGetPosAtCoordsMessage(){};
public:
	RTGetPosAtCoordsMessage(int x, int y) : x(x), y(y) {};
	int x;
	int y;
};

class RTShareListDataAnswer : public RTAnswerMessage <bool> {};
class RTShareListDataMessage : public RTAnswerMessage < shared_ptr<RTShareListDataAnswer> > {};


class RTChangeCPScriptMessage : public RTMessage {
	RTChangeCPScriptMessage(){};
public:
	RTChangeCPScriptMessage(const pfc::string_base &script) : script(script) {};
	pfc::string8 script;
};



class RenderThread {
	DisplayPosition displayPos;
	Renderer renderer;
	AppInstance* appInstance;
public:
	RenderThread(AppInstance* appInstance);
	~RenderThread();

	bool shareLists(HGLRC shareWith); // frees RC in RenderThread, shares, retakes RC in RenderThread
	int getPixelFormat(){
		// TODO: synchronize this properly
		return renderer.getPixelFormat();
	};
	void stopRenderThread();

	CollectionPos getCenteredPos();
	void hardSetCenteredPos(CollectionPos pos);

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


