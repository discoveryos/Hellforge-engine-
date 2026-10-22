#ifndef __SURFACE_SWEPTSPLINE_H__
#define __SURFACE_SWEPTSPLINE_H__

/*
===============================================================================

	Swept Spline surface.

===============================================================================
*/

class hfSurface_SweptSpline : public hfSurface {
public:
							hfSurface_SweptSpline();
							~hfSurface_SweptSpline();

	void					SetSpline( hfCurve_Spline<hfVec4> *spline );
	void					SetSweptSpline( hfCurve_Spline<hfVec4> *sweptSpline );
	void					SetSweptCircle( const float radius );

	void					Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions );

	void					Clear();

protected:
	hfCurve_Spline<hfVec4> *spline;
	hfCurve_Spline<hfVec4> *sweptSpline;

	void					GetFrame( const hfMat3 &previousFrame, const hfVec3 dir, hfMat3 &newFrame );
};

/*
====================
hfSurface_SweptSpline::hfSurface_SweptSpline
====================
*/
HF_INLINE hfSurface_SweptSpline::hfSurface_SweptSpline() {
	spline = NULL;
	sweptSpline = NULL;
}

/*
====================
hfSurface_SweptSpline::~hfSurface_SweptSpline
====================
*/
HF_INLINE hfSurface_SweptSpline::~hfSurface_SweptSpline() {
	delete spline;
	delete sweptSpline;
}

/*
====================
hfSurface_SweptSpline::Clear
====================
*/
HF_INLINE void hfSurface_SweptSpline::Clear() {
	hfSurface::Clear();
	delete spline;
	spline = NULL;
	delete sweptSpline;
	sweptSpline = NULL;
}

#endif /* !__SURFACE_SWEPTSPLINE_H__ */

