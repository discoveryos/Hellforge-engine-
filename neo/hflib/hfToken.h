#ifndef __TOKEN_H__
#define __TOKEN_H__

/*
===============================================================================

	hfToken is a token read from a file or memory with hfLexer or hfParser

===============================================================================
*/

// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid

// string sub type is the length of the string
// literal sub type is the ASCII code
// punctuation sub type is the punctuation id
// name sub type is the length of the name

class hfToken : public hfStr {

	friend class hfParser;
	friend class hfLexer;

public:
	int				type;								// token type
	int				subtype;							// token sub type
	int				line;								// line in script the token was on
	int				linesCrossed;						// number of lines crossed in white space before token
	int				flags;								// token flags, used for recursive defines

public:
					hfToken();
					hfToken( const hfToken *token );
					~hfToken();

	void			operator=( const hfStr& text );
	void			operator=( const char *text );

	double			GetDoubleValue();				// double value of TT_NUMBER
	float			GetFloatValue();				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue();		// unsigned long value of TT_NUMBER
	int				GetIntValue();				// int value of TT_NUMBER
	int				WhiteSpaceBeforeToken() const;// returns length of whitespace before token
	void			ClearTokenWhiteSpace();		// forget whitespace before token

	void			NumberValue();				// calculate values for a TT_NUMBER

private:
	unsigned long	intvalue;							// integer value
	double			floatvalue;							// floating point value
	const char *	whiteSpaceStart_p;					// start of white space before token, only used by hfLexer
	const char *	whiteSpaceEnd_p;					// end of white space before token, only used by hfLexer
	hfToken *		next;								// next token in chain, only used by hfParser

	void			AppendDirty( const char a );		// append character without adding trailing zero
};

HF_INLINE hfToken::hfToken() : type(), subtype(), line(), linesCrossed(), flags() {
}

HF_INLINE hfToken::hfToken( const hfToken *token ) {
	*this = *token;
}

HF_INLINE hfToken::~hfToken() {
}

HF_INLINE void hfToken::operator=( const char *text) {
	*static_cast<hfStr *>(this) = text;
}

HF_INLINE void hfToken::operator=( const hfStr& text ) {
	*static_cast<hfStr *>(this) = text;
}

HF_INLINE double hfToken::GetDoubleValue() {
	if ( type != TT_NUMBER ) {
		return 0.0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return floatvalue;
}

HF_INLINE float hfToken::GetFloatValue() {
	return (float) GetDoubleValue();
}

HF_INLINE unsigned long	hfToken::GetUnsignedLongValue() {
	if ( type != TT_NUMBER ) {
		return 0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return intvalue;
}

HF_INLINE int hfToken::GetIntValue() {
	return (int) GetUnsignedLongValue();
}

HF_INLINE int hfToken::WhiteSpaceBeforeToken() const {
	return ( whiteSpaceEnd_p > whiteSpaceStart_p );
}

HF_INLINE void hfToken::AppendDirty( const char a ) {
	EnsureAlloced( len + 2, true );
	data[len++] = a;
}

#endif /* !__TOKEN_H__ */

