#pragma once

class DisplayPosition
{
	AppInstance* appInstance;
public:
	DisplayPosition(AppInstance* instance, CollectionPos startingPos);
public:
	~DisplayPosition(void);
public:
	void setTarget(CollectionPos pos);
public:
	void update(void);
	float moveDist2targetDist(float moveDist);
private:
	float targetDist2moveDist(float targetDist);

public:
	bool isMoving(void);
public:
	CollectionPos getTarget(void) const;
public:
	CollectionPos getCenteredPos(void) const; //Position centered or left of center (eg. display between 2. and 3. cover -> CenteredPos = 2. cover
public:
	float getCenteredOffset(void) const; // return in range [0;1)  (eg. display on pos 2.59 -- returns 0.59)

public:
	void hardSetTarget(CollectionPos pos);
	void hardSetCenteredPos(CollectionPos pos);


	CriticalSection accessCS;


private:
	CollectionPos targetPos;
	CollectionPos centeredPos;
	volatile float centeredOffset;
	float lastSpeed;

private:
	double lastMovement;
};
