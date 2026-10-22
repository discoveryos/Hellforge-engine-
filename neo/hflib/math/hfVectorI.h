#ifndef __MATH_VECTORI_H__
#define __MATH_VECTORI_H__

static HF_INLINE int MinInt( int a, int b ) { return (a) < (b) ? (a) : (b); }
static HF_INLINE int MaxInt( int a, int b ) { return (a) < (b) ? (b) : (a); }

class hfVec2i {
public:
    int      x, y;

	hfVec2i() {}
	hfVec2i( int _x, int _y ) : x(_x), y(_y ) {}

	void		Set( int _x, int _y ) { x = _x; y = _y; }
	int			Area() const { return x * y; };

	void		Min( hfVec2i &v ) { x = MinInt( x, v.x ); y = MinInt( y, v.y ); }
	void		Max( hfVec2i &v ) { x = MaxInt( x, v.x ); y = MaxInt( y, v.y ); }

	int			operator[]( const int index ) const { assert( index == 0 || index == 1 ); return (&x)[index]; }
	int &		operator[]( const int index ) { assert( index == 0 || index == 1 ); return (&x)[index]; }

	hfVec2i 	operator-() const { return hfVec2i( -x, -y ); }
	hfVec2i 	operator!() const { return hfVec2i( !x, !y ); }

	hfVec2i 	operator>>( const int a ) const { return hfVec2i( x >> a, y >> a ); }
	hfVec2i 	operator<<( const int a ) const { return hfVec2i( x << a, y << a ); }
	hfVec2i 	operator&( const int a ) const { return hfVec2i( x & a, y & a ); }
	hfVec2i 	operator|( const int a ) const { return hfVec2i( x | a, y | a ); }
	hfVec2i 	operator^( const int a ) const { return hfVec2i( x ^ a, y ^ a ); }
	hfVec2i 	operator*( const int a ) const { return hfVec2i( x * a, y * a ); }
	hfVec2i 	operator/( const int a ) const { return hfVec2i( x / a, y / a ); }
	hfVec2i 	operator+( const int a ) const { return hfVec2i( x + a, y + a ); }
	hfVec2i 	operator-( const int a ) const { return hfVec2i( x - a, y - a ); }

	bool		operator==( const hfVec2i &a ) const { return a.x == x && a.y == y; };
	bool		operator!=( const hfVec2i &a ) const { return a.x != x || a.y != y; };

	hfVec2i		operator>>( const hfVec2i &a ) const { return hfVec2i( x >> a.x, y >> a.y ); }
	hfVec2i		operator<<( const hfVec2i &a ) const { return hfVec2i( x << a.x, y << a.y ); }
	hfVec2i		operator&( const hfVec2i &a ) const { return hfVec2i( x & a.x, y & a.y ); }
	hfVec2i		operator|( const hfVec2i &a ) const { return hfVec2i( x | a.x, y | a.y ); }
	hfVec2i		operator^( const hfVec2i &a ) const { return hfVec2i( x ^ a.x, y ^ a.y ); }
	hfVec2i		operator*( const hfVec2i &a ) const { return hfVec2i( x * a.x, y * a.y ); }
	hfVec2i		operator/( const hfVec2i &a ) const { return hfVec2i( x / a.x, y / a.y ); }
	hfVec2i		operator+( const hfVec2i &a ) const { return hfVec2i( x + a.x, y + a.y ); }
	hfVec2i		operator-( const hfVec2i &a ) const { return hfVec2i( x - a.x, y - a.y ); }

	hfVec2i &	operator+=( const int a ) { x += a; y += a; return *this; }
	hfVec2i &	operator-=( const int a ) { x -= a; y -= a; return *this; }
	hfVec2i &	operator/=( const int a ) { x /= a; y /= a; return *this; }
	hfVec2i &	operator*=( const int a ) { x *= a; y *= a; return *this; }
	hfVec2i &	operator>>=( const int a ) { x >>= a; y >>= a; return *this; }
	hfVec2i &	operator<<=( const int a ) { x <<= a; y <<= a; return *this; }
	hfVec2i &	operator&=( const int a ) { x &= a; y &= a; return *this; }
	hfVec2i &	operator|=( const int a ) { x |= a; y |= a; return *this; }
	hfVec2i &	operator^=( const int a ) { x ^= a; y ^= a; return *this; }

	hfVec2i &	operator>>=( const hfVec2i &a ) { x >>= a.x; y >>= a.y; return *this; }
	hfVec2i &	operator<<=( const hfVec2i &a ) { x <<= a.x; y <<= a.y; return *this; }
	hfVec2i &	operator&=( const hfVec2i &a ) { x &= a.x; y &= a.y; return *this; }
	hfVec2i &	operator|=( const hfVec2i &a ) { x |= a.x; y |= a.y; return *this; }
	hfVec2i &	operator^=( const hfVec2i &a ) { x ^= a.x; y ^= a.y; return *this; }
	hfVec2i &	operator+=( const hfVec2i &a ) { x += a.x; y += a.y; return *this; }
	hfVec2i &	operator-=( const hfVec2i &a ) { x -= a.x; y -= a.y; return *this; }
	hfVec2i &	operator/=( const hfVec2i &a ) { x /= a.x; y /= a.y; return *this; }
	hfVec2i &	operator*=( const hfVec2i &a ) { x *= a.x; y *= a.y; return *this; }
};

#endif

