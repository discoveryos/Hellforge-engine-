#ifndef __SWAP_H__
#define __SWAP_H__

/*
================================================================================================
Contains the Swap class, for CrossPlatform endian conversion.

works 
================================================================================================
*/

/*
========================
IsPointer
========================
*/
template< typename type >
bool IsPointer( type ) {
	return false;
}

/*
========================
IsPointer
========================
*/
template< typename type >
bool IsPointer( type * ) {
	return true;
}

/*
================================================
The *Swap* static template class, hfSwap, is used by the SwapClass template class for 
performing EndianSwapping.
================================================
*/
class hfSwap {
public:
	//#define SwapBytes( x, y )		(x) ^= (y) ^= (x) ^= (y)
	#define SwapBytes( x, y )		{ byte t = (x); (x) = (y); (y) = t; }

	template<class type> static void Little( type &c ) {
		// byte swapping pointers is pointless because we should never store pointers on disk
		assert( !IsPointer( c ) );

	}

	template<class type> static void Big( type &c ) {
		// byte swapping pointers is pointless because we should never store pointers on disk
		assert( !IsPointer( c ) );

			if ( sizeof( type ) == 1 ) {
			} else if ( sizeof( type ) == 2 ) {
				byte *b = (byte *)&c;
				SwapBytes( b[0], b[1] );
			} else if ( sizeof( type ) == 4 ) {
				byte *b = (byte *)&c;
				SwapBytes( b[0], b[3] );
				SwapBytes( b[1], b[2] );
			} else if ( sizeof( type ) == 8 ) {
				byte * b = (byte *)&c;
				SwapBytes( b[0], b[7] );
				SwapBytes( b[1], b[6]);
				SwapBytes( b[2], b[5] );
				SwapBytes( b[3], b[4] );
			} else {
				assert( false );
			}
	}

	template<class type> static void LittleArray( type *c, int count ) {
	}

	template<class type> static void BigArray( type *c, int count ) {
		for ( int i = 0; i < count; i++ ) {
			Big( c[i] );
		}
	}

	static void SixtetsForInt( byte *out, int src ) {
			byte *b = (byte *)&src;
			out[0] = ( b[0] & 0xfc ) >> 2;
			out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
			out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
			out[3] = b[2] & 0x3f;
	}

	static int IntForSixtets( byte *in ) {
			int ret = 0;
			byte *b = (byte *)&ret;
			b[0] |= in[0] << 2;
			b[0] |= ( in[1] & 0x30 ) >> 4;
			b[1] |= ( in[1] & 0xf ) << 4;
			b[1] |= ( in[2] & 0x3c ) >> 2;
			b[2] |= ( in[2] & 0x3 ) << 6;
			b[2] |= in[3];
			return ret;
	}

public:		// specializations
#ifndef HF_SWAP_LITE // avoid dependency avalanche for SPU code
#define SWAP_VECTOR( x ) \
	static void Little( x &c ) { LittleArray( c.ToFloatPtr(), c.GetDimension() ); } \
	static void Big( x &c ) {    BigArray( c.ToFloatPtr(), c.GetDimension() ); }

	SWAP_VECTOR( hfVec2 );
	SWAP_VECTOR( hfVec3 );
	SWAP_VECTOR( hfVec4 );
	SWAP_VECTOR( hfVec5 );
	SWAP_VECTOR( hfVec6 );
	SWAP_VECTOR( hfMat2 );
	SWAP_VECTOR( hfMat3 );
	SWAP_VECTOR( hfMat4 );
	SWAP_VECTOR( hfMat5 );
	SWAP_VECTOR( hfMat6 );
	SWAP_VECTOR( hfPlane );
	SWAP_VECTOR( hfQuat );
	SWAP_VECTOR( hfCQuat );
	SWAP_VECTOR( hfAngles );
	SWAP_VECTOR( hfBounds );

	static void Little( hfDrawVert &v ) {
			Little( v.xyz );
			LittleArray( v.st, 2 );
			LittleArray( v.normal, 4 );
			LittleArray( v.tangent, 4 );
			LittleArray( v.color, 4 );
	}
	static void Big( hfDrawVert &v ) {
			Big( v.xyz );
			BigArray( v.st, 2 );
			BigArray( v.normal, 4 );
			BigArray( v.tangent, 4 );
			BigArray( v.color, 4 );
	}
#endif
};

/*
================================================
hfSwapClass is a template class for performing EndianSwapping.
================================================
*/
template<class classType>
class hfSwapClass {
public:
	hfSwapClass() {
		#ifdef _DEBUG
			size = 0;
		#endif
	}
	~hfSwapClass() {
		#ifdef _DEBUG
			assert( size == sizeof( classType ) );
		#endif
	}

	template<class type> void Little( type &c ) {
		hfSwap::Little( c );
		#ifdef _DEBUG
			size += sizeof( type );
		#endif
	}

	template<class type> void Big( type &c ) {
		hfSwap::Big( c );
		#ifdef _DEBUG
			size += sizeof( type );
		#endif
	}

	template<class type> void LittleArray( type *c, int count ) {
		hfSwap::LittleArray( c, count );
		#ifdef _DEBUG
			size += count * sizeof( type );
		#endif
	}

	template<class type> void BigArray( type *c, int count ) {
		hfSwap::BigArray( c, count );
		#ifdef _DEBUG
			size += count * sizeof( type );
		#endif
	}

#ifdef _DEBUG
private:
	int size;
#endif
};

	#define BIG32(v) ((((uint32)(v)) >> 24) | (((uint32)(v) & 0x00FF0000) >> 8) | (((uint32)(v) & 0x0000FF00) << 8) | ((uint32)(v) << 24))
	#define BIG16(v) ((((uint16)(v)) >> 8) | ((uint16)(v) << 8))

#endif // !__SWAP_H__

