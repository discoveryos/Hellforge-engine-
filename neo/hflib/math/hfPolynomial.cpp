#pragma hdrstop
#include "../hfprecompiled.h"

const float EPSILON		= 1e-6f;

/*
=============
hfPolynomial::Laguer
=============
*/
int hfPolynomial::Laguer( const hfComplex *coef, const int degree, hfComplex &x ) const {
	const int MT = 10, MAX_ITERATIONS = MT * 8;
	static const float frac[] = { 0.0f, 0.5f, 0.25f, 0.75f, 0.13f, 0.38f, 0.62f, 0.88f, 1.0f };
	int i, j;
	float abx, abp, abm, err;
	hfComplex dx, cx, b, d, f, g, s, gps, gms, g2;

	for ( i = 1; i <= MAX_ITERATIONS; i++ ) {
		b = coef[degree];
		err = b.Abs();
		d.Zero();
		f.Zero();
		abx = x.Abs();
		for ( j = degree - 1; j >= 0; j-- ) {
			f = x * f + d;
			d = x * d + b;
			b = x * b + coef[j];
			err = b.Abs() + abx * err;
		}
		if ( b.Abs() < err * EPSILON ) {
			return i;
		}
		g = d / b;
		g2 = g * g;
		s = ( ( degree - 1 ) * ( degree * ( g2 - 2.0f * f / b ) - g2 ) ).Sqrt();
		gps = g + s;
		gms = g - s;
		abp = gps.Abs();
		abm = gms.Abs();
		if ( abp < abm ) {
			gps = gms;
		}
		if ( Max( abp, abm ) > 0.0f ) {
			dx = degree / gps;
		} else {
			dx = hfMath::Exp( hfMath::Log( 1.0f + abx ) ) * hfComplex( hfMath::Cos( i ), hfMath::Sin( i ) );
		}
		cx = x - dx;
		if ( x == cx ) {
			return i;
		}
		if ( i % MT == 0 ) {
			x = cx;
		} else {
			x -= frac[i/MT] * dx;
		}
	}
	return i;
}

/*
=============
hfPolynomial::GetRoots
=============
*/
int hfPolynomial::GetRoots( hfComplex *roots ) const {
	int i, j;
	hfComplex x, b, c, *coef;

	coef = (hfComplex *) _alloca16( ( degree + 1 ) * sizeof( hfComplex ) );
	for ( i = 0; i <= degree; i++ ) {
		coef[i].Set( coefficient[i], 0.0f );
	}

	for ( i = degree - 1; i >= 0; i-- ) {
		x.Zero();
		Laguer( coef, i + 1, x );
		if ( hfMath::Fabs( x.i ) < 2.0f * EPSILON * hfMath::Fabs( x.r ) ) {
			x.i = 0.0f;
		}
		roots[i] = x;
		b = coef[i+1];
		for ( j = i; j >= 0; j-- ) {
			c = coef[j];
			coef[j] = b;
			b = x * b + c;
		}
	}

	for ( i = 0; i <= degree; i++ ) {
		coef[i].Set( coefficient[i], 0.0f );
	}
	for ( i = 0; i < degree; i++ ) {
		Laguer( coef, degree, roots[i] );
	}

	for ( i = 1; i < degree; i++ ) {
		x = roots[i];
		for ( j = i - 1; j >= 0; j-- ) {
			if ( roots[j].r <= x.r ) {
				break;
			}
			roots[j+1] = roots[j];
		}
		roots[j+1] = x;
	}

	return degree;
}

/*
=============
hfPolynomial::GetRoots
=============
*/
int hfPolynomial::GetRoots( float *roots ) const {
	int i, num;
	hfComplex *complexRoots;

	switch( degree ) {
		case 0: return 0;
		case 1: return GetRoots1( coefficient[1], coefficient[0], roots );
		case 2: return GetRoots2( coefficient[2], coefficient[1], coefficient[0], roots );
		case 3: return GetRoots3( coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
		case 4: return GetRoots4( coefficient[4], coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
	}

	// The Abel-Ruffini theorem states that there is no general solution
	// in radicals to polynomial equations of degree five or higher.
	// A polynomial equation can be solved by radicals if and only if
	// its Galois group is a solvable group.

	complexRoots = (hfComplex *) _alloca16( degree * sizeof( hfComplex ) );

	GetRoots( complexRoots );

	for ( num = i = 0; i < degree; i++ ) {
		if ( complexRoots[i].i == 0.0f ) {
			roots[i] = complexRoots[i].r;
			num++;
		}
	}
	return num;
}

/*
=============
hfPolynomial::ToString
=============
*/
const char *hfPolynomial::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
hfPolynomial::Test
=============
*/
void hfPolynomial::Test() {
	int i, num;
	float roots[4], value;
	hfComplex complexRoots[4], complexValue;
	hfPolynomial p;

	p = hfPolynomial( -5.0f, 4.0f );
	num = p.GetRoots( roots );
	for ( i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( hfMath::Fabs( value ) < 1e-4f );
	}

	p = hfPolynomial( -5.0f, 4.0f, 3.0f );
	num = p.GetRoots( roots );
	for ( i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( hfMath::Fabs( value ) < 1e-4f );
	}

	p = hfPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( roots );
	for ( i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( hfMath::Fabs( value ) < 1e-4f );
	}

	p = hfPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( roots );
	for ( i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( hfMath::Fabs( value ) < 1e-4f );
	}

	p = hfPolynomial( -5.0f, 4.0f, 3.0f, 2.0f, 1.0f );
	num = p.GetRoots( roots );
	for ( i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( hfMath::Fabs( value ) < 1e-4f );
	}

	p = hfPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( complexRoots );
	for ( i = 0; i < num; i++ ) {
		complexValue = p.GetValue( complexRoots[i] );
		assert( hfMath::Fabs( complexValue.r ) < 1e-4f && hfMath::Fabs( complexValue.i ) < 1e-4f );
	}

	p = hfPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( complexRoots );
	for ( i = 0; i < num; i++ ) {
		complexValue = p.GetValue( complexRoots[i] );
		assert( hfMath::Fabs( complexValue.r ) < 1e-4f && hfMath::Fabs( complexValue.i ) < 1e-4f );
	}
}

