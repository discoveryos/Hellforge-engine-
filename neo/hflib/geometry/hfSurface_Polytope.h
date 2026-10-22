#ifndef __SURFACE_POLYTOPE_H__
#define __SURFACE_POLYTOPE_H__

/*
===============================================================================

	Polytope surface.

	NOTE: vertexes are not duplicated for texture coordinates.

===============================================================================
*/

class hfSurface_Polytope : public hfSurface {
public:
						hfSurface_Polytope();
						explicit hfSurface_Polytope( const hfSurface &surface ) : hfSurface( surface ) {}

	void				FromPlanes( const hfPlane *planes, const int numPlanes );

	void				SetupTetrahedron( const hfBounds &bounds );
	void				SetupHexahedron( const hfBounds &bounds );
	void				SetupOctahedron( const hfBounds &bounds );
	void				SetupDodecahedron( const hfBounds &bounds );
	void				SetupIcosahedron( const hfBounds &bounds );
	void				SetupCylinder( const hfBounds &bounds, const int numSides );
	void				SetupCone( const hfBounds &bounds, const int numSides );

	int					SplitPolytope( const hfPlane &plane, const float epsilon, hfSurface_Polytope **front, hfSurface_Polytope **back ) const;

protected:

};

/*
====================
hfSurface_Polytope::hfSurface_Polytope
====================
*/
HF_INLINE hfSurface_Polytope::hfSurface_Polytope() {
}

#endif /* !__SURFACE_POLYTOPE_H__ */

