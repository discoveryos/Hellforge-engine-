#pragma hdrstop
#include "../hfprecompiled.h"

#define POLYTOPE_VERTEX_EPSILON		0.1f

/*
====================
hfSurface_Polytope::FromPlanes
====================
*/
void hfSurface_Polytope::FromPlanes( const hfPlane *planes, const int numPlanes ) {
	int i, j, k, *windingVerts;
	hfFixedWinding w;
	hfDrawVert newVert;

	windingVerts = (int *) _alloca( MAX_POINTS_ON_WINDING * sizeof( int ) );
	memset( &newVert, 0, sizeof( newVert ) );

	for ( i = 0; i < numPlanes; i++ ) {

		w.BaseForPlane( planes[i] );

		for ( j = 0; j < numPlanes; j++ ) {
			if ( j == i ) {
				continue;
			}
			if ( !w.ClipInPlace( -planes[j], ON_EPSILON, true ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;
		}

		for ( j = 0; j < w.GetNumPoints(); j++ ) {
			for ( k = 0; k < verts.Num(); k++ ) {
				if ( verts[k].xyz.Compare( w[j].ToVec3(), POLYTOPE_VERTEX_EPSILON ) ) {
					break;
				}
			}
			if ( k >= verts.Num() ) {
				newVert.xyz = w[j].ToVec3();
				k = verts.Append( newVert );
			}
			windingVerts[j] = k;
		}

		for ( j = 2; j < w.GetNumPoints(); j++ ) {
			indexes.Append( windingVerts[0] );
			indexes.Append( windingVerts[j-1] );
			indexes.Append( windingVerts[j] );
		}
	}

	GenerateEdgeIndexes();
}

/*
====================
hfSurface_Polytope::SetupTetrahedron
====================
*/
void hfSurface_Polytope::SetupTetrahedron( const hfBounds &bounds ) {
	hfVec3 center, scale;
	float c1, c2, c3;

	c1 = 0.4714045207f;
	c2 = 0.8164965809f;
	c3 = -0.3333333333f;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 4 );
	verts[0].xyz = center + hfVec3( 0.0f, 0.0f, scale.z );
	verts[1].xyz = center + hfVec3( 2.0f * c1 * scale.x, 0.0f, c3 * scale.z );
	verts[2].xyz = center + hfVec3( -c1 * scale.x, c2 * scale.y, c3 * scale.z );
	verts[3].xyz = center + hfVec3( -c1 * scale.x, -c2 * scale.y, c3 * scale.z );

	indexes.SetNum( 4*3 );
	indexes[0*3+0] = 0;
	indexes[0*3+1] = 1;
	indexes[0*3+2] = 2;
	indexes[1*3+0] = 0;
	indexes[1*3+1] = 2;
	indexes[1*3+2] = 3;
	indexes[2*3+0] = 0;
	indexes[2*3+1] = 3;
	indexes[2*3+2] = 1;
	indexes[3*3+0] = 1;
	indexes[3*3+1] = 3;
	indexes[3*3+2] = 2;

	GenerateEdgeIndexes();
}

/*
====================
hfSurface_Polytope::SetupHexahedron
====================
*/
void hfSurface_Polytope::SetupHexahedron( const hfBounds &bounds ) {
	hfVec3 center, scale;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 8 );
	verts[0].xyz = center + hfVec3( -scale.x, -scale.y, -scale.z );
	verts[1].xyz = center + hfVec3(  scale.x, -scale.y, -scale.z );
	verts[2].xyz = center + hfVec3(  scale.x,  scale.y, -scale.z );
	verts[3].xyz = center + hfVec3( -scale.x,  scale.y, -scale.z );
	verts[4].xyz = center + hfVec3( -scale.x, -scale.y,  scale.z );
	verts[5].xyz = center + hfVec3(  scale.x, -scale.y,  scale.z );
	verts[6].xyz = center + hfVec3(  scale.x,  scale.y,  scale.z );
	verts[7].xyz = center + hfVec3( -scale.x,  scale.y,  scale.z );

	indexes.SetNum( 12*3 );
	indexes[ 0*3+0] = 0;
	indexes[ 0*3+1] = 3;
	indexes[ 0*3+2] = 2;
	indexes[ 1*3+0] = 0;
	indexes[ 1*3+1] = 2;
	indexes[ 1*3+2] = 1;
	indexes[ 2*3+0] = 0;
	indexes[ 2*3+1] = 1;
	indexes[ 2*3+2] = 5;
	indexes[ 3*3+0] = 0;
	indexes[ 3*3+1] = 5;
	indexes[ 3*3+2] = 4;
	indexes[ 4*3+0] = 0;
	indexes[ 4*3+1] = 4;
	indexes[ 4*3+2] = 7;
	indexes[ 5*3+0] = 0;
	indexes[ 5*3+1] = 7;
	indexes[ 5*3+2] = 3;
	indexes[ 6*3+0] = 6;
	indexes[ 6*3+1] = 5;
	indexes[ 6*3+2] = 1;
	indexes[ 7*3+0] = 6;
	indexes[ 7*3+1] = 1;
	indexes[ 7*3+2] = 2;
	indexes[ 8*3+0] = 6;
	indexes[ 8*3+1] = 2;
	indexes[ 8*3+2] = 3;
	indexes[ 9*3+0] = 6;
	indexes[ 9*3+1] = 3;
	indexes[ 9*3+2] = 7;
	indexes[10*3+0] = 6;
	indexes[10*3+1] = 7;
	indexes[10*3+2] = 4;
	indexes[11*3+0] = 6;
	indexes[11*3+1] = 4;
	indexes[11*3+2] = 5;

	GenerateEdgeIndexes();
}

