#ifndef HFSTRSTATIC_H
#define HFSTRSTATIC_H

#include "hfStr.h"

#define HF_STATICBUF_LEN( dst, src )	( ( dst ) = ( (int)( sizeof( src ) ) - 1 ) )

/*
===============================================================================
	hfStrStatic is an hfStr that stores its character data inside the object
	itself, avoiding a heap allocation. Construction points the hfStr data
	pointer at the inline buffer.
===============================================================================
*/
template< int _size_ >
class hfStrStatic : public hfStr {
protected:
	char				buffer[ _size_ ];

public:
	HF_INLINE				hfStrStatic() {
		SetStaticBuffer( buffer, _size_ );
	}

	HF_INLINE				hfStrStatic( const char *str ) {
		SetStaticBuffer( buffer, _size_ );
		Copy( str );
	}

	HF_INLINE				hfStrStatic( const hfStr &str ) {
		SetStaticBuffer( buffer, _size_ );
		Copy( str );
	}

	HF_INLINE				~hfStrStatic() {
		// the data lives in the object, so make sure the base class
		// destructor does not attempt to free it
		SetStaticBuffer( baseBuffer, BASE_STRING_BUF );
	}

	HF_INLINE void		operator=( const char *str ) { Copy( str ); }
	HF_INLINE void		operator=( const hfStr &str ) { Copy( str ); }

	HF_INLINE int			Length() const { return hfStr::Length(); }

	// size of the static buffer in use
	HF_INLINE int			StaticSize() const { return _size_; }

	// number of characters that still fit
	HF_INLINE int			Available() const { return ( _size_ - 1 ) - hfStr::Length(); }
};

#endif
