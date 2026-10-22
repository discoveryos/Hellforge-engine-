#ifndef HFSTR_H
#define HFSTR_H

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef HF_INLINE
#define HF_INLINE inline
#endif

// controls the size of the inline buffer before switching to the heap
#define BASE_STRING_BUF		256

// string allocation granularity
#define STR_ALLOC_GRAN		1024

// maximum stack-allocated string size
#define MAX_MEDIUM_STRING	( BASE_STRING_BUF * 8 )

// initial hash value
#define STRING_HASH_INIT	5381

// file name hash table size (must be a power of two)
#define FILE_HASH_SIZE		256

// units for BestUnit/SetUnit
enum Measure_t
{
	MEASURE_SIZE = 0,
	MEASURE_TIME
};

// result of UTF-8 validation
enum utf8Encoding_t
{
	UTF8_INVALID = 0,
	UTF8_ASCII,
	UTF8_UTF2,
	UTF8_UTF3,
	UTF8_UTF4,
	UTF8_UTF5,
	UTF8_UTF6,
	UTF8_UTF7,
	UTF8_UTF8,
	UTF8_ENCODED_BOM,
	UTF8_PURE_ASCII,
	UTF8_ENCODED_NO_BOM,
	UTF8_INVALID_BOM
};

class hfCmdArgs;
class hfVec4;

// temporary string used by the global va() helper, defined in hfStr.cpp
char *va( const char *fmt, ... );

