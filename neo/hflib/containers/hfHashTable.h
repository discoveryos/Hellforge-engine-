#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
================================================================================================
hfHashNodeT is a generic node for a HashTable. It is specialized by the 
StringHashNode and CStringHashNode template classes.
================================================================================================
*/
template< typename _key_, class _value_ > 
class hfHashNodeT {
public:
	hfHashNodeT() 
		:	next( NULL ) {
	}

	hfHashNodeT( const _key_ & key, const _value_ & value, hfHashNodeT * next )
		:	key( key ),
		value( value ),
		next( next ) {
	}

	static int	GetHash( const _key_ & key, const int tableMask ) {
		return key & tableMask;
	}

	static int	Compare( const _key_ & key1, const _key_ & key2 ) {
		if ( key1 < key2 ) {
			return -1;
		} else if ( key1 > key2 ) {
			return 1;
		}
		return 0;
	}

public:
	_key_							key;
	_value_							value;
	hfHashNodeT< _key_, _value_ > *	next;
};

/*
================================================
hfHashNodeT is a HashNode that provides for partial 
specialization for the HashTable, allowing the String class's Cmp function to be used 
for inserting values in sorted order.
================================================
*/
template< class _value_ >
class hfHashNodeT< hfStr, _value_ > {
public:
	hfHashNodeT( const hfStr & key, const _value_ & value, hfHashNodeT * next )
		:	key( key ),
		value( value ),
		next( next ) {
	}

	static int	GetHash( const hfStr & key, const int tableMask ) {
		return ( hfStr::Hash( key ) & tableMask );
	}

	static int	Compare( const hfStr & key1, const hfStr & key2 ) {
		return hfStr::Icmp( key1, key2 );
	}

public:
	hfStr							key;
	_value_							value;
	hfHashNodeT< hfStr, _value_ > *	next;
};

/*
================================================
hfHashNodeT is a HashNode that provides for a partial specialization 
for the HashTable, allowing the String class's Cmp function to 
be used for inserting values in sorted order. It also ensures that a copy of the the key is 
stored in a String (to more closely model the original implementation of the HashTable).
================================================
*/
template< class _value_ >
class hfHashNodeT< const char*, _value_ > {
public:
	hfHashNodeT( const char* const & key, const _value_ & value, hfHashNodeT * next )
		:	key( key ),
		value( value ),
		next( next ) {
	} 

	static int	GetHash( const char* const & key, const int tableMask ) {
		return ( hfStr::Hash( key ) & tableMask );
	}

	static int	Compare( const char* const & key1, const char* const & key2 ) {
		return hfStr::Icmp( key1, key2 );
	}

public:
	hfStr									key;	// char * keys must still get stored in an hfStr
	_value_									value;
	hfHashNodeT< const char *, _value_ > *	next;
};

/*
================================================
hfHashTableT is a general implementation of a hash table data type. It is
slower than the HashIndex, but it can also be used for LinkedLists and other data structures,
rather than just indexes and arrays. 

It uses an arbitrary key type. For String keys, use the StringHashTable template 
specialization.
================================================
*/
template< typename _key_, class _value_ >
class hfHashTableT {
public:
	hfHashTableT( const int tableSize = 256 );
	hfHashTableT( const hfHashTableT & other );
	~hfHashTableT();

	size_t			Allocated() const;
	size_t			Size() const;

	_value_ &		Set( const _key_ & key, const _value_ & value );

	bool			Get( const _key_ & key, _value_ ** value = NULL );
	bool			Get( const _key_ & key, const _value_ ** value = NULL ) const;

	bool			Remove( const _key_ & key );

	void			Clear();
	void			DeleteContents();

	int				Num() const;
	_value_ *		GetIndex( const int index ) const;
	bool			GetIndexKey( const int index, _key_ & key ) const;

	int				GetSpread() const;

