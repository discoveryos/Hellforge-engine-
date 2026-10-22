#ifndef __HF_JSON_H__
#define __HF_JSON_H__

#include "../hfStr.h"
#include <string.h>
#include <stdlib.h>

/*
===============================================================================

	hfJson — minimal RFC-8259 JSON parser in a single header.

	Parses into a Value tree.  Child storage is pointer-based (each child
	Value and each object key hfStr is heap-allocated individually): the
	growable arrays only ever hold pointers, so realloc moves are safe even
	though hfStr keeps optional inline SSO storage.  Legacy malloc/free memory
	management keeps it callable from JIT-compiled scripts.  Call
	HF_Json::Free( root ) when done.  Used by the Phase 5 glTF importer and
	the editor tooling.

===============================================================================
*/

namespace HF_Json {

enum Type {
	JSON_NULL,
	JSON_BOOL,
	JSON_NUM,
	JSON_STR,
	JSON_ARR,
	JSON_OBJ
};

struct Value {
	Type			type;
	double			num;
	bool			b;
	hfStr			str;
	Value **		items;			// array of child Value pointers
	hfStr **		keys;			// object key pointers, index-parallel to items
	int				size;
	int				cap;

	HF_INLINE					Value()
	: type( JSON_NULL ), num( 0.0 ), b( false ), items( NULL ), keys( NULL ), size( 0 ), cap( 0 ) {
	}

	HF_INLINE void				Push() {				// creates one child (items[size]/keys[size])
		if ( cap <= size ) {
			int			ncap = cap ? cap * 2 : 8;
			Value **	nitems = ( Value ** )realloc( items, sizeof( Value * ) * ncap );
			hfStr **	nkeys = ( hfStr ** )realloc( keys, sizeof( hfStr * ) * ncap );
			items = nitems;
			keys = nkeys;
			cap = ncap;
		}
		items[ size ] = new Value();
		keys[ size ] = new hfStr();
		size++;
	}

	HF_INLINE const Value *		Get( int i ) const { return ( i >= 0 && i < size ) ? items[ i ] : NULL; }
	HF_INLINE const Value *		Find( const char *key ) const {
		if ( type != JSON_OBJ ) {
			return NULL;
		}
		for ( int i = 0; i < size; i++ ) {
			if ( *keys[ i ] == key ) {
				return items[ i ];
			}
		}
		return NULL;
	}
	HF_INLINE int				Count() const { return size; }
	HF_INLINE bool				IsNum() const { return type == JSON_NUM; }
	HF_INLINE bool				IsStr() const { return type == JSON_STR; }
	HF_INLINE bool				IsArr() const { return type == JSON_ARR; }
	HF_INLINE bool				IsObj() const { return type == JSON_OBJ; }
	HF_INLINE const char *		AsStr( const char *fallback ) const { return type == JSON_STR ? str.c_str() : fallback; }

