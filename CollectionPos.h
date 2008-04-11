#pragma once

typedef CollectionPos* T_INVALID_COL_POS;
#define INVALID_COL_POS ((T_INVALID_COL_POS)666)

struct CollectionPos
{
public:
	CollectionPos()
		: collection(0){};

	CollectionPos(T_INVALID_COL_POS i)
		: collection(0){};

	CollectionPos(DbAlbumCollection* collection, int p)
		: collection(collection), p(p)
	{
		/*int colSize = collection->getCount();
		this->p = p % colSize;
		if (this->p < 0)
		this->p += colSize;*/
	};

	CollectionPos& operator++(void)
	{
		//p = (p+1) % collection->getCount();
		PFC_ASSERT(collection);
		++p;
		return *this;
	};

	CollectionPos& operator--(void)
	{
		PFC_ASSERT(collection);
		--p;
		/*if (p < 0)
		p += collection->getCount();*/
		return *this;
	};

	CollectionPos operator+(int n) const
	{
		PFC_ASSERT(collection);
		return CollectionPos(collection, p + n);
	};

	CollectionPos operator-(int n) const
	{
		PFC_ASSERT(collection);
		return CollectionPos(collection, p - n);
	};

	CollectionPos& operator+=(int n)
	{
		PFC_ASSERT(collection);
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
		PFC_ASSERT(collection);
		/*if (n > p){ // substraction, so that p would be < 0
		int cCount = collection->getCount();
		p = cCount + ((p-n) % cCount);
		} else {
		p = (p-n) % collection->getCount();
		}*/
		p -= n;
		return *this;
	};

	bool operator==(const CollectionPos& p) const
	{
		PFC_ASSERT(collection);
		return this->toIndex() == p.toIndex();
	};

	bool operator!=(const CollectionPos& p) const
	{
		PFC_ASSERT(collection);
		return this->toIndex() != p.toIndex();
	};

	int operator-(const CollectionPos& p) const // returns the shortest distance between two CollectionPos
	{
		PFC_ASSERT(collection);
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
		PFC_ASSERT(collection);
		int arraySize = collection->getCount();
		if (arraySize < 2)
			return 0;
		p %= arraySize;
		if (p < 0)
			p += arraySize;
		return p;
	};

	int toIndex() const
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


	/*const CollectionPos& operator=(const CollectionPos& p_source)
	{
		p = p_source.p;
		collection = p_source.collection;
		return *this;
	}*/
private:
	int p;
	DbAlbumCollection* collection;

	friend bool operator== (T_INVALID_COL_POS, const CollectionPos&);
	friend bool operator== (const CollectionPos&, T_INVALID_COL_POS);
	friend bool operator!= (T_INVALID_COL_POS, const CollectionPos&);
	friend bool operator!= (const CollectionPos&, T_INVALID_COL_POS);
};

inline bool operator== (T_INVALID_COL_POS a, const CollectionPos& b) { PFC_ASSERT(a==INVALID_COL_POS); return 0 == b.collection; };
inline bool operator== (const CollectionPos& b, T_INVALID_COL_POS a) { PFC_ASSERT(a==INVALID_COL_POS); return 0 == b.collection; };

inline bool operator!= (T_INVALID_COL_POS a, const CollectionPos& b) { PFC_ASSERT(a==INVALID_COL_POS); return 0 != b.collection; };
inline bool operator!= (const CollectionPos& b, T_INVALID_COL_POS a) { PFC_ASSERT(a==INVALID_COL_POS); return 0 != b.collection; };