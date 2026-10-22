#ifndef __PLANESET_H__
#define __PLANESET_H__

/*
===============================================================================

	Plane Set

===============================================================================
*/

class hfPlaneSet : public hfList<hfPlane> {
public:

	void					Clear() { hfList<hfPlane>::Clear(); hash.Free(); }

	int						FindPlane( const hfPlane &plane, const float normalEps, const float distEps );

private:
	hfHashIndex				hash;
};

HF_INLINE int hfPlaneSet::FindPlane( const hfPlane &plane, const float normalEps, const float distEps ) {
	int i, border, hashKey;

	assert( distEps <= 0.125f );

	hashKey = (int)( hfMath::Fabs( plane.Dist() ) * 0.125f );
	for ( border = -1; border <= 1; border++ ) {
		for ( i = hash.First( hashKey + border ); i >= 0; i = hash.Next( i ) ) {
			if ( (*this)[i].Compare( plane, normalEps, distEps ) ) {
				return i;
			}
		}
	}

	if ( plane.Type() >= PLANETYPE_NEGX && plane.Type() < PLANETYPE_TRUEAXIAL ) {
		Append( -plane );
		hash.Add( hashKey, Num()-1 );
		Append( plane );
		hash.Add( hashKey, Num()-1 );
		return ( Num() - 1 );
	}
	else {
		Append( plane );
		hash.Add( hashKey, Num()-1 );
		Append( -plane );
		hash.Add( hashKey, Num()-1 );
		return ( Num() - 2 );
	}
}

#endif /* !__PLANESET_H__ */

