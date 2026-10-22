#ifndef __HF_GLTF_H__
#define __HF_GLTF_H__

#include "../hfStr.h"
#include "../hfBase64.h"
#include "../hfAssetStore.h"
#include "../math/hfVector.h"
#include "../math/hfMatrix.h"
#include "../math/hfQuat.h"
#include "../bv/hfBounds.h"
#include "hfJson.h"
#include <string.h>
#include <stdio.h>

/*
===============================================================================

	hfGltf — offline glTF 2.0 *subset* importer.

	Reads node/child/transform hierarchy, one mesh primitive per mesh
	(POSITION + optional NORMAL/TEXCOORD_0 + INDICES), and the PBR
	metallic-roughness material factors.  Supports data-URI (base64) buffers
	only — no external file reads — so the whole pipeline runs headlessly in
	the harness.  Binary accessors are decoded through the shared
	accessor/bufferView tables.  Output is bounded fixed arrays (no dynamic
	allocation): anything larger is rejected with an error string.

	Game-side indices are triangles; VEC3 positions are mesh-local, matching
	the mesh renderer's unit used everywhere in Phase 5.

===============================================================================
*/

namespace HF_Gltf {

static const int	GLTF_FLOAT		= 5126;
static const int	GLTF_U16		= 5123;
static const int	GLTF_U32		= 5125;

static const int	kMaxNodes		= 128;
static const int	kMaxMeshes		= 64;
static const int	kMaxMats		= 64;
static const int	kMaxVerts		= 4096;
static const int	kMaxTris		= 4096;
static const int	kMaxBytes		= 256 * 1024;

struct hfGltfNode {
	char				name[64];
	int					mesh;
	int					numChildren;
	int					children[16];
	float				local[16];	// column-major 4x4: T*R*S (glTF convention)

	HF_INLINE			hfGltfNode() { memset( this, 0, sizeof( *this ) ); mesh = -1; for ( int i = 0; i < 16; i++ ) { local[ i ] = ( i % 5 ) ? 0.0f : 1.0f; } }
};

struct hfGltfMesh {
	char				name[64];
	int					firstTri;	// index into doc.tris
	int					triCount;
	int					firstVert;	// index into doc.verts (mesh-local run)
	int					vertCount;
	hfBounds			bounds;		// mesh-local

	HF_INLINE			hfGltfMesh() { memset( this, 0, sizeof( *this ) ); }
};

// PBR metallic-roughness factors only (texture refs stay in the .mtr).
struct hfGltfMaterial {
	float				baseColor[4];
	float				metallic;
	float				roughness;
	HF_INLINE			hfGltfMaterial() { baseColor[0] = baseColor[1] = baseColor[2] = baseColor[3] = 1.0f; metallic = 0.0f; roughness = 1.0f; }
};

struct hfGltfDoc {
	int					numNodes;
	int					numMeshes;
	int					numMats;
	int					numVerts;
	int					numTris;
	float				verts[ kMaxVerts * 3 ];
	int					tris[ kMaxTris * 3 ];
	hfGltfNode			nodes[ kMaxNodes ];
	hfGltfMesh			meshes[ kMaxMeshes ];
	hfGltfMaterial		mats[ kMaxMats ];

