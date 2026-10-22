#ifndef __MATH_COMPLEX_H__
#define __MATH_COMPLEX_H__

/*
===============================================================================

  Complex number

===============================================================================
*/

class hfComplex {
public:
	float				r;		// real part
	float				i;		// imaginary part

						hfComplex();
						hfComplex( const float r, const float i );

	void 				Set( const float r, const float i );
	void				Zero();

	float				operator[]( int index ) const;
	float &				operator[]( int index );

	hfComplex			operator-() const;
	hfComplex &			operator=( const hfComplex &a );

	hfComplex			operator*( const hfComplex &a ) const;
	hfComplex			operator/( const hfComplex &a ) const;
	hfComplex			operator+( const hfComplex &a ) const;
	hfComplex			operator-( const hfComplex &a ) const;

	hfComplex &			operator*=( const hfComplex &a );
	hfComplex &			operator/=( const hfComplex &a );
	hfComplex &			operator+=( const hfComplex &a );
	hfComplex &			operator-=( const hfComplex &a );

	hfComplex			operator*( const float a ) const;
	hfComplex			operator/( const float a ) const;
	hfComplex			operator+( const float a ) const;
	hfComplex			operator-( const float a ) const;

	hfComplex &			operator*=( const float a );
	hfComplex &			operator/=( const float a );
	hfComplex &			operator+=( const float a );
	hfComplex &			operator-=( const float a );

	friend hfComplex	operator*( const float a, const hfComplex &b );
	friend hfComplex	operator/( const float a, const hfComplex &b );
	friend hfComplex	operator+( const float a, const hfComplex &b );
	friend hfComplex	operator-( const float a, const hfComplex &b );

	bool				Compare( const hfComplex &a ) const;						// exact compare, no epsilon
	bool				Compare( const hfComplex &a, const float epsilon ) const;	// compare with epsilon
	bool				operator==(	const hfComplex &a ) const;						// exact compare, no epsilon
	bool				operator!=(	const hfComplex &a ) const;						// exact compare, no epsilon

	hfComplex			Reciprocal() const;
	hfComplex			Sqrt() const;
	float				Abs() const;

	int					GetDimension() const;

	const float *		ToFloatPtr() const;
	float *				ToFloatPtr();
	const char *		ToString( int precision = 2 ) const;
};

extern hfComplex complex_origin;
#define complex_zero complex_origin

HF_INLINE hfComplex::hfComplex() {
}

HF_INLINE hfComplex::hfComplex( const float r, const float i ) {
	this->r = r;
	this->i = i;
}

HF_INLINE void hfComplex::Set( const float r, const float i ) {
	this->r = r;
	this->i = i;
}

HF_INLINE void hfComplex::Zero() {
	r = i = 0.0f;
}

HF_INLINE float hfComplex::operator[]( int index ) const {
	assert( index >= 0 && index < 2 );
	return ( &r )[ index ];
}

HF_INLINE float& hfComplex::operator[]( int index ) {
	assert( index >= 0 && index < 2 );
	return ( &r )[ index ];
}

HF_INLINE hfComplex hfComplex::operator-() const {
	return hfComplex( -r, -i );
}

HF_INLINE hfComplex &hfComplex::operator=( const hfComplex &a ) {
	r = a.r;
	i = a.i;
	return *this;
}

HF_INLINE hfComplex hfComplex::operator*( const hfComplex &a ) const {
	return hfComplex( r * a.r - i * a.i, i * a.r + r * a.i );
}

HF_INLINE hfComplex hfComplex::operator/( const hfComplex &a ) const {
	float s, t;
	if ( hfMath::Fabs( a.r ) >= hfMath::Fabs( a.i ) ) {
		s = a.i / a.r;
		t = 1.0f / ( a.r + s * a.i );
		return hfComplex( ( r + s * i ) * t, ( i - s * r ) * t );
	} else {
		s = a.r / a.i;
		t = 1.0f / ( s * a.r + a.i );
		return hfComplex( ( r * s + i ) * t, ( i * s - r ) * t );
	}
}

HF_INLINE hfComplex hfComplex::operator+( const hfComplex &a ) const {
	return hfComplex( r + a.r, i + a.i );
}

HF_INLINE hfComplex hfComplex::operator-( const hfComplex &a ) const {
	return hfComplex( r - a.r, i - a.i );
}

