#include "stdafx.h"

#include "CollectionPos.h"
#include "DbAlbumCollection.h"

CollectionPos& CollectionPos::operator++(void)
{
	//p = (p+1) % collection->getCount();
	PFC_ASSERT(collection);
	++p;
	return *this;
};

CollectionPos& CollectionPos::operator--(void)
{
	PFC_ASSERT(collection);
	--p;
	/*if (p < 0)
	p += collection->getCount();*/
	return *this;
};

CollectionPos CollectionPos::operator+(int n) const
{
	PFC_ASSERT(collection);
	return CollectionPos(collection, p + n);
};

CollectionPos CollectionPos::operator-(int n) const
{
	PFC_ASSERT(collection);
	return CollectionPos(collection, p - n);
};

CollectionPos& CollectionPos::operator+=(int n)
{
	PFC_ASSERT(collection);
	p += n;
	return *this;
};

CollectionPos& CollectionPos::operator-=(int n){
	PFC_ASSERT(collection);
	p -= n;
	return *this;
};

bool CollectionPos::operator==(const CollectionPos& p) const{
	PFC_ASSERT(collection);
	return this->toIndex() == p.toIndex();
};

bool CollectionPos::operator!=(const CollectionPos& p) const
{
	PFC_ASSERT(collection);
	return this->toIndex() != p.toIndex();
};

int CollectionPos::operator-(const CollectionPos& p) const // returns the shortest distance between two CollectionPos
{
	PFC_ASSERT(collection);
	int dist = this->toIndex() - p.toIndex();
	int count = collection->getCount();
	if (abs(dist) > (count / 2)){
		if (dist < 0)
			dist += count;
		else
			dist -= count;
	}
	return dist;
};

size_t CollectionPos::toIndex()
{
	PFC_ASSERT(collection);
	int arraySize = collection->getCount();
	if (arraySize < 2)
		return 0;
	p %= arraySize;
	if (p < 0)
		p += arraySize;
	return p;
};

size_t CollectionPos::toIndex() const
{
	PFC_ASSERT(collection);
	int arraySize = collection->getCount();
	if (arraySize < 2)
		return 0;
	int r = p % arraySize;
	if (r < 0)
		r += arraySize;
	return r;
};

inline bool operator== (T_INVALID_COL_POS a, const CollectionPos& b) {
	PFC_ASSERT(a == INVALID_COL_POS);
	return 0 == b.collection;
};

inline bool operator== (const CollectionPos& b, T_INVALID_COL_POS a) {
	PFC_ASSERT(a == INVALID_COL_POS);
	return 0 == b.collection;
};

inline bool operator!= (T_INVALID_COL_POS a, const CollectionPos& b) {
	PFC_ASSERT(a == INVALID_COL_POS);
	return 0 != b.collection;
};

inline bool operator!= (const CollectionPos& b, T_INVALID_COL_POS a) {
	PFC_ASSERT(a == INVALID_COL_POS);
	return 0 != b.collection;
};