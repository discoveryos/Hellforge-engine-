#ifndef __BV_COLLIDE_H__
#define __BV_COLLIDE_H__

/*
======================================================================
	Collision query module.

	A fresh Hellforge implementation of the collision layer pattern
	used by SynapseEngine:

	  * CollisionTester approaches (sphere/AABB/frustum/cone queries,
	    Outside/Intersect/Inside classification, screen-space size and
	    LOD pick) are provided as a header-only namespace below.
	  * The mesh-collision slice (SynapseEngine bakes model
	    physicsVertices + physicsIndicesPerLod into a physics-engine
	    mesh body via IP hysicsEngine::CreateMeshBody / SetMeshShape)
	    is represented here by hfMeshCollider: an index-triangle soup
	    with closest-point, ray and swept-sphere queries.  The runtime
	    engine can hand this same data to a full dynamics engine (Jolt,
	    as Synapse does) later; the guarantees keep working headless.

	All of it is plain math + hfBounds, so it compiles and self-tests
	with no GPU / OS dependency.  Include hfprecompiled.h first.
======================================================================
*/

#include "../math/hfPlane.h"
#include "hfBounds.h"

// Outside = disjoint, Intersect = touching/overlapping but not contained,
// Inside = fully contained.  Mirrors Synapse's IntersectionType.
enum hfIntersection {
	HF_I_OUTSIDE	= 0,
	HF_I_INTERSECT	= 1,
	HF_I_INSIDE		= 2,
};

/*
===============================================================
	Frustum: 6 inward-facing half-space planes in world space.

	Planes face inward, so a point is inside when
	plane.Distance( point ) >= 0 for all six.
===============================================================
*/
class hfFrustum {
public:
	enum {
		HF_FRUSTUM_LEFT = 0,
		HF_FRUSTUM_RIGHT,
		HF_FRUSTUM_BOTTOM,
		HF_FRUSTUM_TOP,
		HF_FRUSTUM_NEAR,
		HF_FRUSTUM_FAR,
		HF_FRUSTUM_PLANES = 6,
	};

	hfPlane			planes[ HF_FRUSTUM_PLANES ];

	HF_INLINE hfFrustum() {
	}

	// Gribb-Hartmann extraction from a combined view-projection matrix.
	// hfMat4 is row-major and pre-multiplies (clip = M * v), stores rows
	// via operator[] (m[row][col]), so the six clip-space half-space rows
	// are (row[3] +/- row[row]) and a point is inside while
	// plane.Distance( v ) >= 0.  Each plane is scaled to a unit normal
	// (scaling d together with a/b/c) so Distance() is a true signed
	// distance.
	HF_INLINE void FromViewProjMatrix( const hfMat4 &m ) {
		for ( int p = 0; p < HF_FRUSTUM_PLANES; p++ ) {
			int row = p >> 1;							// 0,1,2 -> x,y,z half-space
			bool subtract = ( p & 1 ) != 0;				// 0: row3 + rowX  1: row3 - rowX
			hfVec3 n;
			for ( int c = 0; c < 3; c++ ) {
				n[ c ] = subtract ? ( m[ 3 ][ c ] - m[ row ][ c ] ) : ( m[ 3 ][ c ] + m[ row ][ c ] );
			}
			float d = subtract ? ( m[ 3 ][ 3 ] - m[ row ][ 3 ] ) : ( m[ 3 ][ 3 ] + m[ row ][ 3 ] );
			float len = n.Length();
			if ( len > 0.0000001f ) {
				float inv = 1.0f / len;
				n.x *= inv; n.y *= inv; n.z *= inv;
				d *= inv;
			}
			planes[ p ].SetNormal( n );
			planes[ p ].SetDist( -d );
		}
	}

	// Test helpers --------------------------------------------------
	HF_INLINE float SignedPlaneDistance( const hfPlane &plane, const hfVec3 &point ) const {
		return plane.Distance( point );
	}

	HF_INLINE bool ContainsPoint( const hfVec3 &point ) const {
		for ( int i = 0; i < HF_FRUSTUM_PLANES; i++ ) {
			if ( planes[ i ].Distance( point ) < 0.0f ) { return false; }
		}
		return true;
	}
};

