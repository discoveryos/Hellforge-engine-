#pragma hdrstop
#include "../hfprecompiled.h"

/*
====================
hfSurface_SweptSpline::SetSpline
====================
*/
void hfSurface_SweptSpline::SetSpline( hfCurve_Spline<hfVec4> *spline ) {
	if ( this->spline ) {
		delete this->spline;
	}
	this->spline = spline;
}

/*
====================
hfSurface_SweptSpline::SetSweptSpline
====================
*/
void hfSurface_SweptSpline::SetSweptSpline( hfCurve_Spline<hfVec4> *sweptSpline ) {
	if ( this->sweptSpline ) {
		delete this->sweptSpline;
	}
	this->sweptSpline = sweptSpline;
}

/*
====================
hfSurface_SweptSpline::SetSweptCircle

  Sets the swept spline to a NURBS circle.
====================
*/
void hfSurface_SweptSpline::SetSweptCircle( const float radius ) {
	hfCurve_NURBS<hfVec4> *nurbs = new (TAG_IDLIB_SURFACE) hfCurve_NURBS<hfVec4>();
	nurbs->Clear();
	nurbs->AddValue(   0.0f, hfVec4(  radius,  radius, 0.0f, 0.00f ) );
	nurbs->AddValue( 100.0f, hfVec4( -radius,  radius, 0.0f, 0.25f ) );
	nurbs->AddValue( 200.0f, hfVec4( -radius, -radius, 0.0f, 0.50f ) );
	nurbs->AddValue( 300.0f, hfVec4(  radius, -radius, 0.0f, 0.75f ) );
	nurbs->SetBoundaryType( hfCurve_NURBS<hfVec4>::BT_CLOSED );
	nurbs->SetCloseTime( 100.0f );
	if ( sweptSpline ) {
		delete sweptSpline;
	}
	sweptSpline = nurbs;
}

/*
====================
hfSurface_SweptSpline::GetFrame
====================
*/
void hfSurface_SweptSpline::GetFrame( const hfMat3 &previousFrame, const hfVec3 dir, hfMat3 &newFrame ) {
	float wx, wy, wz;
	float xx, yy, yz;
	float xy, xz, zz;
	float x2, y2, z2;
	float a, c, s, x, y, z;
	hfVec3 d, v;
	hfMat3 axis;

	d = dir;
	d.Normalize();
	v = d.Cross( previousFrame[2] );
	v.Normalize();

	a = hfMath::ACos( previousFrame[2] * d ) * 0.5f;
	c = hfMath::Cos( a );
	s = hfMath::Sqrt( 1.0f - c * c );

	x = v[0] * s;
	y = v[1] * s;
	z = v[2] * s;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;
	xx = x * x2;
	xy = x * y2;
	xz = x * z2;
	yy = y * y2;
	yz = y * z2;
	zz = z * z2;
	wx = c * x2;
	wy = c * y2;
	wz = c * z2;

	axis[0][0] = 1.0f - ( yy + zz );
	axis[0][1] = xy - wz;
	axis[0][2] = xz + wy;
	axis[1][0] = xy + wz;
	axis[1][1] = 1.0f - ( xx + zz );
	axis[1][2] = yz - wx;
	axis[2][0] = xz - wy;
	axis[2][1] = yz + wx;
	axis[2][2] = 1.0f - ( xx + yy );

	newFrame = previousFrame * axis;

	newFrame[2] = dir;
	newFrame[2].Normalize();
	newFrame[1].Cross( newFrame[ 2 ], newFrame[ 0 ] );
	newFrame[1].Normalize();
	newFrame[0].Cross( newFrame[ 1 ], newFrame[ 2 ] );
	newFrame[0].Normalize();
}

/*
====================
hfSurface_SweptSpline::Tessellate

  tesselate the surface
====================
*/
void hfSurface_SweptSpline::Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions ) {
	int i, j, offset, baseOffset, splineDiv, sweptSplineDiv;
	int i0, i1, j0, j1;
	float totalTime, t;
	hfVec4 splinePos, splineD1;
	hfMat3 splineMat;

	if ( !spline || !sweptSpline ) {
		hfSurface::Clear();
		return;
	}

	verts.SetNum( splineSubdivisions * sweptSplineSubdivisions );

	// calculate the points and first derivatives for the swept spline
	totalTime = sweptSpline->GetTime( sweptSpline->GetNumValues() - 1 ) - sweptSpline->GetTime( 0 ) + sweptSpline->GetCloseTime();
	sweptSplineDiv = sweptSpline->GetBoundaryType() == hfCurve_Spline<hfVec3>::BT_CLOSED ? sweptSplineSubdivisions : sweptSplineSubdivisions - 1;
	baseOffset = (splineSubdivisions-1) * sweptSplineSubdivisions;
	for ( i = 0; i < sweptSplineSubdivisions; i++ ) {
		t = totalTime * i / sweptSplineDiv;
		splinePos = sweptSpline->GetCurrentValue( t );
		splineD1 = sweptSpline->GetCurrentFirstDerivative( t );
		verts[baseOffset+i].xyz = splinePos.ToVec3();
		verts[baseOffset+i].SetTexCoordS( splinePos.w );
		verts[baseOffset+i].SetTangent( splineD1.ToVec3() );
	}

	// sweep the spline
	totalTime = spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 ) + spline->GetCloseTime();
	splineDiv = spline->GetBoundaryType() == hfCurve_Spline<hfVec3>::BT_CLOSED ? splineSubdivisions : splineSubdivisions - 1;
	splineMat.Identity();
	hfVec3 tempNormal;
	for ( i = 0; i < splineSubdivisions; i++ ) {
		t = totalTime * i / splineDiv;

		splinePos = spline->GetCurrentValue( t );
		splineD1 = spline->GetCurrentFirstDerivative( t );

		GetFrame( splineMat, splineD1.ToVec3(), splineMat );

		offset = i * sweptSplineSubdivisions;
		for ( j = 0; j < sweptSplineSubdivisions; j++ ) {
			hfDrawVert *v = &verts[offset+j];
			v->xyz = splinePos.ToVec3() + verts[baseOffset+j].xyz * splineMat;
			v->SetTexCoord( verts[baseOffset+j].GetTexCoord().x, splinePos.w );
			v->SetTangent( verts[baseOffset+j].GetTangent() * splineMat );
			v->SetBiTangent( splineD1.ToVec3() );
			tempNormal = v->GetBiTangent().Cross( v->GetTangent() );
			tempNormal.Normalize();
			v->SetNormal( tempNormal );
			v->color[0] = v->color[1] = v->color[2] = v->color[3] = 0;
		}
	}

	indexes.SetNum( splineDiv * sweptSplineDiv * 2 * 3 );

	// create indexes for the triangles
	for ( offset = i = 0; i < splineDiv; i++ ) {

		i0 = (i+0) * sweptSplineSubdivisions;
		i1 = (i+1) % splineSubdivisions * sweptSplineSubdivisions;

		for ( j = 0; j < sweptSplineDiv; j++ ) {

			j0 = (j+0);
			j1 = (j+1) % sweptSplineSubdivisions;

			indexes[offset++] = i0 + j0;
			indexes[offset++] = i0 + j1;
			indexes[offset++] = i1 + j1;

			indexes[offset++] = i1 + j1;
			indexes[offset++] = i1 + j0;
			indexes[offset++] = i0 + j0;
		}
	}

	GenerateEdgeIndexes();
}