/*
====================
hfSurface_Polytope::SetupOctahedron
====================
*/
void hfSurface_Polytope::SetupOctahedron( const hfBounds &bounds ) {
	hfVec3 center, scale;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 6 );
	verts[0].xyz = center + hfVec3(  scale.x, 0.0f, 0.0f );
	verts[1].xyz = center + hfVec3( -scale.x, 0.0f, 0.0f );
	verts[2].xyz = center + hfVec3( 0.0f,  scale.y, 0.0f );
	verts[3].xyz = center + hfVec3( 0.0f, -scale.y, 0.0f );
	verts[4].xyz = center + hfVec3( 0.0f, 0.0f,  scale.z );
	verts[5].xyz = center + hfVec3( 0.0f, 0.0f, -scale.z );

	indexes.SetNum( 8*3 );
	indexes[0*3+0] = 4;
	indexes[0*3+1] = 0;
	indexes[0*3+2] = 2;
	indexes[1*3+0] = 4;
	indexes[1*3+1] = 2;
	indexes[1*3+2] = 1;
	indexes[2*3+0] = 4;
	indexes[2*3+1] = 1;
	indexes[2*3+2] = 3;
	indexes[3*3+0] = 4;
	indexes[3*3+1] = 3;
	indexes[3*3+2] = 0;
	indexes[4*3+0] = 5;
	indexes[4*3+1] = 2;
	indexes[4*3+2] = 0;
	indexes[5*3+0] = 5;
	indexes[5*3+1] = 1;
	indexes[5*3+2] = 2;
	indexes[6*3+0] = 5;
	indexes[6*3+1] = 3;
	indexes[6*3+2] = 1;
	indexes[7*3+0] = 5;
	indexes[7*3+1] = 0;
	indexes[7*3+2] = 3;

	GenerateEdgeIndexes();
}

/*
====================
hfSurface_Polytope::SetupDodecahedron
====================
*/
void hfSurface_Polytope::SetupDodecahedron( const hfBounds &bounds ) {
}

/*
====================
hfSurface_Polytope::SetupIcosahedron
====================
*/
void hfSurface_Polytope::SetupIcosahedron( const hfBounds &bounds ) {
}

/*
====================
hfSurface_Polytope::SetupCylinder
====================
*/
void hfSurface_Polytope::SetupCylinder( const hfBounds &bounds, const int numSides ) {
}

/*
====================
hfSurface_Polytope::SetupCone
====================
*/
void hfSurface_Polytope::SetupCone( const hfBounds &bounds, const int numSides ) {
}

/*
====================
hfSurface_Polytope::SplitPolytope
====================
*/
int hfSurface_Polytope::SplitPolytope( const hfPlane &plane, const float epsilon, hfSurface_Polytope **front, hfSurface_Polytope **back ) const {
	int side, i, j, s, v0, v1, v2, edgeNum;
	hfSurface *surface[2];
	hfSurface_Polytope *polytopeSurfaces[2], *surf;
	int *onPlaneEdges[2];

	onPlaneEdges[0] = (int *) _alloca( indexes.Num() / 3 * sizeof( int ) );
	onPlaneEdges[1] = (int *) _alloca( indexes.Num() / 3 * sizeof( int ) );

	side = Split( plane, epsilon, &surface[0], &surface[1], onPlaneEdges[0], onPlaneEdges[1] );

	*front = polytopeSurfaces[0] = new (TAG_IDLIB_SURFACE) hfSurface_Polytope;
	*back = polytopeSurfaces[1] = new (TAG_IDLIB_SURFACE) hfSurface_Polytope;

	for ( s = 0; s < 2; s++ ) {
		if ( surface[s] ) {
			polytopeSurfaces[s] = new hfSurface_Polytope( *surface[s] );
			delete surface[s];
			surface[s] = NULL;
		}
	}

	*front = polytopeSurfaces[0];
	*back = polytopeSurfaces[1];

	if ( side != SIDE_CROSS ) {
		return side;
	}

	// add triangles to close off the front and back polytope
	for ( s = 0; s < 2; s++ ) {

		surf = polytopeSurfaces[s];

		edgeNum = surf->edgeIndexes[onPlaneEdges[s][0]];
		v0 = surf->edges[abs(edgeNum)].verts[INT32_SIGNBITSET(edgeNum)];
		v1 = surf->edges[abs(edgeNum)].verts[INT32_SIGNBITNOTSET(edgeNum)];

		for ( i = 1; onPlaneEdges[s][i] >= 0; i++ ) {
			for ( j = i+1; onPlaneEdges[s][j] >= 0; j++ ) {
				edgeNum = surf->edgeIndexes[onPlaneEdges[s][j]];
				if ( v1 == surf->edges[abs(edgeNum)].verts[INT32_SIGNBITSET(edgeNum)] ) {
					v1 = surf->edges[abs(edgeNum)].verts[INT32_SIGNBITNOTSET(edgeNum)];
					SwapValues( onPlaneEdges[s][i], onPlaneEdges[s][j] );
					break;
				}
			}
		}

		for ( i = 2; onPlaneEdges[s][i] >= 0; i++ ) {
			edgeNum = surf->edgeIndexes[onPlaneEdges[s][i]];
			v1 = surf->edges[abs(edgeNum)].verts[INT32_SIGNBITNOTSET(edgeNum)];
			v2 = surf->edges[abs(edgeNum)].verts[INT32_SIGNBITSET(edgeNum)];
			surf->indexes.Append( v0 );
			surf->indexes.Append( v1 );
			surf->indexes.Append( v2 );
		}

		surf->GenerateEdgeIndexes();
	}

	return side;
}