// Explicit 6-plane construction helper so tests can build an exact
// frustum (near face + 4 side planes + far face) from a view.
HF_INLINE void hfFrustum_FromCorners( hfFrustum &f, const hfVec3 corners[ 8 ] ) {
	// corners: near face 0..3, far face 4..7 (0=TL,1=TR,2=BL,3=BR)
	for ( int i = 0; i < 6; i++ ) { f.planes[ i ] = hfPlane( 0.0f, 0.0f, 0.0f, 0.0f ); }
	// left: through near-TL, near-BL, far-TL
	{
		hfPlane p;
		p.SetNormal( ( corners[ 2 ] - corners[ 0 ] ).Cross( corners[ 4 ] - corners[ 0 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 0 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_LEFT ] = p;
	}
	// right: through near-TR, near-BR, far-TR
	{
		hfPlane p;
		p.SetNormal( ( corners[ 3 ] - corners[ 1 ] ).Cross( corners[ 1 ] - corners[ 5 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 1 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_RIGHT ] = p;
	}
	// bottom: through near-BL, near-BR, far-BL
	{
		hfPlane p;
		p.SetNormal( ( corners[ 3 ] - corners[ 2 ] ).Cross( corners[ 6 ] - corners[ 2 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 2 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_BOTTOM ] = p;
	}
	// top: through near-TL, near-TR, far-TL
	{
		hfPlane p;
		p.SetNormal( ( corners[ 1 ] - corners[ 0 ] ).Cross( corners[ 0 ] - corners[ 4 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 0 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_TOP ] = p;
	}
	// near: through the near face (normal = forward)
	{
		hfPlane p;
		p.SetNormal( ( corners[ 1 ] - corners[ 0 ] ).Cross( corners[ 2 ] - corners[ 0 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 0 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_NEAR ] = p;
	}
	// far: mirrored normal through the far face
	{
		hfPlane p;
		p.SetNormal( ( corners[ 2 ] - corners[ 0 ] ).Cross( corners[ 1 ] - corners[ 0 ] ) );
		p.Normalize();
		p.FitThroughPoint( corners[ 4 ] );
		f.planes[ hfFrustum::HF_FRUSTUM_FAR ] = p;
	}
}

/*
===============================================================
	Query tester namespace (Synapse CollisionTester idiomatic swap).
===============================================================
*/
namespace hfCollide {

HF_INLINE float DistanceSq( const hfVec3 &a, const hfVec3 &b ) {
	hfVec3 d = a - b;
	return d.LengthSqr();
}

// Sphere/Sphere ---------------------------------------------------
HF_INLINE bool TestSphereSphere( const hfVec3 &ca, float ra, const hfVec3 &cb, float rb ) {
	float d2 = DistanceSq( ca, cb );
	float r = ra + rb;
	return d2 <= r * r;
}

HF_INLINE hfIntersection TestSphereSphereType( const hfVec3 &ca, float ra, const hfVec3 &cb, float rb ) {
	float d2 = DistanceSq( ca, cb );
	float r = ra + rb;
	if ( d2 > r * r ) { return HF_I_OUTSIDE; }
	float diff = ra - rb;
	if ( d2 <= diff * diff ) { return HF_I_INSIDE; }	// one fully inside the other (either direction)
	return HF_I_INTERSECT;
}

// Sphere/AABB -----------------------------------------------------
HF_INLINE hfVec3 ClosestPointOnBounds( const hfBounds &b, const hfVec3 &p ) {
	hfVec3 cp;
	cp.x = ( p.x < b[ 0 ].x ) ? b[ 0 ].x : ( ( p.x > b[ 1 ].x ) ? b[ 1 ].x : p.x );
	cp.y = ( p.y < b[ 0 ].y ) ? b[ 0 ].y : ( ( p.y > b[ 1 ].y ) ? b[ 1 ].y : p.y );
	cp.z = ( p.z < b[ 0 ].z ) ? b[ 0 ].z : ( ( p.z > b[ 1 ].z ) ? b[ 1 ].z : p.z );
	return cp;
}

HF_INLINE bool TestSphereBounds( const hfVec3 &c, float r, const hfBounds &b ) {
	return DistanceSq( c, ClosestPointOnBounds( b, c ) ) <= r * r;
}

HF_INLINE hfIntersection TestSphereBoundsType( const hfVec3 &c, float r, const hfBounds &b ) {
	if ( !TestSphereBounds( c, r, b ) ) { return HF_I_OUTSIDE; }
	// fully inside iff the whole sphere fits within the box on every axis
	if ( c.x - r >= b[ 0 ].x && c.x + r <= b[ 1 ].x &&
		 c.y - r >= b[ 0 ].y && c.y + r <= b[ 1 ].y &&
		 c.z - r >= b[ 0 ].z && c.z + r <= b[ 1 ].z ) { return HF_I_INSIDE; }
	return HF_I_INTERSECT;
}

// Sphere/AABB (Synapse's expanded-clamp variant) ------------------
HF_INLINE bool TestSphereAabb( const hfVec3 &c, float r, const hfBounds &b ) { return TestSphereBounds( c, r, b ); }

// Frustum ---------------------------------------------------------
HF_INLINE bool TestSphereFrustum( const hfVec3 &c, float r, const hfFrustum &f ) {
	for ( int i = 0; i < 6; i++ ) {
		if ( f.planes[ i ].Distance( c ) < -r ) { return false; }
	}
	return true;
}

HF_INLINE hfIntersection TestSphereFrustumType( const hfVec3 &c, float r, const hfFrustum &f ) {
	bool touching = false;
	for ( int i = 0; i < 6; i++ ) {
		float d = f.planes[ i ].Distance( c );
		if ( d < -r ) { return HF_I_OUTSIDE; }
		if ( d < r ) { touching = true; }
	}
	return touching ? HF_I_INTERSECT : HF_I_INSIDE;
}

HF_INLINE bool TestBoundsFrustum( const hfBounds &b, const hfFrustum &f ) {
	hfVec3 center = b.GetCenter();
	hfVec3 half = b[ 1 ] - center;
	for ( int i = 0; i < 6; i++ ) {
		const hfPlane &p = f.planes[ i ];
		float radius = hfMath::Fabs( half.x * p[ 0 ] ) + hfMath::Fabs( half.y * p[ 1 ] ) + hfMath::Fabs( half.z * p[ 2 ] );
		float d = p.Distance( center );
		if ( d < -radius ) { return false; }
	}
	return true;
}

HF_INLINE hfIntersection TestBoundsFrustumType( const hfBounds &b, const hfFrustum &f ) {
	hfVec3 center = b.GetCenter();
	hfVec3 half = b[ 1 ] - center;
	bool touching = false;
	for ( int i = 0; i < 6; i++ ) {
		const hfPlane &p = f.planes[ i ];
		float radius = hfMath::Fabs( half.x * p[ 0 ] ) + hfMath::Fabs( half.y * p[ 1 ] ) + hfMath::Fabs( half.z * p[ 2 ] );
		float d = p.Distance( center );
		if ( d < -radius ) { return HF_I_OUTSIDE; }
		if ( d < radius ) { touching = true; }
	}
	return touching ? HF_I_INTERSECT : HF_I_INSIDE;
}

// Two-stage culling used by Synapse: reject with the sphere test,
// accept with the exact AABB test when the sphere is only touching.
HF_INLINE bool IsInFrustum( const hfVec3 &c, float r, const hfBounds &b, const hfFrustum &f ) {
	hfIntersection s = TestSphereFrustumType( c, r, f );
	if ( s == HF_I_OUTSIDE ) { return false; }
	if ( s == HF_I_INSIDE ) { return true; }
	return TestBoundsFrustum( b, f );
}

HF_INLINE hfIntersection IsInFrustumType( const hfVec3 &c, float r, const hfBounds &b, const hfFrustum &f ) {
	hfIntersection s = TestSphereFrustumType( c, r, f );
	if ( s != HF_I_INTERSECT ) { return s; }
	return TestBoundsFrustumType( b, f );
}

// Cone/Sphere (spot fights, sound cones) --------------------------
HF_INLINE bool TestConeSphere( const hfVec3 &conePos, const hfVec3 &coneDir, float coneRange, float coneCos, float coneSin, const hfVec3 &c, float r ) {
	hfVec3 v = c - conePos;
	float lenSq = v.LengthSqr();
	float along = v * coneDir;
	float perp = hfMath::Sqrt( lenSq - along * along < 0.0f ? 0.0f : lenSq - along * along );
	float angCull = coneCos * perp - along * coneSin;
	bool angleCull = angCull > r;
	bool frontCull = along > r + coneRange;
	bool backCull = along < -r;
	return !( angleCull || frontCull || backCull );
}

// Screen-space size / LOD -----------------------------------------
HF_INLINE float SphereScreenSize( const hfVec3 &viewPos, float radius, float projYScale, const hfVec2 &screenRes ) {
	// viewPos is the center in view space (looking down -Z).
	float dist = -viewPos.z;
	if ( dist <= radius || dist <= 0.000001f ) { return 99999.0f; }
	float pixels = ( radius / dist ) * projYScale * screenRes.y;
	return pixels;
}

HF_INLINE unsigned int CalculateLodFromScreenSize( float screenSizePixels ) {
	if ( screenSizePixels > 512.0f ) { return 0; }
	if ( screenSizePixels > 256.0f ) { return 1; }
	if ( screenSizePixels > 128.0f ) { return 2; }
	return 3;
}

/*
===============================================================
	Triangle closest-point & ray helpers.
===============================================================
*/
HF_INLINE hfVec3 ClosestPointOnTriangle( const hfVec3 &p, const hfVec3 &a, const hfVec3 &b, const hfVec3 &c, float &s, float &t ) {
	hfVec3 ab = b - a;
	hfVec3 ac = c - a;
	hfVec3 ap = p - a;
	float d1 = ab * ap;
	float d2 = ac * ap;
	if ( d1 <= 0.0f && d2 <= 0.0f ) { s = 0.0f; t = 0.0f; return a; }

	hfVec3 bp = p - b;
	float d3 = ab * bp;
	float d4 = ac * bp;
	if ( d3 >= 0.0f && d4 <= d3 ) { s = 1.0f; t = 0.0f; return b; }

	float vc = d1 * d4 - d3 * d2;
	if ( vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f ) {
		float v = d1 / ( d1 - d3 );
		s = v; t = 0.0f;
		return a + ab * v;
	}

	hfVec3 cp = p - c;
	float d5 = ab * cp;
	float d6 = ac * cp;
	if ( d6 >= 0.0f && d5 <= d6 ) { s = 0.0f; t = 1.0f; return c; }

	float vb = d5 * d2 - d1 * d6;
	if ( vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f ) {
		float w = d2 / ( d2 - d6 );
		s = 0.0f; t = w;
		return a + ac * w;
	}

	float va = d3 * d6 - d5 * d4;
	if ( va <= 0.0f && ( d4 - d3 ) >= 0.0f && ( d5 - d6 ) >= 0.0f ) {
		float w = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );
		s = 1.0f - w; t = w;
		return b + ( c - b ) * w;
	}

	float denom = 1.0f / ( va + vb + vc );
	float v = vb * denom;
	float w = vc * denom;
	s = v; t = w;
	return a + ab * v + ac * w;
}

// Moller-Trumbore ray vs triangle.  Returns true + t if the ray hits
// within [0, tMax]; 'u/v' are barycentric at the hit.
HF_INLINE bool RayTriangle( const hfVec3 &origin, const hfVec3 &dir, float tMax, const hfVec3 &a, const hfVec3 &b, const hfVec3 &c, float &t, float &u, float &v ) {
	hfVec3 e1 = b - a;
	hfVec3 e2 = c - a;
	hfVec3 pvec = dir.Cross( e2 );
	float det = e1 * pvec;
	if ( det > -0.000001f && det < 0.000001f ) { return false; }
	float inv = 1.0f / det;
	hfVec3 tvec = origin - a;
	u = ( tvec * pvec ) * inv;
	if ( u < -0.000001f || u > 1.000001f ) { return false; }
	hfVec3 qvec = tvec.Cross( e1 );
	v = ( dir * qvec ) * inv;
	if ( v < -0.000001f || u + v > 1.000001f ) { return false; }
	t = ( e2 * qvec ) * inv;
	if ( t < 0.0f || t > tMax ) { return false; }
	return true;
}

} // namespace hfCollide

/*
===============================================================
	Indexed triangle-mesh collider (baked physics mesh).

	Owns nothing: the model/stream layer hands vertex + index arrays
	(world space, as SynapseEngine bakes physicsVertices /
	physicsIndicesPerLod) and the hull answers closest-point, ray and
	swept-sphere queries over every triangle with an AABB precheck.
===============================================================
*/
class hfMeshCollider {
public:
	const hfVec3 *	vertices;
	int				numVertices;
	const int *		indices;			// 3 indices per triangle
	int				numTriangles;
	hfBounds		bounds;

	HF_INLINE hfMeshCollider() {
		vertices = NULL;
		numVertices = 0;
		indices = NULL;
		numTriangles = 0;
	}

	HF_INLINE void Set( const hfVec3 *verts, int numVerts, const int *tris, int numTris ) {
		vertices = verts;
		numVertices = numVerts;
		indices = tris;
		numTriangles = numTris;
		bounds.Clear();
		for ( int i = 0; i < numVerts; i++ ) { bounds.AddPoint( verts[ i ] ); }
	}

	HF_INLINE bool IsValid() const { return vertices != NULL && indices != NULL && numTriangles > 0; }

	// Closest point on the whole soup to 'p'.
	HF_INLINE hfVec3 ClosestPoint( const hfVec3 &p ) const {
		hfVec3 best;
		float bestD = 0.0f;
		bool have = false;
		for ( int i = 0; i < numTriangles; i++ ) {
			const hfVec3 &a = vertices[ indices[ i * 3 + 0 ] ];
			const hfVec3 &b = vertices[ indices[ i * 3 + 1 ] ];
			const hfVec3 &c = vertices[ indices[ i * 3 + 2 ] ];
			float s, t;
			hfVec3 cp = hfCollide::ClosestPointOnTriangle( p, a, b, c, s, t );
			float d2 = ( p - cp ).LengthSqr();
			if ( !have || d2 < bestD ) {
				have = true;
				bestD = d2;
				best = cp;
			}
		}
		return best;
	}

	// Ray cast; returns the closest hit (t, normal, triangle index).
	HF_INLINE bool RayTrace( const hfVec3 &origin, const hfVec3 &dir, float tMax, float &tOut, hfVec3 &normalOut, int &triOut ) const {
		tOut = tMax;
		triOut = -1;
		bool hit = false;
		for ( int i = 0; i < numTriangles; i++ ) {
			const hfVec3 &a = vertices[ indices[ i * 3 + 0 ] ];
			const hfVec3 &b = vertices[ indices[ i * 3 + 1 ] ];
			const hfVec3 &c = vertices[ indices[ i * 3 + 2 ] ];
			float t, u, v;
			if ( hfCollide::RayTriangle( origin, dir, tOut, a, b, c, t, u, v ) ) {
				tOut = t;
				triOut = i;
				normalOut = ( b - a ).Cross( c - a );
				normalOut.Normalize();
				hit = true;
			}
		}
		return hit;
	}

	// Swept sphere (start -> end, radius r).  Returns the impact time
	// 0..1 (or false), the contact point and the contact normal.
	// Checks the inflated-face plane, then the three edge capsules and
	// the corner spheres (Kasper Fauerby's swept-sphere-triangle path).
	HF_INLINE bool SphereTrace( const hfVec3 &start, const hfVec3 &end, float radius, float &tOut, hfVec3 &pointOut, hfVec3 &normalOut, int &triOut ) const {
		hfVec3 seg = end - start;
		float segLen = seg.Length();
		tOut = 1.0f;
		pointOut = end;
		normalOut = hfVec3( 0.0f, 0.0f, 1.0f );
		triOut = -1;
		if ( segLen <= 0.000001f ) { return false; }
		hfVec3 dir = seg * ( 1.0f / segLen );
		bool hit = false;

		for ( int i = 0; i < numTriangles; i++ ) {
			const hfVec3 &a = vertices[ indices[ i * 3 + 0 ] ];
			const hfVec3 &b = vertices[ indices[ i * 3 + 1 ] ];
			const hfVec3 &c = vertices[ indices[ i * 3 + 2 ] ];

			// ---- face: distance of the moving sphere's center to the
			// triangle plane crossing +/- radius inside the triangle
			hfVec3 n = ( b - a ).Cross( c - a );
			n.Normalize();
			float planeD = n * a;
			float d0 = n * start - planeD;
			float d1 = n * end - planeD;

			float tPlane = -1.0f;
			float s0 = 0.0f, s1 = 0.0f;
			if ( d0 >= radius && d1 < radius ) {			// crossing into the side the normal faces
				tPlane = ( d0 - radius ) / ( d0 - d1 );
				float s = tPlane * segLen;
				hfVec3 centerOnPlane = start + dir * s - n * radius;
				hfVec3 closest = hfCollide::ClosestPointOnTriangle( centerOnPlane, a, b, c, s0, s1 );
				if ( hfCollide::DistanceSq( centerOnPlane, closest ) <= 0.0004f ) {
					tOut = tPlane;
					normalOut = n;
					pointOut = centerOnPlane;
					triOut = i;
					hit = true;
				}
			} else if ( d0 < -radius && d1 >= -radius ) {	// crossing the back face
				tPlane = ( -d0 - radius ) / ( d1 - d0 );
				float s = tPlane * segLen;
				hfVec3 centerOnPlane = start + dir * s + n * radius;
				hfVec3 closest = hfCollide::ClosestPointOnTriangle( centerOnPlane, a, b, c, s0, s1 );
				if ( hfCollide::DistanceSq( centerOnPlane, closest ) <= 0.0004f ) {
					tOut = tPlane;
					normalOut = -n;
					pointOut = centerOnPlane;
					triOut = i;
					hit = true;
				}
			}

			// ---- edges: segment/capsule (center path vs edge + radius)
			const hfVec3 * edge[ 3 ][ 2 ] = { { &a, &b }, { &b, &c }, { &c, &a } };
			for ( int e = 0; e < 3; e++ ) {
				const hfVec3 &p1 = *edge[ e ][ 0 ];
				const hfVec3 &p2 = *edge[ e ][ 1 ];
				hfVec3 ab = p2 - p1;
				// closest points between the segment [start,end] and ab
				hfVec3 delta = start - p1;
				float ab2 = ab.LengthSqr();
				float abSeg = ab * dir;					// = -(ab * (-dir))
				float deltaAb = delta * ab;
				float seg2 = segLen * segLen;
				float a = seg2;
				float b2 = abSeg;
				float c2 = deltaAb;
				float d = delta * dir;
				float e2 = delta * delta;
				float denom = a * ab2 - b2 * b2;
				float tSeg, tEdge;
				if ( hfMath::Fabs( denom ) < 0.000001f ) {
					tSeg = 0.0f;
					tEdge = d / ab2;
				} else {
					tSeg = ( b2 * c2 - a * d ) / denom;
					tEdge = ( ab2 * c2 - b2 * d ) / denom;
				}
				if ( tSeg < 0.0f ) { tSeg = 0.0f; } else if ( tSeg > 1.0f ) { tSeg = 1.0f; }
				if ( tEdge < 0.0f ) { tEdge = 0.0f; } else if ( tEdge > 1.0f ) { tEdge = 1.0f; }
				hfVec3 cpA = start + dir * ( tSeg * segLen );
				hfVec3 cpB = p1 + ab * tEdge;
				hfVec3 between = cpB - cpA;
				float dist2 = between.LengthSqr();
				if ( dist2 <= radius * radius ) {
					float t = tSeg - hfMath::Sqrt( radius * radius - dist2 ) / segLen;
					if ( t < 0.0f ) { t = 0.0f; }
					if ( t < tOut ) {
						tOut = t;
						// Normal points from the nearest edge point toward the
						// sphere center (out of the mesh surface), so the sphere
						// can be pushed back by normalOut * radius.
						if ( dist2 > 0.0000001f ) {
							float invLen = 1.0f / hfMath::Sqrt( dist2 );
							normalOut = between * (-invLen);
						} else {
							normalOut = hfVec3( 0.0f, 0.0f, 1.0f );
						}
						hfVec3 center = start + dir * ( t * segLen );
						pointOut = center - normalOut * radius;
						triOut = i;
						hit = true;
					}
				}
			}
		}
		return hit;
	}
};

#endif