	HF_INLINE			hfGltfDoc() { memset( this, 0, sizeof( *this ) ); }
};

// column-major 4x4 point transform (col*4+row)
HF_INLINE void	TransformPoint( const float m[16], const hfVec3 &p, hfVec3 &out ) {
	out.x = m[0] * p.x + m[4] * p.y + m[8 ] * p.z + m[12];
	out.y = m[1] * p.x + m[5] * p.y + m[9 ] * p.z + m[13];
	out.z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
}

// --- accessor decode plumbing ----------------------------------------------

namespace Detail {

struct hfAcc {
	int					view;		// bufferView index, -1 = none
	int					byteOffset;
	int					compType;	// GLTF_FLOAT / U16 / U32
	int					count;
	int					compCount;	// 1..4 (SCALAR..VEC4)
};

HF_INLINE int	ComponentSize( int compType ) {
	switch ( compType ) {
	case GLTF_FLOAT: return 4;
	case GLTF_U16:	return 2;
	case GLTF_U32:	return 4;
	default:		return 0;
	}
}

HF_INLINE bool	ReadAccessor( const hfAcc &acc, const byte *buf, const int *viewByte, float *outF3 ) {
	const byte *	base = buf;
	if ( acc.view >= 0 ) {
		base += viewByte[ acc.view ];
	}
	base += acc.byteOffset;
	int				compSize = ComponentSize( acc.compType );
	if ( compSize == 0 || acc.compCount < 1 || acc.compCount > 4 ) {
		return false;
	}
	for ( int i = 0; i < acc.count; i++ ) {
		const byte *	src = base + i * compSize * acc.compCount;
		for ( int c = 0; c < acc.compCount; c++ ) {
			if ( acc.compType == GLTF_FLOAT ) {
				float	f;
				memcpy( &f, src + c * 4, 4 );
				outF3[ i * 3 + c ] = f;
			} else if ( acc.compType == GLTF_U16 ) {
				unsigned short	u;
				memcpy( &u, src + c * 2, 2 );
				outF3[ i * 3 + c ] = ( float )u;
			} else {
				unsigned int	u;
				memcpy( &u, src + c * 4, 4 );
				outF3[ i * 3 + c ] = ( float )u;
			}
		}
	}
	return true;
}

HF_INLINE bool	ReadIndices( const hfAcc &acc, const byte *buf, const int *viewByte, int *outIdx, int cap ) {
	const byte *	base = buf;
	if ( acc.view >= 0 ) {
		base += viewByte[ acc.view ];
	}
	base += acc.byteOffset;
	if ( acc.compCount != 1 ) {
		return false;
	}
	for ( int i = 0; i < acc.count; i++ ) {
		if ( i >= cap ) {
			return false;
		}
		if ( acc.compType == GLTF_U16 ) {
			unsigned short	u;
			memcpy( &u, base + i * 2, 2 );
			outIdx[ i ] = ( int )u;
		} else if ( acc.compType == GLTF_U32 ) {
			unsigned int	u;
			memcpy( &u, base + i * 4, 4 );
			outIdx[ i ] = ( int )u;
		} else {
			return false;
		}
	}
	return true;
}

// data-uri: "data:...;base64,<payload>"
HF_INLINE bool	DecodeBufferUri( const char *uri, byte *out, int outCap, int &outLen ) {
	if ( memcmp( uri, "data:", 5 ) ) {
		return false;
	}
	const char *	comma = strchr( uri, ',' );
	if ( !comma ) {
		return false;
	}
	// hfBase64::Decode routes through the endian-swap function pointers that
	// are only installed by Swap_Init() — ensure they are set.
	static bool		swapInited = false;
	if ( !swapInited ) {
		swapInited = true;
		Swap_Init();
	}
	hfStr			payload = comma + 1;
	hfBase64		b64( payload );
	int				need = b64.DecodeLength();
	if ( need < 0 || need > outCap ) {
		return false;
	}
	outLen = b64.Decode( out );
	return true;
}

} // namespace Detail

// --- import ----------------------------------------------------------------

// Returns number of nodes imported, or -1 with message in err.
HF_INLINE int	ImportGltf( const char *json, size_t len, hfGltfDoc &out, hfStr &err ) {
	if ( !json || len == 0 ) {
		err = "empty glTF input";
		return -1;
	}
	HF_Json::Value		root;
	if ( !HF_Json::Parse( json, len, root, err ) ) {
		return -1;
	}
	if ( !root.IsObj() ) {
		err = "root is not an object";
		HF_Json::Free( root );
		return -1;
	}

	// bufferViews
	const HF_Json::Value *views = root.Find( "bufferViews" );
	int				numViews = 0;
	int				viewByte[ 64 ];
	if ( views && views->IsArr() ) {
		numViews = views->Count();
		if ( numViews > 64 ) {
			err = "too many bufferViews";
			HF_Json::Free( root );
			return -1;
		}
		for ( int i = 0; i < numViews; i++ ) {
			viewByte[ i ] = ( int )HF_Json::GetNum( *views->Get( i ), "byteOffset", 0.0 );
		}
	}

	// accessors
	struct HF_Gltf::Detail::hfAcc	accs[ 128 ];
	int				numAccs = 0;
	const HF_Json::Value *accessors = root.Find( "accessors" );
	if ( accessors && accessors->IsArr() ) {
		numAccs = accessors->Count();
		if ( numAccs > 128 ) {
			err = "too many accessors";
			HF_Json::Free( root );
			return -1;
		}
		for ( int i = 0; i < numAccs; i++ ) {
			const HF_Json::Value *a = accessors->Get( i );
			accs[ i ].view = ( int )HF_Json::GetNum( *a, "bufferView", -1.0 );
			accs[ i ].byteOffset = ( int )HF_Json::GetNum( *a, "byteOffset", 0.0 );
			accs[ i ].compType = ( int )HF_Json::GetNum( *a, "componentType", ( double )GLTF_FLOAT );
			accs[ i ].count = ( int )HF_Json::GetNum( *a, "count", 0.0 );
			const char *t = HF_Json::GetStr( *a, "type", "SCALAR" );
			int			cc = 1;
			if ( t[0] == 'V' && t[1] == 'E' ) cc = t[3] - '0';
			accs[ i ].compCount = cc;
		}
	}

	// buffer payload (single slot for the subset)
	byte			bufferData[ kMaxBytes ];
	int				bufferLen = 0;
	const HF_Json::Value *buffers = root.Find( "buffers" );
	if ( buffers && buffers->IsArr() && buffers->Count() > 0 ) {
		const char *uri = HF_Json::GetStr( *buffers->Get( 0 ), "uri", NULL );
		if ( uri && !Detail::DecodeBufferUri( uri, bufferData, kMaxBytes, bufferLen ) ) {
			err = "buffer decode failed";
			HF_Json::Free( root );
			return -1;
		}
	}

	// materials
	const HF_Json::Value *materials = root.Find( "materials" );
	if ( materials && materials->IsArr() ) {
		out.numMats = materials->Count();
		if ( out.numMats > kMaxMats ) {
			err = "too many materials";
			HF_Json::Free( root );
			return -1;
		}
		for ( int i = 0; i < out.numMats; i++ ) {
			const HF_Json::Value *m = materials->Get( i );
			const HF_Json::Value *pbr = m->Find( "pbrMetallicRoughness" );
			hfGltfMaterial &	mat = out.mats[ i ];
			if ( pbr ) {
				const HF_Json::Value *bcf = pbr->Find( "baseColorFactor" );
				if ( bcf && bcf->IsArr() ) {
					for ( int k = 0; k < 4; k++ ) {
						mat.baseColor[ k ] = ( float )bcf->AsVec( k, 1.0 );
					}
				}
				mat.metallic = ( float )HF_Json::GetNum( *pbr, "metallicFactor", 1.0 );
				mat.roughness = ( float )HF_Json::GetNum( *pbr, "roughnessFactor", 1.0 );
			}
		}
	}

	// meshes: first primitive per mesh
	const HF_Json::Value *meshes = root.Find( "meshes" );
	if ( meshes && meshes->IsArr() ) {
		out.numMeshes = meshes->Count();
		if ( out.numMeshes > kMaxMeshes ) {
			err = "too many meshes";
			HF_Json::Free( root );
			return -1;
		}
		for ( int im = 0; im < out.numMeshes; im++ ) {
			const HF_Json::Value *m = meshes->Get( im );
			hfGltfMesh &		gm = out.meshes[ im ];
			strncpy( gm.name, HF_Json::GetStr( *m, "name", "mesh" ), sizeof( gm.name ) - 1 );
			gm.firstTri = out.numTris;
			gm.firstVert = out.numVerts;

			const HF_Json::Value *prims = m->Find( "primitives" );
			if ( !prims || !prims->IsArr() || prims->Count() == 0 ) {
				continue;
			}
			const HF_Json::Value *prim = prims->Get( 0 );
			const HF_Json::Value *attrib = prim->Find( "attributes" );
			if ( !attrib ) {
				continue;
			}
			int				posAcc = ( int )HF_Json::GetNum( *attrib, "POSITION", -1.0 );
			int				idxAcc = ( int )HF_Json::GetNum( *prim, "indices", -1.0 );
			if ( posAcc < 0 ) {
				continue;
			}
			Detail::hfAcc &	pa = accs[ posAcc ];
			if ( out.numVerts + pa.count > kMaxVerts ) {
				err = "vertex overflow";
				HF_Json::Free( root );
				return -1;
			}
			Detail::ReadAccessor( pa, bufferData, viewByte, out.verts + out.numVerts * 3 );
			gm.vertCount = pa.count;

			if ( idxAcc >= 0 ) {
				Detail::hfAcc &	ia = accs[ idxAcc ];
				if ( ia.count % 3 ) {
					err = "non-triangle index buffer";
					HF_Json::Free( root );
					return -1;
				}
				if ( out.numTris + ia.count / 3 > kMaxTris ) {
					err = "triangle overflow";
					HF_Json::Free( root );
					return -1;
				}
				int				tmp[ kMaxVerts ];
				if ( !Detail::ReadIndices( ia, bufferData, viewByte, tmp, kMaxVerts ) ) {
					err = "indices read failed";
					HF_Json::Free( root );
					return -1;
				}
				for ( int k = 0; k < ia.count; k++ ) {
					out.tris[ out.numTris * 3 + k ] = out.numVerts + tmp[ k ];
				}
				gm.triCount = ia.count / 3;
				out.numTris += gm.triCount;
			}

			hfBounds	b;
			b.Clear();
			for ( int v = 0; v < pa.count; v++ ) {
				b.AddPoint( hfVec3( out.verts[ ( out.numVerts + v ) * 3 + 0 ], out.verts[ ( out.numVerts + v ) * 3 + 1 ], out.verts[ ( out.numVerts + v ) * 3 + 2 ] ) );
			}
			gm.bounds = b;
			out.numVerts += pa.count;
		}
	}

	// nodes
	const HF_Json::Value *nodes = root.Find( "nodes" );
	if ( nodes && nodes->IsArr() ) {
		out.numNodes = nodes->Count();
		if ( out.numNodes > kMaxNodes ) {
			err = "too many nodes";
			HF_Json::Free( root );
			return -1;
		}
		for ( int i = 0; i < out.numNodes; i++ ) {
			const HF_Json::Value *n = nodes->Get( i );
			hfGltfNode &		g = out.nodes[ i ];
			g.mesh = ( int )HF_Json::GetNum( *n, "mesh", -1.0 );
			strncpy( g.name, HF_Json::GetStr( *n, "name", "node" ), sizeof( g.name ) - 1 );

			hfVec3			t( 0, 0, 0 );
			if ( n->Find( "translation" ) ) {
				t = hfVec3( ( float )HF_Json::AsVecD( *n, "translation", 0, 0.0 ), ( float )HF_Json::AsVecD( *n, "translation", 1, 0.0 ), ( float )HF_Json::AsVecD( *n, "translation", 2, 0.0 ) );
			}
			hfVec3			s( 1, 1, 1 );
			if ( n->Find( "scale" ) ) {
				s = hfVec3( ( float )HF_Json::AsVecD( *n, "scale", 0, 1.0 ), ( float )HF_Json::AsVecD( *n, "scale", 1, 1.0 ), ( float )HF_Json::AsVecD( *n, "scale", 2, 1.0 ) );
			}
			hfQuat			q( 0, 0, 0, 1 );		// identity (hfQuat() leaves members unset)
			if ( n->Find( "rotation" ) ) {
				q = hfQuat( ( float )HF_Json::AsVecD( *n, "rotation", 0, 0.0 ), ( float )HF_Json::AsVecD( *n, "rotation", 1, 0.0 ), ( float )HF_Json::AsVecD( *n, "rotation", 2, 0.0 ), ( float )HF_Json::AsVecD( *n, "rotation", 3, 1.0 ) );
			}

			// local = T * R * S, column-major
			float		m[ 16 ];
			hfMat3		rm = q.ToMat3();
			for ( int c = 0; c < 3; c++ ) {
				for ( int r = 0; r < 3; r++ ) {
					m[ c * 4 + r ] = rm[ c ][ r ] * s[ c ];
				}
				m[ c * 4 + 3 ] = 0.0f;
			}
			m[ 12 ] = t.x; m[ 13 ] = t.y; m[ 14 ] = t.z; m[ 15 ] = 1.0f;
			memcpy( g.local, m, sizeof( m ) );

			const HF_Json::Value *kids = n->Find( "children" );
			if ( kids && kids->IsArr() ) {
				g.numChildren = kids->Count();
				for ( int k = 0; k < g.numChildren && k < 16; k++ ) {
					g.children[ k ] = ( int )kids->AsVec( k, -1.0 );
				}
			}
		}
	}

	HF_Json::Free( root );
	return out.numNodes;
}

// --- content pipeline: glTF -> packed store assets --------------------------

// Render-mesh binary "HFRTRIS1": magic[8], int numVert, int numTri,
// float pos[numVert*3], int tri[numTri*3]  (mesh-local indices)
static const char	kMeshMagic[8]	= { 'H', 'F', 'R', 'T', 'R', 'I', 'S', '1' };

// Assemble the Pack() item list for one glTF import: the render-mesh blob and
// the .mtr text.  meshPath/mtrPath are the store names; baseName is the token
// used inside the .mtr def.  Returns item count written, or -1 on overflow.
HF_INLINE int	BuildAssetItems( const hfGltfDoc &doc, const char *meshPath, const char *mtrPath, const char *baseName, HF_Asset::AssetSource *items, char *mtrBuf, int mtrCap ) {
	int				count = 0;
	if ( doc.numMeshes > 0 ) {
		const hfGltfMesh &	m = doc.meshes[ 0 ];
		int				blobSize = 8 + 4 + 4 + m.vertCount * 3 * 4 + m.triCount * 3 * 4;
		static byte		blob[ kMaxVerts * 3 * 4 + 8 + 8 + kMaxTris * 3 * 4 ];
		int				ofs = 8;
		memcpy( blob, kMeshMagic, 8 );
		memcpy( blob + ofs, &m.vertCount, 4 ); ofs += 4;
		memcpy( blob + ofs, &m.triCount, 4 ); ofs += 4;
		memcpy( blob + ofs, doc.verts + m.firstVert * 3, m.vertCount * 3 * 4 ); ofs += m.vertCount * 3 * 4;
		// mesh-local index relinking
		int *			tris = ( int * )( blob + ofs );
		for ( int t = 0; t < m.triCount * 3; t++ ) {
			tris[ t ] = doc.tris[ m.firstTri * 3 + t ] - m.firstVert;
		}
		items[ count ].name = meshPath;
		items[ count ].data = blob;
		items[ count ].size = ( unsigned int )blobSize;
		count++;
	}
	// .mtr text entry (built manually — hfStr.h remaps snprintf away)
	if ( mtrBuf && mtrCap > 0 ) {
		char *		p = mtrBuf;
		char *		end = mtrBuf + mtrCap - 1;
		#define HF_GLTF_APPEND( s )	do { const char *_s = ( s ); while ( *_s && p < end ) { *p++ = *_s++; } } while (0)
		HF_GLTF_APPEND( "name " ); HF_GLTF_APPEND( baseName );
		HF_GLTF_APPEND( " {\n  stages {\n    map " ); HF_GLTF_APPEND( baseName ); HF_GLTF_APPEND( "_tex\n  }\n}\n" );
		#undef HF_GLTF_APPEND
		*p = 0;
		items[ count ].name = mtrPath;
		items[ count ].data = mtrBuf;
		items[ count ].size = ( unsigned int )( p - mtrBuf ) + 1;		// include NUL terminator
		count++;
	}
	return count;
}

} // namespace HF_Gltf

#endif