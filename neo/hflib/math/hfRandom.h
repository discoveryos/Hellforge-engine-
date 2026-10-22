#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

/*
===============================================================================

	Random number generator

===============================================================================
*/

class hfRandom {
public:
						hfRandom( int seed = 0 );

	void				SetSeed( int seed );
	int					GetSeed() const;

	int					RandomInt();			// random integer in the range [0, MAX_RAND]
	int					RandomInt( int max );		// random integer in the range [0, max[
	float				RandomFloat();		// random number in the range [0.0f, 1.0f]
	float				CRandomFloat();		// random number in the range [-1.0f, 1.0f]

	static const int	MAX_RAND = 0x7fff;

private:
	int					seed;
};

HF_INLINE hfRandom::hfRandom( int seed ) {
	this->seed = seed;
}

HF_INLINE void hfRandom::SetSeed( int seed ) {
	this->seed = seed;
}

HF_INLINE int hfRandom::GetSeed() const {
	return seed;
}

HF_INLINE int hfRandom::RandomInt() {
	seed = 69069 * seed + 1;
	return ( seed & hfRandom::MAX_RAND );
}

HF_INLINE int hfRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

HF_INLINE float hfRandom::RandomFloat() {
	return ( RandomInt() / ( float )( hfRandom::MAX_RAND + 1 ) );
}

HF_INLINE float hfRandom::CRandomFloat() {
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
}


/*
===============================================================================

	Random number generator

===============================================================================
*/

class hfRandom2 {
public:
							hfRandom2( unsigned long seed = 0 );

	void					SetSeed( unsigned long seed );
	unsigned long			GetSeed() const;

	int						RandomInt();			// random integer in the range [0, MAX_RAND]
	int						RandomInt( int max );		// random integer in the range [0, max]
	float					RandomFloat();		// random number in the range [0.0f, 1.0f]
	float					CRandomFloat();		// random number in the range [-1.0f, 1.0f]

	static const int		MAX_RAND = 0x7fff;

private:
	unsigned long			seed;

	static const unsigned long	IEEE_ONE = 0x3f800000;
	static const unsigned long	IEEE_MASK = 0x007fffff;
};

HF_INLINE hfRandom2::hfRandom2( unsigned long seed ) {
	this->seed = seed;
}

HF_INLINE void hfRandom2::SetSeed( unsigned long seed ) {
	this->seed = seed;
}

HF_INLINE unsigned long hfRandom2::GetSeed() const {
	return seed;
}

HF_INLINE int hfRandom2::RandomInt() {
	seed = 1664525L * seed + 1013904223L;
	return ( (int) seed & hfRandom2::MAX_RAND );
}

HF_INLINE int hfRandom2::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - hfMath::BitsForInteger( max ) ) ) % max;
}

HF_INLINE float hfRandom2::RandomFloat() {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = hfRandom2::IEEE_ONE | ( seed & hfRandom2::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

HF_INLINE float hfRandom2::CRandomFloat() {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = hfRandom2::IEEE_ONE | ( seed & hfRandom2::IEEE_MASK );
	return ( 2.0f * ( *(float *)&i ) - 3.0f );
}

#endif /* !__MATH_RANDOM_H__ */