struct hfStr {
	// construction / destruction
	HF_INLINE hfStr() : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
	}

	HF_INLINE hfStr( const char *text ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		if ( text ) {
			len = (int)strlen( text );
			EnsureAlloced( len + 1, false );
			memcpy( this->data, text, len );
			this->data[ len ] = '\0';
		}
	}

	HF_INLINE hfStr( const hfStr &text ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		*this = text;
	}

	HF_INLINE hfStr( const int value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		*this = value;
	}

	HF_INLINE hfStr( const unsigned int value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		*this = (int)value;
	}

	HF_INLINE hfStr( const unsigned long value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		*this = (unsigned long long)value;
	}

	HF_INLINE hfStr( const unsigned long long value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		sprintf( baseBuffer, "%llu", value );
		len = (int)strlen( baseBuffer );
	}

	HF_INLINE hfStr( const float value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		sprintf( baseBuffer, "%.6f", value );
		len = (int)strlen( baseBuffer );
	}

	HF_INLINE hfStr( const double value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		sprintf( baseBuffer, "%.6f", value );
		len = (int)strlen( baseBuffer );
	}

	HF_INLINE hfStr( const bool value ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		if ( value ) { baseBuffer[ 0 ] = '1'; baseBuffer[ 1 ] = '\0'; len = 1; } else { baseBuffer[ 0 ] = '0'; baseBuffer[ 1 ] = '\0'; len = 1; }
	}

	// substring of a C string, [start,end] inclusive
	HF_INLINE hfStr( const char *text, const int start, const int end ) : data( baseBuffer ), len( 0 ), alloced( BASE_STRING_BUF ), allowLargeStrings( true ), buffer( baseBuffer ) {
		baseBuffer[ 0 ] = '\0';
		hash = 0;
		if ( text ) {
			int l = end - start + 1;
			if ( l < 0 ) { l = 0; }
			Copy( text + start, l );
		}
	}

	HF_INLINE ~hfStr() {
		if ( data && data != baseBuffer && data != buffer ) {
			FreeData();
		}
	}

	// assignment
	void				operator=( const char *text );
	HF_INLINE void		operator=( const int value ) { Set( "%i", value ); }
	HF_INLINE void		operator=( const unsigned int value ) { Set( "%u", value ); }
	HF_INLINE void		operator=( const unsigned long value ) { Set( "%lu", value ); }
	HF_INLINE void		operator=( const unsigned long long value ) { Set( "%llu", value ); }
	HF_INLINE void		operator=( const float value ) { Set( "%f", value ); }
	HF_INLINE void		operator=( const double value ) { Set( "%.6f", value ); }
	HF_INLINE void		operator=( const bool value ) { Set( value ? "1" : "0" ); }
	HF_INLINE void		operator=( const hfStr &text ) {
		if ( &text != this ) {
			ReAllocate( text.len + 1, false );
			memcpy( this->data, text.data, text.len + 1 );
			len = text.len;
		}
	}

	// conversion to const char *
	HF_INLINE operator const char *() const { return data; }
	HF_INLINE operator char *() { return data; }

	// concatenation
	HF_INLINE hfStr		operator+( const char *text ) const { hfStr s( *this ); s += text; return s; }
	HF_INLINE hfStr		operator+( const char c ) const { hfStr s( *this ); s += c; return s; }
	HF_INLINE hfStr		operator+( const hfStr &text ) const { hfStr s( *this ); s += text; return s; }

	// equality
	bool				operator==( const hfStr &text ) const { return Cmp( text ) == 0; }
	HF_INLINE bool		operator!=( const char *text ) const { return Cmp( text ) != 0; }
	HF_INLINE bool		operator!=( const hfStr &text ) const { return Cmp( text ) != 0; }

	// basic sizing / state
	HF_INLINE int		Length() const { return len; }
	HF_INLINE bool		IsEmpty() const { return len == 0; }
	HF_INLINE void		Empty() const { const_cast<hfStr *>( this )->len = 0; data[ 0 ] = '\0'; }
	HF_INLINE int		Size() const { return alloced; }
	HF_INLINE int		AllocedLength() const { return alloced; }
	HF_INLINE int		GetAlloced() const { return alloced; }
	HF_INLINE void		SetAlloced( int amount ) { alloced = amount; }
	HF_INLINE bool		Allocated() const { return data != baseBuffer; }
	// bytes this string owns on the heap; strings still living in the inline
	// (or static) buffer report zero
	HF_INLINE int		DynamicMemoryUsed() const { return ( data != baseBuffer && data != buffer ) ? alloced : 0; }

	// data access
	HF_INLINE char *		Buffer() { return data; }
	HF_INLINE const char *Buffer() const { return data; }
	HF_INLINE char *		c_str() { return data; }
	HF_INLINE const char *c_str() const { return data; }

	// hash
	HF_INLINE int		Hash() const { return hash; }
	static HF_INLINE int IHash( const char *string, int length ) {
		int i, h = 0;
		for ( i = 0; i < length; i++ ) { h = ( h << 5 ) - h + ( (int)( (unsigned char)string[ i ] ) ); }
		return h;
	}
	static HF_INLINE int	Hash( const char *string ) { return string ? IHash( string, (int)strlen( string ) ) : 0; }
	static HF_INLINE int	IHash( const char *string ) { return Hash( string ); }
	static HF_INLINE int	IHash( const hfStr &string ) { return IHash( string.data, string.len ); }
	static HF_INLINE int	Hash( const hfStr &string ) { return IHash( string.data, string.len ); }
	HF_INLINE void		Hash( int h ) { hash = h; }

	// clearing
	HF_INLINE void		Clear() { len = 0; data[ 0 ] = '\0'; }
	HF_INLINE void		Reset() { Clear(); }

	// memory management
	void				ReAllocate( int amount, bool keepold = true );
	void				FreeData();
	HF_INLINE void		Reserve( int amount ) { ReAllocate( amount, true ); }

	HF_INLINE void		EnsureAlloced( int l, bool keepold = true ) {
		int difference = l - alloced;
		if ( difference > 0 ) {
			ReAllocate( alloced + difference, keepold );
		}
	}

	// modification
	HF_INLINE hfStr &	operator+=( const char *text ) { Append( text ); return *this; }
	HF_INLINE hfStr &	operator+=( const char c ) { Append( c ); return *this; }
	HF_INLINE hfStr &	operator+=( const hfStr &text ) { Append( text ); return *this; }

	HF_INLINE void		Append( const char *text ) { if ( text ) { Append( text, (int)strlen( text ) ); } }
	HF_INLINE void		Append( char c ) { Append( &c, 1 ); }
	HF_INLINE void		Append( const hfStr &text ) { Append( text.data, text.len ); }
	HF_INLINE void		Append( const char *text, int length ) {
		if ( !text || length <= 0 ) { return; }
		if ( len + length + 1 > alloced ) { ReAllocate( len + length + 1, true ); }
		memcpy( data + len, text, length );
		len += length;
		data[ len ] = '\0';
	}
	HF_INLINE void		Insert( const char c, int index ) { char t[2]; t[0] = c; t[1] = '\0'; Insert( t, index ); }
	HF_INLINE void		Insert( const char *text, int index ) {
		if ( !text ) { return; }
		int l = (int)strlen( text );
		if ( l == 0 ) { return; }
		if ( index < 0 ) { index = 0; }
		if ( index > len ) { index = len; }
		if ( len + l + 1 > alloced ) { ReAllocate( len + l + 1, true ); }
		memmove( data + index + l, data + index, len - index + 1 );
		memcpy( data + index, text, l );
		len += l;
	}
	void				AppendUTF8Char( uint32 c );
	void				Delete( int start, int len );
	void				Delete( int len );
	// removes every occurrence of c
	HF_INLINE void		Strip( const char c ) {
		int i, j = 0;
		for ( i = 0; i < len; i++ ) {
			if ( data[ i ] != c ) { data[ j++ ] = data[ i ]; }
		}
		data[ j ] = '\0';
		len = j;
	}
	HF_INLINE void		Strip( const char *chars ) {
		if ( !chars ) { return; }
		int i, j = 0;
		for ( i = 0; i < len; i++ ) {
			if ( !strchr( chars, data[ i ] ) ) { data[ j++ ] = data[ i ]; }
		}
		data[ j ] = '\0';
		len = j;
	}
	void				DeleteChar( const char c, bool caseSensitive = true );
	void				DeleteChar( const char *chars, bool caseSensitive = true );
	HF_INLINE void		Truncate( int n ) { if ( n < len ) { len = n; data[ len ] = '\0'; } }
	HF_INLINE void		TruncateLeft( int n ) { if ( n < len ) { memmove( data, data + n, len - n ); len -= n; data[ len ] = '\0'; } }

	// setting
	void				Print( const char *fmt, ... );
	void				Format( const char *fmt, ... );
	void				FormatVA( const char *fmt, va_list argptr );
	void				Set( const char *fmt, ... );
	void				SetVA( const char *fmt, va_list argptr );
	HF_INLINE void		Copy( const char *text ) { if ( !text ) { Clear(); return; } Copy( text, (int)strlen( text ) ); }
	HF_INLINE void		Copy( const char *text, int l ) {
		if ( !text ) { Clear(); return; }
		if ( l < 0 ) { l = 0; }
		if ( l + 1 > alloced ) { ReAllocate( l + 1, false ); }
		memmove( data, text, l );
		len = l;
		data[ len ] = '\0';
	}
	HF_INLINE void		Copy( const hfStr &text ) { Copy( text.data, text.len ); }
	void				Copy( const hfStr &text, int start, int len );
	void				Copy( const char *text, const char *stop, bool escape = false );

	// character access
	HF_INLINE char &		operator[]( int index ) { return data[ index ]; }
	HF_INLINE const char &operator[]( int index ) const { return data[ index ]; }

	// returned by Last() when the character is not present
	static const int		INVALID_POSITION = -1;

	// copies a byte range out of an external buffer
	HF_INLINE void		CopyRange( const char *src, int start, int end ) {
		int l = end - start + 1;
		if ( l < 0 ) { l = 0; }
		Copy( src + start, l );
	}

	// fills the string with n copies of c
	HF_INLINE void		Fill( const char c, int n ) {
		if ( n < 0 ) { n = 0; }
		EnsureAlloced( n + 1, false );
		memset( data, c, n );
		len = n;
		data[ len ] = '\0';
	}

	// keeps at most n characters of the string
	HF_INLINE void		CapLength( int n ) { if ( n < len ) { len = n; data[ len ] = '\0'; } }

	// lookups
	HF_INLINE int		Find( const char *text, bool casesensitive = true ) const {
		if ( !text ) {
			return -1;
		}
		if ( casesensitive ) {
			const char *f = strstr( data, text );
			return f ? (int)( f - data ) : -1;
		}
		/*
			The match lands inside the temporary lowercase copy, so it has to be
			measured against that copy. Subtracting 'data' would compare two
			unrelated buffers and report a nonsense offset.
		*/
		hfStr lower( data );
		lower.ToLower();
		hfStr t( text );
		t.ToLower();
		const char *f = strstr( lower, t );
		return f ? (int)( f - lower.c_str() ) : -1;
	}
	HF_INLINE int		Find( const char c, bool casesensitive = true ) const {
		if ( casesensitive ) {
			const char *p = strchr( data, c );
			return p ? (int)( p - data ) : -1;
		}
		hfStr lower( data );
		lower.ToLower();
		const char *f = strchr( lower, ToLower( c ) );
		return f ? (int)( f - lower.c_str() ) : -1;
	}
	static int			FindChar( const char *str, const char c, int start, int end );
	// index of c in a plain C string, or -1
	static int			FindChar( const char *str, const char c ) {
		const char *f = str ? strchr( str, c ) : NULL;
		return f ? (int)( f - str ) : -1;
	}
	const char *			FindText( const char *text, const bool casesensitive = true, const int start = 0, const int end = -1 ) const;
	static int			FindText( const char *str, const char *text, bool casesensitive, int start, int end = -1 );
	// index of text within str over the whole string; -1 when not found
	static int			FindText( const char *str, const char *text, bool casesensitive = true ) {
		return FindText( str, text, casesensitive, 0, -1 );
	}
	// index form used by Find(); searches the string starting at offset 'start'
	HF_INLINE int			FindText( const char *text, bool casesensitive, int start ) const {
		if ( !text ) {
			return -1;
		}
		if ( casesensitive ) {
			const char *f = strstr( data + start, text );
			return f ? (int)( f - data ) : -1;
		}
		/*
			The lowercase copy is a temporary, so the match has to be turned into
			an offset while it is still alive; holding the pointer and measuring
			it afterwards would read a destroyed buffer.
		*/
		hfStr lower( data + start );
		lower.ToLower();
		hfStr t( text );
		t.ToLower();
		const char *f = strstr( lower, t );
		return f ? (int)( f - lower.c_str() ) + start : -1;
	}
	int					Last( const char c ) const;
	bool				StartsWith( const char *text, const bool caseSensitive = true ) const;
	bool				EndsWith( const char *text, const bool caseSensitive = true ) const;

	// comparisons
	static int			Length( const char *string ) { return string ? (int)strlen( string ) : 0; }
	int					Cmp( const char *s2 ) const { return Cmp( data, s2 ); }
	int					Icmp( const char *s2 ) const { return Icmp( data, s2 ); }
	HF_INLINE int		Icmpn( const char *s2, int n ) const { return Icmpn( data, s2, n ); }
	// returns 0 when s2 is a case insensitive prefix of this string
	HF_INLINE int		IcmpPrefix( const char *s2 ) const { return s2 ? Icmpn( s2, Length( s2 ) ) : -1; }
	HF_INLINE int		IcmpPath( const char *s2 ) const { return IcmpPath( data, s2 ); }
	HF_INLINE int		IcmpPrefixPath( const char *s2 ) const { return IcmpPrefixPath( data, s2 ); }
	HF_INLINE int		IcmpNoColor( const char *s2 ) const { return IcmpNoColor( data, s2 ); }
	HF_INLINE int		Cmpn( const char *s2, int n ) const { return Cmpn( data, s2, n ); }
	HF_INLINE bool		operator==( const char *text ) const { return Icmp( text ) == 0; }
	int					StrCmp( const char *s2 ) const { return Cmp( data, s2 ); }
	HF_INLINE bool		Equal( const char *s2 ) const { return Cmp( s2 ) == 0; }
	HF_INLINE bool		Equal( const hfStr &s2 ) const { return Cmp( s2 ) == 0; }
	HF_INLINE bool		Compare( const char *s2 ) const { return Cmp( s2 ) == 0; }

	// case conversion
	HF_INLINE void		ToLower() { ToLower( (char *)data ); }
	HF_INLINE void		ToUpper() { ToUpper( (char *)data ); }
	// lower/case a C string in place (static forms, see below)
	HF_INLINE hfStr &	Lower() { ToLower(); return *this; }
	HF_INLINE hfStr &	Upper() { ToUpper(); return *this; }
	// ToLower folds an uppercase letter down; ToUpper folds a lowercase letter
	// up. Each must test the *opposite* case class, otherwise neither converts.
	static char			ToLower( const char c ) { return CharIsUpper( c ) ? c + ( 'a' - 'A' ) : c; }
	// in place lowercase of a caller owned C string
	static void			ToLower( char *string ) { if ( string ) { while ( *string ) { *string = ToLower( *string ); string++; } } }
	static void			ToUpper( char *string ) { if ( string ) { while ( *string ) { *string = ToUpper( *string ); string++; } } }
	static char			ToUpper( const char c ) { return CharIsLower( c ) ? c - ( 'a' - 'A' ) : c; }

	// replacing
	bool				Replace( const char *old, const char *nw );
	bool				ReplaceChar( const char old, const char nw );
	bool				ReplaceOnce( const char *old, const char *nw );

	// substring / split
	const char *			Mid( int start, int len, hfStr &result ) const;
	hfStr				Mid( int start, int len ) const;
	HF_INLINE const char *	Left( const int len, hfStr &result ) const {
		int l = len < Length() ? len : Length();
		result.Copy( data, l );
		return result;
	}
	HF_INLINE const char *	Right( const int len, hfStr &result ) const {
		int start = Length() - len;
		if ( start < 0 ) { start = 0; }
		result.Copy( data + start, Length() - start );
		return result;
	}
	HF_INLINE hfStr		Right( const int len ) const { hfStr r; Right( len, r ); return r; }
	HF_INLINE hfStr		Left( const int len ) const { hfStr r; Left( len, r ); return r; }

	// stripping (member versions)
	hfStr &				StripFilename();
	hfStr &				StripPath();
	void				StripLeading( const char c );
	void				StripLeading( const char *string );
	bool				StripLeadingOnce( const char *string );
	void				StripTrailing( const char c );
	void				StripTrailing( const char *string );
	bool				StripTrailingOnce( const char *string );
	void				StripLeadingWhitespace();
	void				StripTrailingWhitespace();
	hfStr &				StripQuotes();
	hfStr &				StripAbsoluteFileExtension();
	void				StripAbsolutePath( hfStr &prefix, int subpath = 0, bool only = false );

	// filename / path
	void				AppendPath( const char *text );
	hfStr &				BackSlashesToSlashes();
	hfStr &				SlashesToBackSlashes();
	hfStr &				SetFileExtension( const char *extension );
	hfStr &				StripFileExtension();
	hfStr &				DefaultFileExtension( const char *extension );
	hfStr &				DefaultPath( const char *basepath );
	void				ExtractFilePath( hfStr &dest ) const;
	void				ExtractFileName( hfStr &dest ) const;
	void				ExtractFileBase( hfStr &dest ) const;
	void				ExtractFileExtension( hfStr &dest ) const;
	int					FileNameHash() const;

	// formatted numbers
	hfStr				FormatInt( const int num, bool isCash );
	static hfStr			FormatNumber( int number );
	static const char *	FloatArrayToString( const float *array, const int length, const int precision );
	int					ParseNumbers( const char *string, int *numbers, int maxNumbers ) const;
	int					ParseNumbers( char *string, int *numbers, int maxNumbers ) const;

	// units
	int					BestUnit( const char *format, float value, Measure_t measure );

	// concatenation with a leading C string
	HF_INLINE friend hfStr operator+( const char *text, const hfStr &s ) { hfStr r( text ); r += s; return r; }
	void				SetUnit( const char *format, float value, int unit, Measure_t measure );
	void				SetUnit( const char *format, float value, Measure_t measure );
	void				SetPrecision( const char *format, int unit, Measure_t measure );

	// stream reading
	const char *			ReadString( int &readPos, int maxLength = 0 ) const;
	int					ReadInteger( int &readPos ) const;
	int					ReadInteger( int &readPos, int min, int max ) const;
	int					ReadHex( int &readPos ) const;
	int					ReadHex( int &readPos, int min, int max ) const;
	int					ReadBool( int &readPos ) const;
	int					ReadFloat( int &readPos ) const;

	// static buffer support (used by hfStrStatic)
	void				SetStaticBuffer( char *newBuffer, int newBufferSize );
	HF_INLINE char *	GetStaticBuffer() { return buffer; }
	HF_INLINE char *	GetBaseObject() const { return ( char * )baseBuffer; }
	HF_INLINE bool		IsStatic() const { return ( ( data > buffer ) && ( data < buffer + MAX_MEDIUM_STRING ) ) ? true : false; }
	HF_INLINE bool		Validate() const {
		if ( data < baseBuffer || ( data > buffer && data < buffer + MAX_MEDIUM_STRING ) ) {
			return false;
		}
		if ( (int)strlen( data ) != len ) {
			return false;
		}
		return true;
	}
	void				ForceRelAllocate();
	HF_INLINE bool		ColorStripped() const { return colorStripped; }
	HF_INLINE void		SetColorStripped( bool s ) { colorStripped = s; }
	HF_INLINE void		StripColors() { colorStripped = true; }

	// static comparison / utility
	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, int n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, int n );
	static int			IcmpNoColor( const char *s1, const char *s2 );
	static int			IcmpPath( const char *s1, const char *s2 );
	static int			IcmpnPath( const char *s1, const char *s2, int n );
	// returns 0 when s2 is a path prefix of s1, ignoring case and slash direction
	static int			IcmpPrefixPath( const char *s1, const char *s2 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	// true when this name matches the given filter pattern
	bool				Filter( const char *pattern, bool caseSensitive = false ) const { return Filter( pattern, data, caseSensitive ); }
	static bool			CheckExtension( const char *name, const char *ext );
	HF_INLINE bool		CheckExtension( const char *ext ) const { return CheckExtension( data, ext ); }
	static void			StripMediaName( const char *name, hfStr &mediaName );
	static bool			IsNumeric( const char *s );
	HF_INLINE bool		IsNumeric() const { return IsNumeric( data ); }
	static HF_INLINE bool CharIsLower( const char c ) { return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF && c != 0xF7 ); }
	static HF_INLINE bool CharIsUpper( const char c ) { return ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xDE && c != 0xD7 ); }
	// 7 bit printable ASCII. Anything with the high bit set is assumed to be an
	// extended character and treated as printable, which is what the widget and
	// text entry code expects when it asks about a raw key code.
	static HF_INLINE bool CharIsPrintable( const char c ) {
		if ( ( (unsigned char)c & 0x80 ) != 0 ) { return true; }
		return c >= 0x20 && c < 0x7F;
	}
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
	static void			Copynz( char *dest, const char *src, int destsize );
	static void			Append( char *dest, int size, const char *src );
	static bool			IsValidUTF8( const uint8 *s, const int maxLen, utf8Encoding_t &encoding );
	static int			UTF8Length( const byte *s );
	static uint32		UTF8Char( const byte *s, int &idx );
	// decodes the character at idx and advances idx past the bytes it used
	HF_INLINE uint32	UTF8Char( int &idx ) const { return UTF8Char( (const byte *)data, idx ); }
	static int			LengthWithoutColors( const char *s );
	HF_INLINE int		LengthWithoutColors() const { return LengthWithoutColors( data ); }
	static char *		RemoveColors( char *string );
	// in place color removal on this string
	HF_INLINE char *		RemoveColors() { return RemoveColors( data ); }
	static int			snPrintf( char *dest, int size, const char *fmt, ... );
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );
	static void			InitMemory();
	static void			ShutdownMemory();
	static void			PurgeMemory();
	static void			ShowMemoryUsage_f( const hfCmdArgs &args );
	static const char *	CStyleQuote( const char *str );
	static const char *	CStyleUnQuote( const char *str );
	static hfVec4 &		ColorForIndex( int i );
	// Maps a color code (the character after S_COLOR_ESCAPE) onto the index
	// used by the color table. Values that are already table indices pass
	// through unchanged, which keeps console packing round-trip safe.
	static HF_INLINE int ColorIndex( int c ) {
		if ( c >= '0' && c <= '9' ) { return c - '0'; }
		if ( c >= 0 && c <= 15 ) { return c; }
		return 0;
	}
	static bool			IsColor( const char *s ) {
		return ( s && *s == '^' && s[ 1 ] && s[ 1 ] != '^' && ( s[ 2 ] == 0 || s[ 2 ] == '^' ) );
	}
	static bool			IsValidUTF( const char *s );

	// static path / filename helpers
	static void			AppendPath( hfStr &dest, const char *text, const char *sep = "/" );
	static void			StripPath( const char *text, hfStr &dest );
	static void			StripFileExtension( const char *text, hfStr &dest );
	static void			StripAbsoluteFileExtension( const char *text, hfStr &dest );
	static void			StripFilename( const char *text, hfStr &dest );
	static void			StripLeading( const char *text, hfStr &dest, const char c );
	static void			StripLeading( const char *text, hfStr &dest, const char *string );
	static void			StripTrailing( const char *text, hfStr &dest, const char c );
	static void			StripTrailing( const char *text, hfStr &dest, const char *string );
	static void			ExtractFilePath( const char *text, hfStr &dest );
	static void			ExtractFileName( const char *text, hfStr &dest );
	static void			ExtractFileBase( const char *text, hfStr &dest );
	static void			ExtractFileExtension( const char *text, hfStr &dest );
	static void			Copy( const char *source, hfStr &dest, const int start, const int length );
	static void			Copy( const char *source, hfStr &dest, const int start );
	static void			Copy( const char *source, hfStr &dest );
	static void			Copy( const hfStr &source, hfStr &dest, const int start, const int length );
	static void			Copy( const hfStr &source, hfStr &dest );
	static void			Copy( const char *source, hfStr &dest, const char *stop, bool escape = false );
	static void			Append( const char *dest, const int destSize, const char *src, int srcLen = -1 );
	static void			Append( char *dest, int size, const char *src, int srcLen );
	static void			Append( char *dest, int size, const char c );
	static void			EnsureTotalLength( hfStr &dest, const int length );
	static void			FixPathSlashes( char c, hfStr &dest );

	// data
	char *				data;
	int					len;
	int					alloced;
	bool				allowLargeStrings;
	char				baseBuffer[ BASE_STRING_BUF ];
	char *				buffer;
	int					hash;
	bool				colorStripped;
};

#endif
