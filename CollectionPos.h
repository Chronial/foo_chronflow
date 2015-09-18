#pragma once

class DbAlbumCollection;

struct CollectionPos;

struct CollectionPos
{
public:
	CollectionPos()
		: collection(0){};

	CollectionPos(DbAlbumCollection* collection, int p)
		: collection(collection)
	{
		this->p = bound(p);
	};

	CollectionPos& operator++(void);
	CollectionPos& operator--(void);
	CollectionPos operator+(int n) const;
	CollectionPos operator-(int n) const;
	CollectionPos& operator+=(int n);
	CollectionPos& operator-=(int n);
	bool operator==(const CollectionPos& p) const;
	bool operator!=(const CollectionPos& p) const;
	int operator-(const CollectionPos& p) const; // returns the shortest distance between two CollectionPos
	size_t toIndex() const;

private:
	int p;
	DbAlbumCollection* collection;

	int bound(int n) const;
};