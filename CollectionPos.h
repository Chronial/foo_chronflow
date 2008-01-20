#pragma once

class AlbumCollection abstract;


struct CollectionPos
{
public:
	CollectionPos(AlbumCollection* collection, int p);
public:
	CollectionPos& operator++(void);
public:
	CollectionPos& operator--(void);
public:
	CollectionPos operator+(int n);
public:
	CollectionPos operator-(int n);
public:
	CollectionPos& operator+=(int n);
public:
	CollectionPos& operator-=(int n);
public:
	bool operator==(CollectionPos p);
public:
	int operator-(CollectionPos p); // returns the shortest distance between two CollectionPos
public:
	int toIndex();
private:
	int p;
	AlbumCollection* collection;
};
