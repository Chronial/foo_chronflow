#pragma once

class DisplayPosition
{
public:
	DisplayPosition(AlbumCollection* collection, CollectionPos startingPos, HWND redrawTarget);
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
	CollectionPos getTarget(void);
public:
	CollectionPos getCenteredPos(void); //Position centered or left of center (eg. display between 2. and 3. cover -> CenteredPos = 2. cover
public:
	float getCenteredOffset(void); // return in range [0;1)  (eg. display on pos 2.59 -- returns 0.59)

public:
	void hardSetTarget(CollectionPos pos);
	void hardSetCenteredPos(CollectionPos pos);

private:
	AlbumCollection* collection;
	CollectionPos targetPos;
	CollectionPos centeredPos;
	float centeredOffset;
	HWND redrawTarget;
	float lastSpeed;
private:
	double lastMovement;
};
