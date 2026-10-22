#ifndef __HASHINDEX_H__
#define __HASHINDEX_H__

/*
===============================================================================

	Fast hash table for indexes and arrays.
	Does not allocate memory until the first key/index pair is added.

===============================================================================
*/

#define DEFAULT_HASH_SIZE			1024
#define DEFAULT_HASH_GRANULARITY	1024

class hfHashIndex {
public:
	static const int NULL_INDEX = -1;
					hfHashIndex();
					hfHashIndex( const int initialHashSize, const int initialIndexSize );
					~hfHashIndex();

					// returns total size of allocated memory
	size_t			Allocated() const;
					// returns total size of allocated memory including size of hash index type
	size_t			Size() const;

	hfHashIndex &	operator=( const hfHashIndex &other );
					// add an index to the hash, assumes the index has not yet been added to the hash
	void			Add( const int key, const int index );
					// remove an index from the hash
	void			Remove( const int key, const int index );
					// get the first index from the hash, returns -1 if empty hash entry
	int				First( const int key ) const;
					// get the next index from the hash, returns -1 if at the end of the hash chain
	int				Next( const int index ) const;

	// For porting purposes...
	int				GetFirst( const int key ) const { return First( key ); }
	int				GetNext( const int index ) const { return Next( index ); }

					// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void			InsertIndex( const int key, const int index );
					// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void			RemoveIndex( const int key, const int index );
					// clear the hash
	void			Clear();
					// clear and resize
	void			Clear( const int newHashSize, const int newIndexSize );
					// free allocated memory
	void			Free();
					// get size of hash table
	int				GetHashSize() const;
					// get size of the index
	int				GetIndexSize() const;
					// set granularity
	void			SetGranularity( const int newGranularity );
					// force resizing the index, current hash table stays intact
	void			ResizeIndex( const int newIndexSize );
					// returns number in the range [0-100] representing the spread over the hash table
	int				GetSpread() const;
					// returns a key for a string
	int				GenerateKey( const char *string, bool caseSensitive = true ) const;
					// returns a key for a vector
	int				GenerateKey( const hfVec3 &v ) const;
					// returns a key for two integers
	int				GenerateKey( const int n1, const int n2 ) const;
					// returns a key for a single integer
	int				GenerateKey( const int n ) const;

private:
	int				hashSize;
	int *			hash;
	int				indexSize;
	int *			indexChain;
	int				granularity;
	int				hashMask;
	int				lookupMask;

	static int		INVALID_INDEX[1];

	void			Init( const int initialHashSize, const int initialIndexSize );
	void			Allocate( const int newHashSize, const int newIndexSize );
};

/*
================
hfHashIndex::hfHashIndex
================
*/
HF_INLINE hfHashIndex::hfHashIndex() {
	Init( DEFAULT_HASH_SIZE, DEFAULT_HASH_SIZE );
}

/*
================
hfHashIndex::hfHashIndex
================
*/
HF_INLINE hfHashIndex::hfHashIndex( const int initialHashSize, const int initialIndexSize ) {
	Init( initialHashSize, initialIndexSize );
}

/*
================
hfHashIndex::~hfHashIndex
================
*/
HF_INLINE hfHashIndex::~hfHashIndex() {
	Free();
}

/*
================
hfHashIndex::Allocated
================
*/
HF_INLINE size_t hfHashIndex::Allocated() const {
	return hashSize * sizeof( int ) + indexSize * sizeof( int );
}

/*
================
hfHashIndex::Size
================
*/
HF_INLINE size_t hfHashIndex::Size() const {
	return sizeof( *this ) + Allocated();
}

/*
================
hfHashIndex::operator=
================
*/
HF_INLINE hfHashIndex &hfHashIndex::operator=( const hfHashIndex &other ) {
	granularity = other.granularity;
	hashMask = other.hashMask;
	lookupMask = other.lookupMask;

	if ( other.lookupMask == 0 ) {
		hashSize = other.hashSize;
		indexSize = other.indexSize;
		Free();
	}
	else {
		if ( other.hashSize != hashSize || hash == INVALID_INDEX ) {
			if ( hash != INVALID_INDEX ) {
				delete[] hash;
			}
			hashSize = other.hashSize;
			hash = new (TAG_IDLIB_HASH) int[hashSize];
		}
		if ( other.indexSize != indexSize || indexChain == INVALID_INDEX ) {
			if ( indexChain != INVALID_INDEX ) {
				delete[] indexChain;
			}
			indexSize = other.indexSize;
			indexChain = new (TAG_IDLIB_HASH) int[indexSize];
		}
		memcpy( hash, other.hash, hashSize * sizeof( hash[0] ) );
		memcpy( indexChain, other.indexChain, indexSize * sizeof( indexChain[0] ) );
	}

	return *this;
}

