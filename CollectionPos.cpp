#include "chronflow.h"

CollectionPos::CollectionPos(AlbumCollection* collection, int p)
{
	this->p = p;
	this->collection = collection;
}

CollectionPos& CollectionPos::operator++(void)
{
	++p;
	return *this;
}

CollectionPos& CollectionPos::operator--(void)
{
	--p;
	return *this;
}

CollectionPos CollectionPos::operator+(int n)
{
	return CollectionPos(collection, p + n);
}

CollectionPos CollectionPos::operator-(int n)
{
	return CollectionPos(collection, p - n);
}

CollectionPos& CollectionPos::operator+=(int n)
{
	p += n;
	return *this;
}

CollectionPos& CollectionPos::operator-=(int n)
{
	p -= n;
	return *this;
}

bool CollectionPos::operator==(CollectionPos p)
{
	return ((this->toIndex() == p.toIndex()));
}

int CollectionPos::operator-(CollectionPos p)
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
}

int CollectionPos::toIndex(){
	int arraySize = collection->getCount();
	if (arraySize < 2)
		return 0;
	int pos = p % (arraySize);
	if (pos < 0)
		pos += arraySize;
	return pos;
}