	hfHashTableT &	operator=( const hfHashTableT & other );

protected:
	void			Copy( const hfHashTableT & other );

private:
	typedef hfHashNodeT< _key_, _value_ > hashnode_t;

	hashnode_t	**	heads;

	int				tableSize;
	int				numEntries;
	int				tableSizeMask;
};

/*
========================
hfHashTableT<_key_,_value_>::hfHashTableT
========================
*/
template< typename _key_, class _value_ >
HF_INLINE hfHashTableT<_key_,_value_>::hfHashTableT( const int tableSize ) {
	assert( hfMath::IsPowerOfTwo( tableSize ) );

	this->tableSize = tableSize;

	heads = new (TAG_IDLIB_HASH) hashnode_t *[ tableSize ];
	memset( heads, 0, sizeof( hashnode_t * ) * tableSize );

	numEntries = 0;
	tableSizeMask = tableSize - 1;
}

/*
========================
hfHashTableT<_key_,_value_>::hfHashTableT
========================
*/
template< typename _key_, class _value_ >
HF_INLINE hfHashTableT<_key_,_value_>::hfHashTableT( const hfHashTableT & other ) {
	Copy( other );
}

/*
========================
hfHashTableT<_key_,_value_>::~hfHashTableT
========================
*/
template< typename _key_, class _value_ >
HF_INLINE hfHashTableT<_key_,_value_>::~hfHashTableT() {
	Clear();
	delete [] heads;
	heads = NULL;
	tableSize = 0;
	tableSizeMask = 0;
	numEntries = 0;
}

/*
========================
hfHashTableT<_key_,_value_>::Allocated
========================
*/
template< typename _key_, class _value_ >
HF_INLINE size_t hfHashTableT<_key_,_value_>::Allocated() const {
	return sizeof( heads ) * tableSize + sizeof( hashnode_t* ) * numEntries;
}

/*
========================
hfHashTableT<_key_,_value_>::Size
========================
*/
template< typename _key_, class _value_ >
HF_INLINE size_t hfHashTableT<_key_,_value_>::Size() const {
	return sizeof( hfHashTableT ) + sizeof( heads )* tableSize + sizeof( hashnode_t* ) * numEntries;
}

/*
========================
hfHashTableT<_key_,_value_>::Set
========================
*/
template< typename _key_, class _value_ >
HF_INLINE _value_ & hfHashTableT<_key_,_value_>::Set( const _key_ & key, const _value_ & value ) {
	// insert sorted
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t ** nextPtr = &(heads[ hash ] );
	hashnode_t * node = * nextPtr;
	for ( ; 
		node != NULL;
		nextPtr = &(node->next), node = *nextPtr ) {
			int s = node->Compare( node->key, key );
			if ( s == 0 ) {
				// return existing hashed item
				node->value = value;
				return node->value;
			}
			if ( s > 0 ) {
				break;
			}
	}

	numEntries++;

	*nextPtr = new (TAG_IDLIB_HASH) hashnode_t( key, value, heads[ hash ] );
	(*nextPtr)->next = node;
	return (*nextPtr)->value;
}

