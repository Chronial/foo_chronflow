#pragma once

class DbAlbumCollection;

struct CollectionPos;

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
	{};

	CollectionPos& operator++(void);
	CollectionPos& operator--(void);
	CollectionPos operator+(int n) const;
	CollectionPos operator-(int n) const;
	CollectionPos& operator+=(int n);
	CollectionPos& operator-=(int n);
	bool operator==(const CollectionPos& p) const;
	bool operator!=(const CollectionPos& p) const;
	int operator-(const CollectionPos& p) const; // returns the shortest distance between two CollectionPos
	int toIndex();
	int toIndex() const;

private:
	int p;
	DbAlbumCollection* collection;

	friend bool operator== (T_INVALID_COL_POS, const CollectionPos&);
	friend bool operator== (const CollectionPos&, T_INVALID_COL_POS);
	friend bool operator!= (T_INVALID_COL_POS, const CollectionPos&);
	friend bool operator!= (const CollectionPos&, T_INVALID_COL_POS);
};