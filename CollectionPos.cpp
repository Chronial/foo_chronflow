#include "stdafx.h"

#include "CollectionPos.h"
#include "DbAlbumCollection.h"

CollectionPos& CollectionPos::operator++(void)
{
	p = bound(p + 1);
	return *this;
};

CollectionPos& CollectionPos::operator--(void)
{
	PFC_ASSERT(collection);
	p = bound(p - 1);
	return *this;
};

CollectionPos CollectionPos::operator+(int n) const
{
	PFC_ASSERT(collection);
	return CollectionPos(collection, bound(p + n));
};

CollectionPos CollectionPos::operator-(int n) const
{
	PFC_ASSERT(collection);
	return CollectionPos(collection, bound(p - n));
};

CollectionPos& CollectionPos::operator+=(int n)
{
	PFC_ASSERT(collection);
	p = bound(p + n);
	return *this;
};

CollectionPos& CollectionPos::operator-=(int n){
	PFC_ASSERT(collection);
	p = bound(p - n);
	return *this;
};

bool CollectionPos::operator==(const CollectionPos& p) const{
	return this->toIndex() == p.toIndex();
};

bool CollectionPos::operator!=(const CollectionPos& p) const
{
	return this->toIndex() != p.toIndex();
};

int CollectionPos::operator-(const CollectionPos& p) const
{
	int dist = this->toIndex() - p.toIndex();
	return dist;
};

size_t CollectionPos::toIndex() const
{
	return p;
};


int CollectionPos::bound(int n) const
{
	PFC_ASSERT(collection);
	int arraySize = collection->getCount();
	return max(0, min(arraySize - 1, n));
};