/*
========================
hfHashTableT<_key_,_value_>::Get
========================
*/
template< typename _key_, class _value_ >
HF_INLINE bool hfHashTableT<_key_,_value_>::Get( const _key_ & key, _value_ ** value ) {
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t * node = heads[ hash ];
	for ( ; node != NULL; node = node->next ) {
		int s = node->Compare( node->key, key );
		if ( s == 0 ) {
			if ( value ) {
				*value = &node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}
	if ( value ) {
		*value = NULL;
	}
	return false;
}

/*
========================
hfHashTableT<_key_,_value_>::Get
========================
*/
template< typename _key_, class _value_ >
HF_INLINE bool hfHashTableT<_key_,_value_>::Get( const _key_ & key, const _value_ ** value ) const {
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t * node = heads[ hash ];
	for ( ; node != NULL; node = node->next ) {
		int s = node->Compare( node->key, key );
		if ( s == 0 ) {
			if ( value ) {
				*value = &node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}
	if ( value ) {
		*value = NULL;
	}
	return false;
}

/*
========================
hfHashTableT<_key_,_value_>::GetIndex
========================
*/
template< typename _key_, class _value_ >
HF_INLINE _value_ * hfHashTableT<_key_,_value_>::GetIndex( const int index ) const {
	if ( index < 0 || index > numEntries ) {
		assert( 0 );
		return NULL;
	}

	int count = 0;
	for ( int i = 0; i < tableSize; i++ ) {
		for ( hashnode_t * node = heads[ i ]; node != NULL; node = node->next ) {
			if ( count == index ) {
				return &node->value;
			}
			count++;
		}
	}
	return NULL;
}

/*
========================
hfHashTableT<_key_,_value_>::GetIndexKey
========================
*/
template< typename _key_, class _value_ >
HF_INLINE bool hfHashTableT<_key_,_value_>::GetIndexKey( const int index, _key_ & key ) const {
	if ( index < 0 || index > numEntries ) {
		assert( 0 );
		return false;
	}

	int count = 0;
	for ( int i = 0; i < tableSize; i++ ) {
		for ( hashnode_t * node = heads[ i ]; node != NULL; node = node->next ) {
			if ( count == index ) {
				key = node->key;
				return true;
			}
			count++;
		}
	}
	return false;
}

/*
========================
hfHashTableT<_key_,_value_>::Remove
========================
*/
template< typename _key_, class _value_ >
HF_INLINE bool hfHashTableT<_key_,_value_>::Remove( const _key_ & key ) {
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t ** head = &heads[ hash ];
	if ( *head ) {
		hashnode_t * prev = NULL;
		hashnode_t * node = *head;
		for ( ; node != NULL; prev = node, node = node->next ) {
			if ( node->key == key ) {
				if ( prev ) {
					prev->next = node->next;
				} else {
					*head = node->next;
				}

				delete node;
				numEntries--;
				return true;
			}
		}
	}
	return false;
}

/*
========================
hfHashTableT<_key_,_value_>::Clear
========================
*/
template< typename _key_, class _value_ >
HF_INLINE void hfHashTableT<_key_,_value_>::Clear() {
	for ( int i = 0; i < tableSize; i++ ) {
		hashnode_t * next = heads[ i ];
		while ( next != NULL ) {
			hashnode_t * node = next;
			next = next->next;
			delete node;
		}
		heads[ i ] = NULL;
	}
	numEntries = 0;
}

/*
========================
hfHashTableT<_key_,_value_>::DeleteContents
========================
*/
template< typename _key_, class _value_ >
HF_INLINE void hfHashTableT<_key_,_value_>::DeleteContents() {
	for ( int i = 0; i < tableSize; i++ ) {
		hashnode_t * next = heads[ i ];
		while ( next != NULL ) {
			hashnode_t * node = next;
			next = next->next;
			delete node->value;
			delete node;
		}
		heads[ i ] = NULL;
	}
	numEntries = 0;
}

/*
========================
hfHashTableT<_key_,_value_>::Num
========================
*/
template< typename _key_, class _value_ >
HF_INLINE int hfHashTableT<_key_,_value_>::Num() const {
	return numEntries;
}

/*
========================
hfHashTableT<_key_,_value_>::GetSpread
========================
*/
template< typename _key_, class _value_ >
HF_INLINE int hfHashTableT<_key_,_value_>::GetSpread() const {
	if ( !numEntries ) {
		return 100;
	}

	int average = numEntries / tableSize;
	int error = 0;
	for ( int i = 0; i < tableSize; i++ ) {
		int numItems = 0;
		for ( hashnode_t * node = heads[ i ]; node != NULL; node = node->next ) {
			numItems++;
		}
		int e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - ( error * 100 / numEntries );
}

/*
========================
hfHashTableT<_key_,_value_>::operator=
========================
*/
template< typename _key_, class _value_ >
HF_INLINE hfHashTableT< _key_, _value_ > & hfHashTableT<_key_,_value_>::operator=( const hfHashTableT & other ) {
	Copy( other );
	return *this;
}

/*
========================
hfHashTableT<_key_,_value_>::Copy
========================
*/
template< typename _key_, class _value_ >
HF_INLINE void hfHashTableT<_key_,_value_>::Copy( const hfHashTableT & other ) {
	if ( &other == this ) {
		return;
	}
	assert( other.tableSize > 0 );

	tableSize		= other.tableSize;
	heads			= new (TAG_IDLIB_HASH) hashnode_t *[ tableSize ];
	numEntries		= other.numEntries;
	tableSizeMask	= other.tableSizeMask;

	for ( int i = 0; i < tableSize; i++ ) {
		if ( !other.heads[ i ] ) {
			heads[ i ] = NULL;
			continue;
		}
		hashnode_t ** prev = & heads[ i ];
		for ( hashnode_t * node = other.heads[ i ]; node != NULL; node = node->next ) {
			*prev = new (TAG_IDLIB_HASH) hashnode_t( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
}

/*
===============================================================================

	General hash table. Slower than hfHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class hfHashTable {
public:
					hfHashTable( int newtablesize = 256 );
					hfHashTable( const hfHashTable<Type> &map );
					~hfHashTable();

					// returns total size of allocated memory
	size_t			Allocated() const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size() const;

	void			Set( const char *key, Type &value );
	bool			Get( const char *key, Type **value = NULL ) const;
	bool			Remove( const char *key );

	void			Clear();
	void			DeleteContents();

					// the entire contents can be itterated over, but note that the
					// exact index for a given element may change when new elements are added
	int				Num() const;
	Type *			GetIndex( int index ) const;

	int				GetSpread() const;

private:
	struct hashnode_s {
		hfStr		key;
		Type		value;
		hashnode_s *next;

		hashnode_s( const hfStr &k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
		hashnode_s( const char *k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
	};

	hashnode_s **	heads;

	int				tablesize;
	int				numentries;
	int				tablesizemask;

	int				GetHash( const char *key ) const;
};

/*
================
hfHashTable<Type>::hfHashTable
================
*/
template< class Type >
HF_INLINE hfHashTable<Type>::hfHashTable( int newtablesize ) {

	assert( hfMath::IsPowerOfTwo( newtablesize ) );

	tablesize = newtablesize;
	assert( tablesize > 0 );

	heads = new (TAG_IDLIB_HASH) hashnode_s *[ tablesize ];
	memset( heads, 0, sizeof( *heads ) * tablesize );

	numentries		= 0;

	tablesizemask = tablesize - 1;
}

/*
================
hfHashTable<Type>::hfHashTable
================
*/
template< class Type >
HF_INLINE hfHashTable<Type>::hfHashTable( const hfHashTable<Type> &map ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	**prev;

	assert( map.tablesize > 0 );

	tablesize		= map.tablesize;
	heads			= new (TAG_IDLIB_HASH) hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;

	for( i = 0; i < tablesize; i++ ) {
		if ( !map.heads[ i ] ) {
			heads[ i ] = NULL;
			continue;
		}

		prev = &heads[ i ];
		for( node = map.heads[ i ]; node != NULL; node = node->next ) {
			*prev = new (TAG_IDLIB_HASH) hashnode_s( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
}

/*
================
hfHashTable<Type>::~hfHashTable<Type>
================
*/
template< class Type >
HF_INLINE hfHashTable<Type>::~hfHashTable() {
	Clear();
	delete[] heads;
}

/*
================
hfHashTable<Type>::Allocated
================
*/
template< class Type >
HF_INLINE size_t hfHashTable<Type>::Allocated() const {
	return sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
hfHashTable<Type>::Size
================
*/
template< class Type >
HF_INLINE size_t hfHashTable<Type>::Size() const {
	return sizeof( hfHashTable<Type> ) + sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
hfHashTable<Type>::GetHash
================
*/
template< class Type >
HF_INLINE int hfHashTable<Type>::GetHash( const char *key ) const {
	return ( hfStr::Hash( key ) & tablesizemask );
}

/*
================
hfHashTable<Type>::Set
================
*/
template< class Type >
HF_INLINE void hfHashTable<Type>::Set( const char *key, Type &value ) {
	hashnode_s *node, **nextPtr;
	int hash, s;

	hash = GetHash( key );
	for( nextPtr = &(heads[hash]), node = *nextPtr; node != NULL; nextPtr = &(node->next), node = *nextPtr ) {
		s = node->key.Cmp( key );
		if ( s == 0 ) {
			node->value = value;
			return;
		}
		if ( s > 0 ) {
			break;
		}
	}

	numentries++;

	*nextPtr = new (TAG_IDLIB_HASH) hashnode_s( key, value, heads[ hash ] );
	(*nextPtr)->next = node;
}

/*
================
hfHashTable<Type>::Get
================
*/
template< class Type >
HF_INLINE bool hfHashTable<Type>::Get( const char *key, Type **value ) const {
	hashnode_s *node;
	int hash, s;

	hash = GetHash( key );
	for( node = heads[ hash ]; node != NULL; node = node->next ) {
		s = node->key.Cmp( key );
		if ( s == 0 ) {
			if ( value ) {
				*value = &node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}

	if ( value ) {
		*value = NULL;
	}

	return false;
}

/*
================
hfHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
HF_INLINE Type *hfHashTable<Type>::GetIndex( int index ) const {
	hashnode_s	*node;
	int			count;
	int			i;

	if ( ( index < 0 ) || ( index > numentries ) ) {
		assert( 0 );
		return NULL;
	}

	count = 0;
	for( i = 0; i < tablesize; i++ ) {
		for( node = heads[ i ]; node != NULL; node = node->next ) {
			if ( count == index ) {
				return &node->value;
			}
			count++;
		}
	}

	return NULL;
}

/*
================
hfHashTable<Type>::Remove
================
*/
template< class Type >
HF_INLINE bool hfHashTable<Type>::Remove( const char *key ) {
	hashnode_s	**head;
	hashnode_s	*node;
	hashnode_s	*prev;
	int			hash;

	hash = GetHash( key );
	head = &heads[ hash ];
	if ( *head ) {
		for( prev = NULL, node = *head; node != NULL; prev = node, node = node->next ) {
			if ( node->key == key ) {
				if ( prev ) {
					prev->next = node->next;
				} else {
					*head = node->next;
				}

				delete node;
				numentries--;
				return true;
			}
		}
	}

	return false;
}

/*
================
hfHashTable<Type>::Clear
================
*/
template< class Type >
HF_INLINE void hfHashTable<Type>::Clear() {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
hfHashTable<Type>::DeleteContents
================
*/
template< class Type >
HF_INLINE void hfHashTable<Type>::DeleteContents() {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
hfHashTable<Type>::Num
================
*/
template< class Type >
HF_INLINE int hfHashTable<Type>::Num() const {
	return numentries;
}

#if defined(HF_TYPEINFO)
#define __GNUC__ 99
#endif

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
hfHashTable<Type>::GetSpread
================
*/
template< class Type >
int hfHashTable<Type>::GetSpread() const {
	int i, average, error, e;
	hashnode_s	*node;

	// if no items in hash
	if ( !numentries ) {
		return 100;
	}
	average = numentries / tablesize;
	error = 0;
	for ( i = 0; i < tablesize; i++ ) {
		numItems = 0;
		for( node = heads[ i ]; node != NULL; node = node->next ) {
			numItems++;
		}
		e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - (error * 100 / numentries);
}
#endif

#if defined(HF_TYPEINFO)
#undef __GNUC__
#endif

#endif /* !__HASHTABLE_H__ */

