#ifndef __HF_CSG_SYSTEM_H__
#define __HF_CSG_SYSTEM_H__

#include "../hfEcs.h"
#include "../math/hfVector.h"
#include "../bv/hfBounds.h"
#include "hfSystem.h"
#include "hfCsgComponent.h"

/*
===============================================================================

	hfCsgSystem — tessellates CSG brushes registered as entities.

	The world owns one CsgSystem.  Every entity with a hfCsgComponent marked
	dirty gets its brush rebuilt into a CPU triangle soup (hfMeshSoup) with a
	watertight local AABB.  This is the non-destructive "source of truth" for
	rendering bounds, gazer/cull and later physics colliders/export.

===============================================================================
*/

namespace HF_Csg {

const int	HF_CSG_MAX_TRIS = 128;

struct hfMeshSoup {
	int					numTri;
	hfVec3				verts[ HF_CSG_MAX_TRIS * 3 ];

	HF_INLINE			hfMeshSoup() { numTri = 0; }
	HF_INLINE void		Clear() { numTri = 0; }
	HF_INLINE void		AddQuad( const hfVec3 &a, const hfVec3 &b, const hfVec3 &c, const hfVec3 &d ) {
		if ( numTri + 2 > HF_CSG_MAX_TRIS ) {
			return;
		}
		verts[ numTri * 3 + 0 ] = a;
		verts[ numTri * 3 + 1 ] = b;
		verts[ numTri * 3 + 2 ] = c;
		numTri++;
		verts[ numTri * 3 + 0 ] = a;
		verts[ numTri * 3 + 1 ] = c;
		verts[ numTri * 3 + 2 ] = d;
		numTri++;
	}
	HF_INLINE void		AddTri( const hfVec3 &a, const hfVec3 &b, const hfVec3 &c ) {
		if ( numTri + 1 > HF_CSG_MAX_TRIS ) {
			return;
		}
		verts[ numTri * 3 + 0 ] = a;
		verts[ numTri * 3 + 1 ] = b;
		verts[ numTri * 3 + 2 ] = c;
		numTri++;
	}
};

HF_INLINE void	TessellateShape( const HF_Scene::hfCsgComponent &comp, hfMeshSoup &soup ) {
	soup.Clear();
	switch ( comp.shape ) {
	case HF_Scene::HF_CSG_BOX: {
		const hfVec3 &	h = comp.halfExtents;
		// 6 faces (CCW outward), 12 triangles
		hfVec3			f[6][4];
		// +X
		f[0][0]=hfVec3( h.x,-h.y,-h.z); f[0][1]=hfVec3( h.x, h.y,-h.z); f[0][2]=hfVec3( h.x, h.y, h.z); f[0][3]=hfVec3( h.x,-h.y, h.z);
		// -X
		f[1][0]=hfVec3(-h.x,-h.y, h.z); f[1][1]=hfVec3(-h.x, h.y, h.z); f[1][2]=hfVec3(-h.x, h.y,-h.z); f[1][3]=hfVec3(-h.x,-h.y,-h.z);
		// +Y
		f[2][0]=hfVec3(-h.x, h.y,-h.z); f[2][1]=hfVec3( h.x, h.y,-h.z); f[2][2]=hfVec3( h.x, h.y, h.z); f[2][3]=hfVec3(-h.x, h.y, h.z);
		// -Y
		f[3][0]=hfVec3(-h.x,-h.y, h.z); f[3][1]=hfVec3( h.x,-h.y, h.z); f[3][2]=hfVec3( h.x,-h.y,-h.z); f[3][3]=hfVec3(-h.x,-h.y,-h.z);
		// +Z
		f[4][0]=hfVec3(-h.x,-h.y, h.z); f[4][1]=hfVec3( h.x,-h.y, h.z); f[4][2]=hfVec3( h.x, h.y, h.z); f[4][3]=hfVec3(-h.x, h.y, h.z);
		// -Z
		f[5][0]=hfVec3(-h.x, h.y,-h.z); f[5][1]=hfVec3( h.x, h.y,-h.z); f[5][2]=hfVec3( h.x,-h.y,-h.z); f[5][3]=hfVec3(-h.x,-h.y,-h.z);
		for ( int i = 0; i < 6; i++ ) {
			soup.AddQuad( f[i][0], f[i][1], f[i][2], f[i][3] );
		}
		break;
	}
	case HF_Scene::HF_CSG_SPHERE: {
		float			r = comp.halfExtents.x;
		const float		p = ( 1.0f + 2.414213562f ) * 0.0f + 0.6180339887f;	// = phi-1 (used below)
		( void )p;
		const float		phi = 1.6180339887f;
		const float		iPhi = 1.0f / phi;
		hfVec3			v[12];
		v[0] = hfVec3(-1.f, phi, 0.f); v[1] = hfVec3( 1.f, phi, 0.f);
		v[2] = hfVec3(-1.f,-phi, 0.f); v[3] = hfVec3( 1.f,-phi, 0.f);
		v[4] = hfVec3( 0.f,-1.f, phi); v[5] = hfVec3( 0.f, 1.f, phi);
		v[6] = hfVec3( 0.f,-1.f,-phi); v[7] = hfVec3( 0.f, 1.f,-phi);
		v[8] = hfVec3( phi, 0.f,-iPhi); v[9] = hfVec3( phi, 0.f, iPhi);
		v[10]= hfVec3(-phi, 0.f,-iPhi); v[11]= hfVec3(-phi, 0.f, iPhi);
		static const int faces[20][3] = {
			{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
			{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
			{3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
			{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
		};
		for ( int i = 0; i < 12; i++ ) {
			v[ i ].Normalize();		// returns length; normalizes in place
			v[ i ] = hfVec3( v[ i ].x * r, v[ i ].y * r, v[ i ].z * r );
		}
		for ( int i = 0; i < 20; i++ ) {
			soup.AddTri( v[ faces[i][0] ], v[ faces[i][1] ], v[ faces[i][2] ] );
		}
		break;
	}
	case HF_Scene::HF_CSG_QUAD: {
		const hfVec3 &	h = comp.halfExtents;
		// X-Y plane at z=0 (matches the map grid convention)
		soup.AddQuad( hfVec3(-h.x,-h.y,0), hfVec3( h.x,-h.y,0), hfVec3( h.x, h.y,0), hfVec3(-h.x, h.y,0) );
		break;
	}
	case HF_Scene::HF_CSG_NONE:
	default:
		break;
	}
}

HF_INLINE void	BoundsFromSoup( const hfMeshSoup &soup, hfBounds &out ) {
	out.Clear();
	for ( int t = 0; t < soup.numTri; t++ ) {
		out.AddPoint( soup.verts[ t * 3 + 0 ] );
		out.AddPoint( soup.verts[ t * 3 + 1 ] );
		out.AddPoint( soup.verts[ t * 3 + 2 ] );
	}
}

} // namespace HF_Csg

class hfCsgSystem : public HF_Scene::hfSystem {
public:
	const char *		GetName() const override { return "CsgSystem"; }
	int					GetPriority() const override { return 40; }	// before render cull (30) uses bounds

	void				Update( HF_Ecs::Registry &registry, float dt ) override {
		( void )dt;
		registry.Each< HF_Scene::hfCsgComponent >( [ this ]( HF_Ecs::Entity e, HF_Scene::hfCsgComponent &c ) {
			( void )e;
			if ( !c.dirty ) {
				return;
			}
			HF_Csg::TessellateShape( c, m_soup );
			HF_Csg::BoundsFromSoup( m_soup, c.localBounds );
			c.triCount = m_soup.numTri;
			c.dirty = false;
		} );
	}

	const HF_Csg::hfMeshSoup &	GetLastSoup() const { return m_soup; }

private:
	HF_Csg::hfMeshSoup			m_soup;
};

#endif