/*
================
hfHashIndex::Add
================
*/
HF_INLINE void hfHashIndex::Add( const int key, const int index ) {
	int h;

	assert( index >= 0 );
	if ( hash == INVALID_INDEX ) {
		Allocate( hashSize, index >= indexSize ? index + 1 : indexSize );
	}
	else if ( index >= indexSize ) {
		ResizeIndex( index + 1 );
	}
	h = key & hashMask;
	indexChain[index] = hash[h];
	hash[h] = index;
}

/*
================
hfHashIndex::Remove
================
*/
HF_INLINE void hfHashIndex::Remove( const int key, const int index ) {
	int k = key & hashMask;

	if ( hash == INVALID_INDEX ) {
		return;
	}
	if ( hash[k] == index ) {
		hash[k] = indexChain[index];
	}
	else {
		for ( int i = hash[k]; i != -1; i = indexChain[i] ) {
			if ( indexChain[i] == index ) {
				indexChain[i] = indexChain[index];
				break;
			}
		}
	}
	indexChain[index] = -1;
}

/*
================
hfHashIndex::First
================
*/
HF_INLINE int hfHashIndex::First( const int key ) const {
	return hash[key & hashMask & lookupMask];
}

/*
================
hfHashIndex::Next
================
*/
HF_INLINE int hfHashIndex::Next( const int index ) const {
	assert( index >= 0 && index < indexSize );
	return indexChain[index & lookupMask];
}

/*
================
hfHashIndex::InsertIndex
================
*/
HF_INLINE void hfHashIndex::InsertIndex( const int key, const int index ) {
	int i, max;

	if ( hash != INVALID_INDEX ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( hash[i] >= index ) {
				hash[i]++;
				if ( hash[i] > max ) {
					max = hash[i];
				}
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( indexChain[i] >= index ) {
				indexChain[i]++;
				if ( indexChain[i] > max ) {
					max = indexChain[i];
				}
			}
		}
		if ( max >= indexSize ) {
			ResizeIndex( max + 1 );
		}
		for ( i = max; i > index; i-- ) {
			indexChain[i] = indexChain[i-1];
		}
		indexChain[index] = -1;
	}
	Add( key, index );
}

/*
================
hfHashIndex::RemoveIndex
================
*/
HF_INLINE void hfHashIndex::RemoveIndex( const int key, const int index ) {
	int i, max;

	Remove( key, index );
	if ( hash != INVALID_INDEX ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( hash[i] >= index ) {
				if ( hash[i] > max ) {
					max = hash[i];
				}
				hash[i]--;
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( indexChain[i] >= index ) {
				if ( indexChain[i] > max ) {
					max = indexChain[i];
				}
				indexChain[i]--;
			}
		}
		for ( i = index; i < max; i++ ) {
			indexChain[i] = indexChain[i+1];
		}
		indexChain[max] = -1;
	}
}

/*
================
hfHashIndex::Clear
================
*/
HF_INLINE void hfHashIndex::Clear() {
	// only clear the hash table because clearing the indexChain is not really needed
	if ( hash != INVALID_INDEX ) {
		memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	}
}

/*
================
hfHashIndex::Clear
================
*/
HF_INLINE void hfHashIndex::Clear( const int newHashSize, const int newIndexSize ) {
	Free();
	hashSize = newHashSize;
	indexSize = newIndexSize;
}

/*
================
hfHashIndex::GetHashSize
================
*/
HF_INLINE int hfHashIndex::GetHashSize() const {
	return hashSize;
}

/*
================
hfHashIndex::GetIndexSize
================
*/
HF_INLINE int hfHashIndex::GetIndexSize() const {
	return indexSize;
}

/*
================
hfHashIndex::SetGranularity
================
*/
HF_INLINE void hfHashIndex::SetGranularity( const int newGranularity ) {
	assert( newGranularity > 0 );
	granularity = newGranularity;
}

/*
================
hfHashIndex::GenerateKey
================
*/
HF_INLINE int hfHashIndex::GenerateKey( const char *string, bool caseSensitive ) const {
	if ( caseSensitive ) {
		return ( hfStr::Hash( string ) & hashMask );
	} else {
		return ( hfStr::IHash( string ) & hashMask );
	}
}

/*
================
hfHashIndex::GenerateKey
================
*/
HF_INLINE int hfHashIndex::GenerateKey( const hfVec3 &v ) const {
	return ( (((int) v[0]) + ((int) v[1]) + ((int) v[2])) & hashMask );
}

/*
================
hfHashIndex::GenerateKey
================
*/
HF_INLINE int hfHashIndex::GenerateKey( const int n1, const int n2 ) const {
	return ( ( n1 + n2 ) & hashMask );
}

/*
================
hfHashIndex::GenerateKey
================
*/
HF_INLINE int hfHashIndex::GenerateKey( const int n ) const {
	return n & hashMask;
}

#endif /* !__HASHINDEX_H__ */