HF_INLINE hfComplex &hfComplex::operator*=( const hfComplex &a ) {
	*this = hfComplex( r * a.r - i * a.i, i * a.r + r * a.i );
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator/=( const hfComplex &a ) {
	float s, t;
	if ( hfMath::Fabs( a.r ) >= hfMath::Fabs( a.i ) ) {
		s = a.i / a.r;
		t = 1.0f / ( a.r + s * a.i );
		*this = hfComplex( ( r + s * i ) * t, ( i - s * r ) * t );
	} else {
		s = a.r / a.i;
		t = 1.0f / ( s * a.r + a.i );
		*this = hfComplex( ( r * s + i ) * t, ( i * s - r ) * t );
	}
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator+=( const hfComplex &a ) {
	r += a.r;
	i += a.i;
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator-=( const hfComplex &a ) {
	r -= a.r;
	i -= a.i;
	return *this;
}

HF_INLINE hfComplex hfComplex::operator*( const float a ) const {
	return hfComplex( r * a, i * a );
}

HF_INLINE hfComplex hfComplex::operator/( const float a ) const {
	float s = 1.0f / a;
	return hfComplex( r * s, i * s );
}

HF_INLINE hfComplex hfComplex::operator+( const float a ) const {
	return hfComplex( r + a, i );
}

HF_INLINE hfComplex hfComplex::operator-( const float a ) const {
	return hfComplex( r - a, i );
}

HF_INLINE hfComplex &hfComplex::operator*=( const float a ) {
	r *= a;
	i *= a;
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator/=( const float a ) {
	float s = 1.0f / a;
	r *= s;
	i *= s;
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator+=( const float a ) {
	r += a;
	return *this;
}

HF_INLINE hfComplex &hfComplex::operator-=( const float a ) {
	r -= a;
	return *this;
}

HF_INLINE hfComplex operator*( const float a, const hfComplex &b ) {
	return hfComplex( a * b.r, a * b.i );
}

HF_INLINE hfComplex operator/( const float a, const hfComplex &b ) {
	float s, t;
	if ( hfMath::Fabs( b.r ) >= hfMath::Fabs( b.i ) ) {
		s = b.i / b.r;
		t = a / ( b.r + s * b.i );
		return hfComplex( t, - s * t );
	} else {
		s = b.r / b.i;
		t = a / ( s * b.r + b.i );
		return hfComplex( s * t, - t );
	}
}

HF_INLINE hfComplex operator+( const float a, const hfComplex &b ) {
	return hfComplex( a + b.r, b.i );
}

HF_INLINE hfComplex operator-( const float a, const hfComplex &b ) {
	return hfComplex( a - b.r, -b.i );
}

HF_INLINE hfComplex hfComplex::Reciprocal() const {
	float s, t;
	if ( hfMath::Fabs( r ) >= hfMath::Fabs( i ) ) {
		s = i / r;
		t = 1.0f / ( r + s * i );
		return hfComplex( t, - s * t );
	} else {
		s = r / i;
		t = 1.0f / ( s * r + i );
		return hfComplex( s * t, - t );
	}
}

HF_INLINE hfComplex hfComplex::Sqrt() const {
	float x, y, w;

	if ( r == 0.0f && i == 0.0f ) {
		return hfComplex( 0.0f, 0.0f );
	}
	x = hfMath::Fabs( r );
	y = hfMath::Fabs( i );
	if ( x >= y ) {
		w = y / x;
		w = hfMath::Sqrt( x ) * hfMath::Sqrt( 0.5f * ( 1.0f + hfMath::Sqrt( 1.0f + w * w ) ) );
	} else {
		w = x / y;
		w = hfMath::Sqrt( y ) * hfMath::Sqrt( 0.5f * ( w + hfMath::Sqrt( 1.0f + w * w ) ) );
	}
	if ( w == 0.0f ) {
		return hfComplex( 0.0f, 0.0f );
	}
	if ( r >= 0.0f ) {
		return hfComplex( w, 0.5f * i / w );
	} else {
		return hfComplex( 0.5f * y / w, ( i >= 0.0f ) ? w : -w );
	}
}

HF_INLINE float hfComplex::Abs() const {
	float x, y, t;
	x = hfMath::Fabs( r );
	y = hfMath::Fabs( i );
	if ( x == 0.0f ) {
		return y;
	} else if ( y == 0.0f ) {
		return x;
	} else if ( x > y ) {
		t = y / x;
		return x * hfMath::Sqrt( 1.0f + t * t );
	} else {
		t = x / y;
		return y * hfMath::Sqrt( 1.0f + t * t );
	}
}

HF_INLINE bool hfComplex::Compare( const hfComplex &a ) const {
	return ( ( r == a.r ) && ( i == a.i ) );
}

HF_INLINE bool hfComplex::Compare( const hfComplex &a, const float epsilon ) const {
	if ( hfMath::Fabs( r - a.r ) > epsilon ) {
		return false;
	}
	if ( hfMath::Fabs( i - a.i ) > epsilon ) {
		return false;
	}
	return true;
}

HF_INLINE bool hfComplex::operator==( const hfComplex &a ) const {
	return Compare( a );
}

HF_INLINE bool hfComplex::operator!=( const hfComplex &a ) const {
	return !Compare( a );
}

HF_INLINE int hfComplex::GetDimension() const {
	return 2;
}

HF_INLINE const float *hfComplex::ToFloatPtr() const {
	return &r;
}

HF_INLINE float *hfComplex::ToFloatPtr() {
	return &r;
}

#endif /* !__MATH_COMPLEX_H__ */

