#pragma once

#include "AlbumCollection.h"
#include "DbAlbumCollection.h"


struct CollectionPos
{
public:
	CollectionPos(DbAlbumCollection* collection, int p)
	{
		this->collection = collection;
		this->p = p;
		/*int colSize = collection->getCount();
		this->p = p % colSize;
		if (this->p < 0)
		this->p += colSize;*/
	};

	CollectionPos& operator++(void)
	{
		//p = (p+1) % collection->getCount();
		++p;
		return *this;
	};

	CollectionPos& operator--(void)
	{
		--p;
		/*if (p < 0)
		p += collection->getCount();*/
		return *this;
	};

	CollectionPos operator+(int n)
	{
		return CollectionPos(collection, p + n);
	};

	CollectionPos operator-(int n)
	{
		return CollectionPos(collection, p - n);
	};

	CollectionPos& operator+=(int n)
	{
		/*if (-n > p){ // substraction, so that p would be < 0
		int cCount = collection->getCount();
		p = cCount + ((p+n) % cCount);
		} else {
		p = (p+n) % collection->getCount();
		}*/
		p += n;
		return *this;
	};

	CollectionPos& operator-=(int n)
	{
		/*if (n > p){ // substraction, so that p would be < 0
		int cCount = collection->getCount();
		p = cCount + ((p-n) % cCount);
		} else {
		p = (p-n) % collection->getCount();
		}*/
		p -= n;
		return *this;
	};

	bool operator==(CollectionPos p)
	{
		return ((this->toIndex() == p.toIndex()));
	};

	int operator-(CollectionPos p) // returns the shortest distance between two CollectionPos
	{
		int dist = this->toIndex() - p.toIndex();
		int count = collection->getCount();
		if (abs(dist) > (count/2)){
			if (dist < 0)
				dist += count;
			else
				dist -= count;
		}
		return dist;
	};

	int toIndex()
	{
		int arraySize = collection->getCount();
		if (arraySize < 2)
			return 0;
		p %= arraySize;
		if (p < 0)
			p += arraySize;
		return p;
	};
private:
	int p;
	DbAlbumCollection* collection;
};