	HF_INLINE double			AsVec( int i, double fallback ) const {	// element of a JSON array
		const Value *	e = Get( i );
		return ( e && e->IsNum() ) ? e->num : fallback;
	}
};

HF_INLINE void	Free( Value &v ) {
	if ( v.items ) {
		for ( int i = 0; i < v.size; i++ ) {
			if ( v.items[ i ] ) {
				if ( v.type == JSON_ARR || v.type == JSON_OBJ ) {
					Free( *v.items[ i ] );
				}
				delete v.items[ i ];
			}
			delete v.keys[ i ];
		}
		free( v.items );
		free( v.keys );
	}
	v.items = NULL;
	v.keys = NULL;
	v.size = 0;
	v.cap = 0;
}

namespace Detail {

HF_INLINE void	SkipWs( char *&s, const char *end ) {
	while ( s < end && ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) ) {
		s++;
	}
}

HF_INLINE int	HexVal( char c ) {
	if ( c >= '0' && c <= '9' ) return c - '0';
	if ( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
	if ( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
	return -1;
}

HF_INLINE void	ParseEscapeSequence( char *&s, const char *end, hfStr &out ) {
	switch ( *s ) {
	case '"':	out.Append( '"' ); s++; return;
	case '\\':	out.Append( '\\' ); s++; return;
	case '/':	out.Append( '/' ); s++; return;
	case 'b':	out.Append( '\b' ); s++; return;
	case 'f':	out.Append( '\f' ); s++; return;
	case 'n':	out.Append( '\n' ); s++; return;
	case 'r':	out.Append( '\r' ); s++; return;
	case 't':	out.Append( '\t' ); s++; return;
	case 'u': {
		s++;
		if ( s + 4 > end ) return;
		int			code = 0;
		for ( int k = 0; k < 4; k++ ) {
			int		h = HexVal( s[ k ] );
			if ( h < 0 ) return;
			code = ( code << 4 ) | h;
		}
		s += 4;
		if ( code < 0x80 ) {
			out.Append( ( char )code );
		} else if ( code < 0x800 ) {
			out.Append( ( char )( 0xC0 | ( code >> 6 ) ) );
			out.Append( ( char )( 0x80 | ( code & 0x3F ) ) );
		} else {
			out.Append( ( char )( 0xE0 | ( code >> 12 ) ) );
			out.Append( ( char )( 0x80 | ( ( code >> 6 ) & 0x3F ) ) );
			out.Append( ( char )( 0x80 | ( code & 0x3F ) ) );
		}
		return;
	}
	default:
		return;
	}
}

HF_INLINE bool	ParseString( char *&s, const char *end, hfStr &out ) {
	if ( s >= end || *s != '"' ) {
		return false;
	}
	s++;
	const char *	start = s;
	for ( const char *c = s; c < end; c++ ) {
		if ( *c == '"' ) {
			out.Clear();				// no escapes hit: bulk copy [start, c)
			out.Append( start, c - start );
			s = ( char * )c + 1;
			return true;
		}
		if ( *c == '\\' ) {
			break;						// slow path below
		}
	}
	out.Clear();						// slow path (escape handling)
	while ( s < end && *s != '"' ) {
		if ( *s == '\\' ) {
			s++;
			if ( s >= end ) {
				return false;
			}
			ParseEscapeSequence( s, end, out );
			continue;
		}
		out.Append( *s );
		s++;
	}
	if ( s >= end ) {
		return false;
	}
	s++;								// closing quote
	return true;
}

HF_INLINE bool	ParseValue( char *&s, const char *end, Value &out );

HF_INLINE bool	ParseArray( char *&s, const char *end, Value &out ) {
	out.type = JSON_ARR;
	for ( ;; ) {
		SkipWs( s, end );
		if ( s >= end ) {
			return false;
		}
		if ( *s == ']' ) {
			s++;
			return true;
		}
		out.Push();
		if ( !ParseValue( s, end, *out.items[ out.size - 1 ] ) ) {
			return false;
		}
		SkipWs( s, end );
		if ( s >= end ) {
			return false;
		}
		if ( *s == ',' ) {
			s++;
			continue;
		}
		if ( *s == ']' ) {
			s++;
			return true;
		}
		return false;
	}
}

HF_INLINE bool	ParseObject( char *&s, const char *end, Value &out ) {
	out.type = JSON_OBJ;
	for ( ;; ) {
		SkipWs( s, end );
		if ( s >= end ) {
			return false;
		}
		if ( *s == '}' ) {
			s++;
			return true;
		}
		if ( *s != '"' ) {
			return false;
		}
		hfStr		keyTmp;
		if ( !ParseString( s, end, keyTmp ) ) {
			return false;
		}
		SkipWs( s, end );
		if ( s >= end || *s != ':' ) {
			return false;
		}
		s++;
		out.Push();
		*out.keys[ out.size - 1 ] = keyTmp;
		if ( !ParseValue( s, end, *out.items[ out.size - 1 ] ) ) {
			return false;
		}
		SkipWs( s, end );
		if ( s >= end ) {
			return false;
		}
		if ( *s == ',' ) {
			s++;
			continue;
		}
		if ( *s == '}' ) {
			s++;
			return true;
		}
		return false;
	}
}

HF_INLINE bool	ParseValue( char *&s, const char *end, Value &out ) {
	SkipWs( s, end );
	if ( s >= end ) {
		return false;
	}
	switch ( *s ) {
	case '{':
		s++;
		return ParseObject( s, end, out );
	case '[':
		s++;
		return ParseArray( s, end, out );
	case '"':
		out.type = JSON_STR;
		return ParseString( s, end, out.str );
	case 't':
		if ( end - s >= 4 && !memcmp( s, "true", 4 ) ) { out.type = JSON_BOOL; out.b = true; s += 4; return true; }
		return false;
	case 'f':
		if ( end - s >= 5 && !memcmp( s, "false", 5 ) ) { out.type = JSON_BOOL; out.b = false; s += 5; return true; }
		return false;
	case 'n':
		if ( end - s >= 4 && !memcmp( s, "null", 4 ) ) { out.type = JSON_NULL; s += 4; return true; }
		return false;
	default: {
		char *		next = NULL;
		double		num = strtod( s, &next );
		if ( next > s ) {
			out.type = JSON_NUM;
			out.num = num;
			s = next;
			return true;
		}
		return false;
	}
	}
}

} // namespace Detail

// Parses [text, text+len).  On failure returns false and sets error.
HF_INLINE bool	Parse( const char *text, size_t len, Value &root, hfStr &error ) {
	char *		s = ( char * )text;
	char *		end = ( char * )text + len;
	Detail::SkipWs( s, end );
	if ( !Detail::ParseValue( s, end, root ) ) {
		error = "malformed JSON";
		return false;
	}
	Detail::SkipWs( s, end );
	if ( s != end ) {
		error = "trailing data after JSON value";
		return false;
	}
	return true;
}

// Convenience accessors used by the glTF importer.
HF_INLINE double	GetNum( const Value &obj, const char *key, double fallback ) {
	const Value *	v = obj.Find( key );
	return ( v && v->IsNum() ) ? v->num : fallback;
}

HF_INLINE bool		GetBool( const Value &obj, const char *key, bool fallback ) {
	const Value *	v = obj.Find( key );
	return ( v && v->type == JSON_BOOL ) ? v->b : fallback;
}

HF_INLINE const char *	GetStr( const Value &obj, const char *key, const char *fallback ) {
	const Value *	v = obj.Find( key );
	return ( v && v->IsStr() ) ? v->str.c_str() : fallback;
}

HF_INLINE double		AsVecD( const Value &obj, const char *key, int idx, double fallback ) {
	const Value *	arr = obj.Find( key );
	return ( arr && arr->IsArr() ) ? arr->AsVec( idx, fallback ) : fallback;
}

} // namespace HF_Json

#endif