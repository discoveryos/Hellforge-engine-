#ifndef __STRPOOL_H__
#define __STRPOOL_H__

/*
===============================================================================

	hfStrPool

===============================================================================
*/

class hfStrPool;

class hfPoolStr : public hfStr {
	friend class hfStrPool;

public:
						hfPoolStr() { numUsers = 0; }
						~hfPoolStr() { assert( numUsers == 0 ); }

						// returns total size of allocated memory
	size_t				Allocated() const { return hfStr::Allocated(); }
						// returns total size of allocated memory including size of string pool type
	size_t				Size() const { return sizeof( *this ) + Allocated(); }
						// returns a pointer to the pool this string was allocated from
	const hfStrPool *	GetPool() const { return pool; }

private:
	hfStrPool *			pool;
	mutable int			numUsers;
};

class hfStrPool {
public:
						hfStrPool() { caseSensitive = true; }

	void				SetCaseSensitive( bool caseSensitive );

	int					Num() const { return pool.Num(); }
	size_t				Allocated() const;
	size_t				Size() const;

	const hfPoolStr *	operator[]( int index ) const { return pool[index]; }

	const hfPoolStr *	AllocString( const char *string );
	void				FreeString( const hfPoolStr *poolStr );
	const hfPoolStr *	CopyString( const hfPoolStr *poolStr );
	void				Clear();

private:
	bool				caseSensitive;
	hfList<hfPoolStr *>	pool;
	hfHashIndex			poolHash;
};

/*
================
hfStrPool::SetCaseSensitive
================
*/
HF_INLINE void hfStrPool::SetCaseSensitive( bool caseSensitive ) {
	this->caseSensitive = caseSensitive;
}

/*
================
hfStrPool::AllocString
================
*/
HF_INLINE const hfPoolStr *hfStrPool::AllocString( const char *string ) {
	int i, hash;
	hfPoolStr *poolStr;

	hash = poolHash.GenerateKey( string, caseSensitive );
	if ( caseSensitive ) {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Cmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	} else {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Icmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	}

	poolStr = new (TAG_IDLIB_STRING) hfPoolStr;
	*static_cast<hfStr *>(poolStr) = string;
	poolStr->pool = this;
	poolStr->numUsers = 1;
	poolHash.Add( hash, pool.Append( poolStr ) );
	return poolStr;
}

/*
================
hfStrPool::FreeString
================
*/
HF_INLINE void hfStrPool::FreeString( const hfPoolStr *poolStr ) {
	int i, hash;

	assert( poolStr->numUsers >= 1 );
	assert( poolStr->pool == this );

	poolStr->numUsers--;
	if ( poolStr->numUsers <= 0 ) {
		hash = poolHash.GenerateKey( poolStr->c_str(), caseSensitive );
		if ( caseSensitive ) { 
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Cmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		} else {
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Icmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		}
		assert( i != -1 );
		assert( pool[i] == poolStr );
		delete pool[i];
		pool.RemoveIndex( i );
		poolHash.RemoveIndex( hash, i );
	}
}

/*
================
hfStrPool::CopyString
================
*/
HF_INLINE const hfPoolStr *hfStrPool::CopyString( const hfPoolStr *poolStr ) {

	assert( poolStr->numUsers >= 1 );

	if ( poolStr->pool == this ) {
		// the string is from this pool so just increase the user count
		poolStr->numUsers++;
		return poolStr;
	} else {
		// the string is from another pool so it needs to be re-allocated from this pool.
		return AllocString( poolStr->c_str() );
	}
}

/*
================
hfStrPool::Clear
================
*/
HF_INLINE void hfStrPool::Clear() {
	int i;

	for ( i = 0; i < pool.Num(); i++ ) {
		pool[i]->numUsers = 0;
	}
	pool.DeleteContents( true );
	poolHash.Free();
}

/*
================
hfStrPool::Allocated
================
*/
HF_INLINE size_t hfStrPool::Allocated() const {
	int i;
	size_t size;

	size = pool.Allocated() + poolHash.Allocated();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Allocated();
	}
	return size;
}

/*
================
hfStrPool::Size
================
*/
HF_INLINE size_t hfStrPool::Size() const {
	int i;
	size_t size;

	size = pool.Size() + poolHash.Size();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Size();
	}
	return size;
}

#endif /* !__STRPOOL_H__ */

