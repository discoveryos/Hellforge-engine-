#ifndef __BASE64_H__
#define __BASE64_H__

/*
===============================================================================

	base64

===============================================================================
*/

class hfBase64 {
public:
				hfBase64();
				hfBase64( const hfStr &s );
				~hfBase64();

	void		Encode( const byte *from, int size );
	void		Encode( const hfStr &src );
	int			DecodeLength() const; // minimum size in bytes of destination buffer for decoding
	int			Decode( byte *to ) const; // does not append a \0 - needs a DecodeLength() bytes buffer
	void		Decode( hfStr &dest ) const; // decodes the binary content to an hfStr (a bit dodgy, \0 and other non-ascii are possible in the decoded content)
	void		Decode( hfFile *dest ) const;

	const char	*c_str() const;

	void 		operator=( const hfStr &s );

private:
	byte *		data;
	int			len;
	int			alloced;

	void		Init();
	void		Release();
	void		EnsureAlloced( int size );
};

HF_INLINE hfBase64::hfBase64() {
	Init();
}

HF_INLINE hfBase64::hfBase64( const hfStr &s ) {
	Init();
	*this = s;
}

HF_INLINE hfBase64::~hfBase64() {
	Release();
}

HF_INLINE const char *hfBase64::c_str() const {
	return (const char *)data;
}

HF_INLINE void hfBase64::Init() {
	len = 0;
	alloced = 0;
	data = NULL;
}

HF_INLINE void hfBase64::Release() {
	if ( data ) {
		delete[] data;
	}
	Init();
}

HF_INLINE void hfBase64::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new (TAG_IDLIB) byte[size];
	alloced = size;
}

HF_INLINE void hfBase64::operator=( const hfStr &s ) {
	EnsureAlloced( s.Length()+1 ); // trailing \0 - beware, this does a Release
	strcpy( (char *)data, s.c_str() );
	len = s.Length();
}

#endif /* !__BASE64_H__ */

