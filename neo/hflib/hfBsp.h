#ifndef __HF_BSP_H__
#define __HF_BSP_H__

#include "hfprecompiled.h"
#include "hfStr.h"
#include "math/hfMath.h"
#include "math/hfVector.h"
#include "containers/hfList.h"
#include "containers/hfStrList.h"

/*
===============================================================================

	hfBsp — reader for compiled idTech3 `.bsp` level files (BSP_VERSION 46).

	A `.bsp` is a lump container: a dheader_t with a fixed table of lump_t
	{ fileofs, filelen } entries, each pointing at an array of fixed size
	records inside the same file. Nothing is compressed and there is no
	per-record magic, so every array has to be validated by dividing its lump
	length by the record size before it is trusted.

	This reader deliberately stops at *structure + brushes*. It validates and
	exposes the lumps, turns brushes into convex solids (planes + a triangle
	fan mesh) and parses the entity lump into key/value pairs. Rendering,
	PVS decompression and lightmap stitching are out of scope; the raw lumps
	are still reachable through the accessors below so a front-end can add
	those later.

	Per the engine convention this header is header-only so the script JIT can
	compile against it mid-air. All multi-byte fields in a .bsp are little
	endian, which matches every platform this engine targets; a big endian host
	would need a byteswap on load.

===============================================================================
*/

// "IBSP" stored little endian, exactly as idTech writes it.
#define HF_BSP_IDENT			( ( 'P' << 24 ) + ( 'S' << 16 ) + ( 'B' << 8 ) + 'I' )
#define HF_BSP_VERSION			46

// the engine's relative path budget; also the on-disk shader/fog name field
#define HF_BSP_MAX_QPATH		64

// lumps, in on-disk order
enum {
	HF_BSP_LUMP_ENTITIES = 0,
	HF_BSP_LUMP_SHADERS,
	HF_BSP_LUMP_PLANES,
	HF_BSP_LUMP_NODES,
	HF_BSP_LUMP_LEAFS,
	HF_BSP_LUMP_LEAFSURFACES,
	HF_BSP_LUMP_LEAFBRUSHES,
	HF_BSP_LUMP_MODELS,
	HF_BSP_LUMP_BRUSHES,
	HF_BSP_LUMP_BRUSHSIDES,
	HF_BSP_LUMP_DRAWVERTS,
	HF_BSP_LUMP_DRAWINDEXES,
	HF_BSP_LUMP_FOGS,
	HF_BSP_LUMP_SURFACES,
	HF_BSP_LUMP_LIGHTMAPS,
	HF_BSP_LUMP_LIGHTGRID,
	HF_BSP_LUMP_VISIBILITY,
	HF_BSP_HEADER_LUMPS
};

// surface geometry classification (mapSurfaceType_t)
enum {
	HF_BSP_SURF_BAD = 0,
	HF_BSP_SURF_PLANAR,
	HF_BSP_SURF_PATCH,
	HF_BSP_SURF_TRIANGLE_SOUP,
	HF_BSP_SURF_FLARE
};

// how far a point may sit outside a brush plane and still count as on it
#define HF_BSP_PLANE_EPS		0.03125f

// format limits, as idTech declares them. These are sanity ceilings: a file
// claiming more than this is treated as corrupt rather than trusted.
#define HF_BSP_MAX_MODELS		0x400
#define HF_BSP_MAX_BRUSHES		0x8000
#define HF_BSP_MAX_ENTITIES		0x800
#define HF_BSP_MAX_SHADERS		0x400
#define HF_BSP_MAX_PLANES		0x20000
#define HF_BSP_MAX_NODES		0x20000
#define HF_BSP_MAX_LEAFS		0x20000

/*
================
on-disk records
================
*/
struct hfBspLump {
	int					fileofs;
	int					filelen;
};

struct hfBspHeader {
	int					ident;
	int					version;
	hfBspLump			lumps[HF_BSP_HEADER_LUMPS];
};

struct hfBspModel {
	float				mins[3];
	float				maxs[3];
	int					firstSurface;
	int					numSurfaces;
	int					firstBrush;
	int					numBrushes;
};

struct hfBspShader {
	char				shader[HF_BSP_MAX_QPATH];
	int					surfaceFlags;
	int					contentFlags;
};

struct hfBspPlaneRec {
	float				normal[3];
	float				dist;
};

struct hfBspNode {
	int					planeNum;
	int					children[2];		// negative values are -(leaf+1), not nodes
	int					mins[3];
	int					maxs[3];
};

struct hfBspLeaf {
	int					cluster;			// -1 marks an opaque leaf
	int					area;
	int					mins[3];
	int					maxs[3];
	int					firstLeafSurface;
	int					numLeafSurfaces;
	int					firstLeafBrush;
	int					numLeafBrushes;
};

struct hfBspBrushSide {
	int					planeNum;			// positive side faces out of the brush
	int					shaderNum;
};

struct hfBspBrushRec {
	int					firstSide;
	int					numSides;
	int					shaderNum;			// decides the contents flags
};

struct hfBspFog {
	char				shader[HF_BSP_MAX_QPATH];
	int					brushNum;
	int					visibleSide;		// side ray tests clip against, -1 == none
};

struct hfBspDrawVert {
	float				xyz[3];
	float				st[2];
	float				lightmap[2];
	float				normal[3];
	unsigned char		color[4];
};

struct hfBspSurface {
	int					shaderNum;
	int					fogNum;
	int					surfaceType;

	int					firstVert;
	int					numVerts;

	int					firstIndex;
	int					numIndexes;

	int					lightmapNum;
	int					lightmapX, lightmapY;
	int					lightmapWidth, lightmapHeight;

	float				lightmapOrigin[3];
	float				lightmapVecs[3][3];	// for patches, [0] and [1] are lodbounds

	int					patchWidth;
	int					patchHeight;
};

/*
================
hfBspPlane

	A brush face as a point + distance. A point is inside the convex solid when
	it is on the negative side of every face plane.
================
*/
class hfBspPlane {
public:
	hfVec3				normal;
	float				dist;

	void				FromRecord( const hfBspPlaneRec &rec ) {
		normal.x = rec.normal[0];
		normal.y = rec.normal[1];
		normal.z = rec.normal[2];
		dist = rec.dist;
	}

	float				Distance( const hfVec3 &p ) const {
		return normal.x * p.x + normal.y * p.y + normal.z * p.z - dist;
	}
};

/*
================
hfBspBrush

	One convex solid. Geometry is produced once, lazily, as a triangle fan
	around the centroid; a convex solid's vertices are exactly the points that
	lie on three or more of its face planes.
================
*/
class hfBspBrush {
public:
	int					firstSide;
	int					numSides;
	int					shaderNum;
	int					modelIndex;			// which dmodel_t owns this brush
	const hfBspPlane *	planes;
	int					numPlanes;

	// solve the three plane equations for their common point with Cramer's
	// rule: ni . p = di. returns false when the three planes have no common
	// point, which includes the very common case of two parallel faces.
	static bool			Intersect3Planes( const hfBspPlane &a, const hfBspPlane &b, const hfBspPlane &c, hfVec3 &out ) {
		const hfVec3 bc = b.normal.Cross( c.normal );
		float det = a.normal * bc;
		if ( hfMath::Fabs( det ) < 1e-6f ) { return false; }
		out = bc * a.dist + c.normal.Cross( a.normal ) * b.dist + a.normal.Cross( b.normal ) * c.dist;
		out *= 1.0f / det;
		return true;
	}

	// a unit vector along whichever axis the brush is thinnest in, so the
	// ring ordering projects onto a well conditioned plane. Projecting a
	// convex solid along its smallest extent is what keeps the projection from
	// collapsing onto a segment.
	static hfVec3		DominantAxis( const hfVec3 &span ) {
		float ax = hfMath::Fabs( span.x );
		float ay = hfMath::Fabs( span.y );
		float az = hfMath::Fabs( span.z );
		hfVec3 a;
		a.Zero();
		if ( ax <= ay && ax <= az ) { a.x = 1.0f; return a; }
		if ( ay <= az ) { a.y = 1.0f; return a; }
		a.z = 1.0f;
		return a;
	}

	// any unit vector perpendicular to n
	static hfVec3		Perpendicular( const hfVec3 &n ) {
		hfVec3 other;
		other.Zero();
		if ( hfMath::Fabs( n.x ) < 0.9f ) { other.x = 1.0f; }
		else { other.y = 1.0f; }
		hfVec3 r = n.Cross( other );
		r.Normalize();
		return r;
	}

	// true when the point is on the inside of every face
	bool				ContainsPoint( const hfVec3 &p, float epsilon = HF_BSP_PLANE_EPS ) const {
		for ( int i = 0; i < numPlanes; i++ ) {
			if ( planes[i].Distance( p ) > epsilon ) {
				return false;
			}
		}
		return true;
	}

	/*
	=========
	BuildHull

	Recovers the corners of the solid. A vertex of a convex polyhedron is where
	three face planes meet, so every corner is found by solving each triple of
	planes and keeping the points that satisfy all of the remaining half spaces.
	(Intersecting only *pairs* is not enough: the point closest to the origin
	on a pair's line lies in the middle of an edge, not on a corner.)

	Returns the corner count. A valid solid always has at least 4.
	=========
	*/
	int					BuildHull( hfList<hfVec3> &hull ) const {
		if ( planes == NULL || numPlanes < 4 ) { return 0; }
		hull.SetGranularity( 16 );

		for ( int i = 0; i < numPlanes; i++ ) {
			for ( int j = i + 1; j < numPlanes; j++ ) {
				for ( int k = j + 1; k < numPlanes; k++ ) {
					hfVec3 p;
					if ( !Intersect3Planes( planes[i], planes[j], planes[k], p ) ) { continue; }

					// the corner has to satisfy every other face, and it has to
					// actually sit on this triple rather than somewhere along it
					int touches = 0;
					bool valid = true;
					for ( int f = 0; f < numPlanes; f++ ) {
						float d = planes[f].Distance( p );
						if ( d > HF_BSP_PLANE_EPS ) { valid = false; break; }
						if ( d > -HF_BSP_PLANE_EPS ) { touches++; }
					}
					if ( !valid || touches < 3 ) { continue; }

					// drop duplicates produced by coplanar or symmetric faces
					bool duplicate = false;
					for ( int v = 0; v < hull.Num(); v++ ) {
						if ( ( hull[v] - p ).LengthSqr() < 1e-6f ) { duplicate = true; break; }
					}
					if ( !duplicate ) { hull.Append( p ); }
				}
			}
		}

		return hull.Num();
	}

	/*
	=========
	BuildMesh

	Turns the convex solid into a triangle fan around its centroid. Because the
	solid is convex, a fan over the corners in angular order is a valid
	tessellation and no ear clipping is needed.

	This is the general-purpose path a loader can afford; a map compiler uses
	the faster winding-clip method because it has to run over every brush in a
	level. Returns the corner count, or 0 if the brush is degenerate.
	=========
	*/
	int					BuildMesh( hfList<hfVec3> &outVerts, hfList<int> &outIndexes ) const {
		hfList<hfVec3> found;
		if ( BuildHull( found ) < 4 ) { return 0; }

		// centroid, used as the fan apex
		hfVec3 center;
		center.Zero();
		for ( int v = 0; v < found.Num(); v++ ) { center += found[v]; }
		center *= ( 1.0f / (float)found.Num() );

		/*
			Order the ring by angle around the axis the brush is thinnest in, so
			the projection onto that axis' plane stays a simple polygon.
		*/
		hfVec3 mins, maxs;
		ComputeBounds( mins, maxs );
		hfVec3 axis = DominantAxis( maxs - mins );
		hfVec3 u = Perpendicular( axis );
		hfVec3 v = axis.Cross( u );
		u.Normalize();
		v.Normalize();

		int *order = new int[ found.Num() ];
		float *angle = new float[ found.Num() ];
		for ( int i = 0; i < found.Num(); i++ ) {
			hfVec3 d = found[i] - center;
			order[i] = i;
			angle[i] = atan2f( d * v, d * u );
		}
		// insertion sort: brush vertex counts are tiny and this keeps it simple
		for ( int i = 1; i < found.Num(); i++ ) {
			int o = order[i];
			float a = angle[i];
			int j = i - 1;
			while ( j >= 0 && angle[j] > a ) {
				order[j + 1] = order[j];
				angle[j + 1] = angle[j];
				j--;
			}
			order[j + 1] = o;
			angle[j + 1] = a;
		}

		int base = outVerts.Num();
		outVerts.Append( center );
		for ( int i = 0; i < found.Num(); i++ ) { outVerts.Append( found[ order[i] ] ); }

		for ( int i = 0; i < found.Num(); i++ ) {
			outIndexes.Append( base );
			outIndexes.Append( base + 1 + i );
			outIndexes.Append( base + 1 + ( ( i + 1 ) % found.Num() ) );
		}

		delete[] order;
		delete[] angle;
		return (int)found.Num();
	}

	/*
	=========
	ComputeBounds

	Exact axis-aligned bounds of the solid. A half space on its own says
	nothing about how far the solid reaches along an axis, so the extremes are
	taken from the corners rather than from the planes.
	=========
	*/
	void				ComputeBounds( hfVec3 &outMins, hfVec3 &outMaxs ) const {
		outMins.Zero();
		outMaxs.Zero();

		hfList<hfVec3> hull;
		if ( BuildHull( hull ) < 4 ) { return; }

		bool first = true;
		for ( int v = 0; v < hull.Num(); v++ ) {
			for ( int c = 0; c < 3; c++ ) {
				const float f = ( &hull[v].x )[c];
				if ( first ) {
					( &outMins.x )[c] = f;
					( &outMaxs.x )[c] = f;
					continue;
				}
				if ( f < ( &outMins.x )[c] ) { ( &outMins.x )[c] = f; }
				if ( f > ( &outMaxs.x )[c] ) { ( &outMaxs.x )[c] = f; }
			}
			first = false;
		}
	}
};

/*
================
hfBspEntity

	One entity from the entities lump, stored as the usual idTech key/value
	pairs. The first entity is always worldspawn.
================
*/
class hfBspEntity {
public:
	hfStrList			keys;
	hfStrList			values;

	int					Num() const { return keys.Num(); }
	const char *		Key( int i ) const { return keys[i].c_str(); }
	const char *		Value( int i ) const { return values[i].c_str(); }

	bool				Get( const char *key, hfStr &out ) const {
		for ( int i = 0; i < keys.Num(); i++ ) {
			if ( keys[i].Cmp( key ) == 0 ) {
				out = values[i];
				return true;
			}
		}
		return false;
	}
	const char *		GetString( const char *key ) const {
		for ( int i = 0; i < keys.Num(); i++ ) {
			if ( keys[i].Cmp( key ) == 0 ) {
				return values[i].c_str();
			}
		}
		return NULL;
	}
};

/*
================
hfBspFile

	Owns nothing but the caller's buffer, which must outlive the reader.
================
*/
class hfBspFile {
public:
						hfBspFile() { Zero(); }
						~hfBspFile() { Clear(); }

	// reason Load() rejected a file
	enum {
		HF_BSP_OK = 0,
		HF_BSP_ERR_ARGUMENTS,
		HF_BSP_ERR_IDENT,
		HF_BSP_ERR_VERSION,
		HF_BSP_ERR_TRUNCATED_HEADER,
		HF_BSP_ERR_LUMP_BOUNDS,
		HF_BSP_ERR_LUMP_SIZE,
		HF_BSP_ERR_COUNT
	};

	static const char *	ErrorString( int code ) {
		switch ( code ) {
			case HF_BSP_OK:						return "ok";
			case HF_BSP_ERR_ARGUMENTS:			return "null or empty buffer";
			case HF_BSP_ERR_IDENT:				return "not a bsp file (bad ident)";
			case HF_BSP_ERR_VERSION:			return "unsupported bsp version";
			case HF_BSP_ERR_TRUNCATED_HEADER:	return "file shorter than the header";
			case HF_BSP_ERR_LUMP_BOUNDS:		return "lump points outside the file";
			case HF_BSP_ERR_LUMP_SIZE:			return "lump length is not a whole number of records";
			default:							return "unknown error";
		}
	}

	/*
	=========
	Load

	Validates the container and indexes every lump. The brush convex solids are
	built here; meshes stay lazy because most callers only want collision or
	the entity list.
	=========
	*/
	int Load( const void *buffer, int size ) {
		Clear();

		if ( buffer == NULL || size <= 0 ) {
			return HF_BSP_ERR_ARGUMENTS;
		}
		if ( size < (int)sizeof( hfBspHeader ) ) {
			return HF_BSP_ERR_TRUNCATED_HEADER;
		}

		data = (const unsigned char *)buffer;
		dataSize = size;
		memcpy( &header, data, sizeof( header ) );

		if ( header.ident != HF_BSP_IDENT ) {
			return HF_BSP_ERR_IDENT;
		}
		if ( header.version != HF_BSP_VERSION ) {
			return HF_BSP_ERR_VERSION;
		}

		// every lump must lie inside the file and describe whole records
		for ( int i = 0; i < HF_BSP_HEADER_LUMPS; i++ ) {
			const hfBspLump &l = header.lumps[i];
			if ( l.filelen < 0 || l.fileofs < 0 ||
				 l.fileofs > size || l.filelen > size - l.fileofs ) {
				return HF_BSP_ERR_LUMP_BOUNDS;
			}
		}
		if ( !LumpFits( HF_BSP_LUMP_ENTITIES, 1 ) ||
			 !LumpFits( HF_BSP_LUMP_SHADERS, sizeof( hfBspShader ) ) ||
			 !LumpFits( HF_BSP_LUMP_PLANES, sizeof( hfBspPlaneRec ) ) ||
			 !LumpFits( HF_BSP_LUMP_NODES, sizeof( hfBspNode ) ) ||
			 !LumpFits( HF_BSP_LUMP_LEAFS, sizeof( hfBspLeaf ) ) ||
			 !LumpFits( HF_BSP_LUMP_LEAFSURFACES, sizeof( int ) ) ||
			 !LumpFits( HF_BSP_LUMP_LEAFBRUSHES, sizeof( int ) ) ||
			 !LumpFits( HF_BSP_LUMP_MODELS, sizeof( hfBspModel ) ) ||
			 !LumpFits( HF_BSP_LUMP_BRUSHES, sizeof( hfBspBrushRec ) ) ||
			 !LumpFits( HF_BSP_LUMP_BRUSHSIDES, sizeof( hfBspBrushSide ) ) ||
			 !LumpFits( HF_BSP_LUMP_DRAWVERTS, sizeof( hfBspDrawVert ) ) ||
			 !LumpFits( HF_BSP_LUMP_DRAWINDEXES, sizeof( int ) ) ||
			 !LumpFits( HF_BSP_LUMP_FOGS, sizeof( hfBspFog ) ) ||
			 !LumpFits( HF_BSP_LUMP_SURFACES, sizeof( hfBspSurface ) ) ) {
			return HF_BSP_ERR_LUMP_SIZE;
		}

		// the lightmap / lightgrid / visibility lumps are raw byte blobs, so
		// there is no record size to check for them

		models      = (const hfBspModel *)Lump( HF_BSP_LUMP_MODELS );
		numModels   = LumpCount( HF_BSP_LUMP_MODELS, sizeof( hfBspModel ) );
		shaders     = (const hfBspShader *)Lump( HF_BSP_LUMP_SHADERS );
		numShaders  = LumpCount( HF_BSP_LUMP_SHADERS, sizeof( hfBspShader ) );
		planes      = (const hfBspPlaneRec *)Lump( HF_BSP_LUMP_PLANES );
		numPlanes   = LumpCount( HF_BSP_LUMP_PLANES, sizeof( hfBspPlaneRec ) );
		nodes       = (const hfBspNode *)Lump( HF_BSP_LUMP_NODES );
		numNodes    = LumpCount( HF_BSP_LUMP_NODES, sizeof( hfBspNode ) );
		leafs       = (const hfBspLeaf *)Lump( HF_BSP_LUMP_LEAFS );
		numLeafs    = LumpCount( HF_BSP_LUMP_LEAFS, sizeof( hfBspLeaf ) );
		leafSurfaces= (const int *)Lump( HF_BSP_LUMP_LEAFSURFACES );
		numLeafSurfaces = LumpCount( HF_BSP_LUMP_LEAFSURFACES, sizeof( int ) );
		leafBrushes = (const int *)Lump( HF_BSP_LUMP_LEAFBRUSHES );
		numLeafBrushes  = LumpCount( HF_BSP_LUMP_LEAFBRUSHES, sizeof( int ) );
		brushRecs   = (const hfBspBrushRec *)Lump( HF_BSP_LUMP_BRUSHES );
		numBrushRecs= LumpCount( HF_BSP_LUMP_BRUSHES, sizeof( hfBspBrushRec ) );
		brushSides  = (const hfBspBrushSide *)Lump( HF_BSP_LUMP_BRUSHSIDES );
		numBrushSides = LumpCount( HF_BSP_LUMP_BRUSHSIDES, sizeof( hfBspBrushSide ) );
		drawVerts   = (const hfBspDrawVert *)Lump( HF_BSP_LUMP_DRAWVERTS );
		numDrawVerts= LumpCount( HF_BSP_LUMP_DRAWVERTS, sizeof( hfBspDrawVert ) );
		drawIndexes = (const int *)Lump( HF_BSP_LUMP_DRAWINDEXES );
		numDrawIndexes = LumpCount( HF_BSP_LUMP_DRAWINDEXES, sizeof( int ) );
		fogs        = (const hfBspFog *)Lump( HF_BSP_LUMP_FOGS );
		numFogs     = LumpCount( HF_BSP_LUMP_FOGS, sizeof( hfBspFog ) );
		surfaces    = (const hfBspSurface *)Lump( HF_BSP_LUMP_SURFACES );
		numSurfaces = LumpCount( HF_BSP_LUMP_SURFACES, sizeof( hfBspSurface ) );
		lightmaps   = (const unsigned char *)Lump( HF_BSP_LUMP_LIGHTMAPS );
		lightmapSize= header.lumps[HF_BSP_LUMP_LIGHTMAPS].filelen;
		lightGrid   = (const unsigned char *)Lump( HF_BSP_LUMP_LIGHTGRID );
		lightGridSize = header.lumps[HF_BSP_LUMP_LIGHTGRID].filelen;
		visibility  = (const unsigned char *)Lump( HF_BSP_LUMP_VISIBILITY );
		visibilitySize = header.lumps[HF_BSP_LUMP_VISIBILITY].filelen;

		if ( !BuildBrushes() ) {
			Clear();
			return HF_BSP_ERR_LUMP_BOUNDS;
		}
		if ( !BuildEntities() ) {
			Clear();
			return HF_BSP_ERR_LUMP_BOUNDS;
		}

		return HF_BSP_OK;
	}

	void				Clear() {
		// only valid once the object is at least Zero()ed, which the
		// constructor does: reading these counts before that would walk a
		// pointer that was never assigned
		for ( int i = 0; i < numEntities; i++ ) { entities[i].keys.Clear(); entities[i].values.Clear(); }
		delete[] brushPlanes;
		delete[] brushes;
		delete[] entities;
		Zero();
	}

	// ---- raw access ----
	const hfBspHeader &	GetHeader() const { return header; }
	const void *			Lump( int which ) const {
		const hfBspLump &l = header.lumps[which];
		return l.filelen > 0 ? (const void *)( data + l.fileofs ) : NULL;
	}
	int					LumpLength( int which ) const { return header.lumps[which].filelen; }
	int					LumpCount( int which, int recordSize ) const {
		int len = header.lumps[which].filelen;
		return recordSize > 0 ? len / recordSize : 0;
	}
	const char *			ShaderName( int i ) const {
		if ( i < 0 || i >= numShaders ) { return ""; }
		return shaders[i].shader;
	}

	// ---- entities ----
	int					GetNumEntities() const { return numEntities; }
	const hfBspEntity &	GetEntity( int i ) const {
		static hfBspEntity empty;
		if ( i < 0 || i >= numEntities ) { return empty; }
		return entities[i];
	}

	// ---- world geometry ----
	int					GetNumModels() const { return numModels; }
	const hfBspModel &	GetModel( int i ) const {
		static hfBspModel empty;
		if ( i < 0 || i >= numModels ) { memset( &empty, 0, sizeof( empty ) ); return empty; }
		return models[i];
	}
	int					GetNumBrushes() const { return numBrushes; }
	const hfBspBrush &	GetBrush( int i ) const {
		static hfBspBrush empty;
		if ( i < 0 || i >= numBrushes ) { return empty; }
		return brushes[i];
	}
	int					GetNumShaders() const { return numShaders; }
	int					GetNumPlanes() const { return numPlanes; }
	const hfBspPlaneRec &	GetPlane( int i ) const {
		static hfBspPlaneRec empty;
		if ( i < 0 || i >= numPlanes ) { memset( &empty, 0, sizeof( empty ) ); return empty; }
		return planes[i];
	}
	int					GetNumFogs() const { return numFogs; }
	int					GetNumDrawVerts() const { return numDrawVerts; }
	int					GetNumDrawIndexes() const { return numDrawIndexes; }
	int					GetNumSurfaces() const { return numSurfaces; }
	const hfBspSurface &	GetSurface( int i ) const {
		static hfBspSurface empty;
		if ( i < 0 || i >= numSurfaces ) { return empty; }
		return surfaces[i];
	}

	// ---- traversal ----
	int					GetNumLeafs() const { return numLeafs; }
	const hfBspLeaf &	GetLeaf( int i ) const {
		static hfBspLeaf empty;
		if ( i < 0 || i >= numLeafs ) { return empty; }
		return leafs[i];
	}
	int					GetNumNodes() const { return numNodes; }
	const hfBspNode &	GetNode( int i ) const {
		static hfBspNode empty;
		if ( i < 0 || i >= numNodes ) { return empty; }
		return nodes[i];
	}

	// ---- vertex / index access for a surface ----
	int					SurfaceVertCount( const hfBspSurface &s ) const {
		if ( s.firstVert < 0 || s.numVerts <= 0 ) { return 0; }
		if ( s.firstVert + s.numVerts > numDrawVerts ) { return 0; }
		return s.numVerts;
	}
	const hfBspDrawVert &	SurfaceVert( const hfBspSurface &s, int i ) const {
		return drawVerts[ s.firstVert + i ];
	}
	int					SurfaceIndexCount( const hfBspSurface &s ) const {
		if ( s.firstIndex < 0 || s.numIndexes <= 0 ) { return 0; }
		if ( s.firstIndex + s.numIndexes > numDrawIndexes ) { return 0; }
		return s.numIndexes;
	}
	int					SurfaceIndex( const hfBspSurface &s, int i ) const {
		return drawIndexes[ s.firstIndex + i ];
	}

private:
	const unsigned char *	data;
	int					dataSize;
	hfBspHeader			header;

	const hfBspModel *	models;	int numModels;
	const hfBspShader *	shaders;	int numShaders;
	const hfBspPlaneRec *	planes;	int numPlanes;
	const hfBspNode *		nodes;		int numNodes;
	const hfBspLeaf *		leafs;		int numLeafs;
	const int *			leafSurfaces;	int numLeafSurfaces;
	const int *			leafBrushes;	int numLeafBrushes;
	const hfBspBrushRec *	brushRecs;	int numBrushRecs;
	const hfBspBrushSide *	brushSides;	int numBrushSides;
	const hfBspDrawVert *	drawVerts;	int numDrawVerts;
	const int *			drawIndexes;	int numDrawIndexes;
	const hfBspFog *		fogs;		int numFogs;
	const hfBspSurface *	surfaces;	int numSurfaces;
	const unsigned char *	lightmaps;	int lightmapSize;
	const unsigned char *	lightGrid;	int lightGridSize;
	const unsigned char *	visibility;	int visibilitySize;

	hfBspBrush *		brushes;	int numBrushes;
	hfBspEntity *		entities;	int numEntities;
	// storage backing every brush's plane array
	hfBspPlane *		brushPlanes;	int numBrushPlanes;

	bool				LumpFits( int which, int recordSize ) const {
		int len = header.lumps[which].filelen;
		return recordSize <= 0 || ( len % recordSize ) == 0;
	}

	static bool			IsSpace( const char c ) {
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}
	static void			SkipSpace( const char *text, int len, int &pos ) {
		while ( pos < len && IsSpace( text[pos] ) ) { pos++; }
	}

	/*
	=========
	ReadQuoted

	Reads a quoted string starting just after the opening quote and returns
	the offset of the first byte after the closing one. A value may contain an
	escaped quote, so the scan tracks the backslash rather than just looking for
	the next `\"`. Only `\"` is unescaped; every other backslash is kept as
	written, which is what the map compiler means by `"line one\nline two"`.
	=========
	*/
	static int			ReadQuoted( const char *text, int len, int pos, hfStr &out ) {
		while ( pos < len && text[pos] != '\"' ) {
			if ( text[pos] == '\\' && pos + 1 < len ) {
				if ( text[ pos + 1 ] == '\"' ) {
					out += '\"';
					pos += 2;
					continue;
				}
				out += '\\';
				pos++;
				continue;
			}
			out += text[pos];
			pos++;
		}
		return ( pos < len ) ? pos + 1 : pos;
	}

	// A '}' ends the entity currently being filled. An entity that collected
	// no pairs is not a real entity, so it is dropped rather than counted.
	static void			CloseEntity( hfBspEntity *entities, int &numEntities, int cap ) {
		if ( numEntities < cap && entities[numEntities].Num() > 0 ) {
			numEntities++;
		}
	}

	// Puts every scalar and pointer into a defined empty state without
	// touching any heap block, so it is safe on a brand new object.
	void				Zero() {		data = NULL;
		dataSize = 0;
		memset( &header, 0, sizeof( header ) );

		models = NULL; numModels = 0;
		shaders = NULL; numShaders = 0;
		planes = NULL; numPlanes = 0;
		nodes = NULL; numNodes = 0;
		leafs = NULL; numLeafs = 0;
		leafSurfaces = NULL; numLeafSurfaces = 0;
		leafBrushes = NULL; numLeafBrushes = 0;
		brushRecs = NULL; numBrushRecs = 0;
		brushSides = NULL; numBrushSides = 0;
		drawVerts = NULL; numDrawVerts = 0;
		drawIndexes = NULL; numDrawIndexes = 0;
		fogs = NULL; numFogs = 0;
		surfaces = NULL; numSurfaces = 0;
		lightmaps = NULL; lightmapSize = 0;
		lightGrid = NULL; lightGridSize = 0;
		visibility = NULL; visibilitySize = 0;

		brushes = NULL; numBrushes = 0;
		entities = NULL; numEntities = 0;
		brushPlanes = NULL; numBrushPlanes = 0;
	}

	/*
	=========
	BuildBrushes

	Turns each dbrush_t plus its sides into a convex solid, and hands every
	side's plane to a shared plane pool so a brush can point straight at it.
	=========
	*/
	bool				BuildBrushes() {
		delete[] brushPlanes;
		delete[] brushes;
		brushPlanes = NULL;
		brushes = NULL;
		numBrushPlanes = 0;
		numBrushes = 0;

		if ( numBrushRecs <= 0 ) { return true; }
		if ( numBrushRecs > HF_BSP_MAX_BRUSHES ) { return false; }

		// pass 1: total up the sides so both pools can be sized exactly, and
		// count the brushes that survive validation
		int totalSides = 0;
		int *sideCounts = new int[ numBrushRecs ];
		for ( int i = 0; i < numBrushRecs; i++ ) {
			const hfBspBrushRec &rec = brushRecs[i];
			int n = 0;
			if ( rec.firstSide >= 0 && rec.numSides > 0 &&
				 rec.firstSide + rec.numSides <= numBrushSides ) {
				n = rec.numSides;
				// every side must reference a real plane
				for ( int s = 0; s < n; s++ ) {
					int p = brushSides[ rec.firstSide + s ].planeNum;
					if ( p < 0 || p >= numPlanes ) { n = 0; break; }
				}
			}
			sideCounts[i] = n;
			totalSides += n;
		}
		if ( totalSides <= 0 ) {
			delete[] sideCounts;
			numBrushRecs = 0;
			return true;
		}

		brushPlanes = new hfBspPlane[ totalSides ];
		brushes = new hfBspBrush[ numBrushRecs ];
		memset( brushes, 0, numBrushRecs * sizeof( hfBspBrush ) );

		// pass 2: fill the plane pool back to back, then point each brush at
		// its own slice. Nothing reallocates, so the pointers stay valid.
		numBrushPlanes = 0;
		numBrushes = 0;
		for ( int i = 0; i < numBrushRecs; i++ ) {
			if ( sideCounts[i] <= 0 ) { continue; }
			const hfBspBrushRec &rec = brushRecs[i];

			for ( int s = 0; s < rec.numSides; s++ ) {
				brushPlanes[ numBrushPlanes ].FromRecord( planes[ brushSides[ rec.firstSide + s ].planeNum ] );
				numBrushPlanes++;
			}

			hfBspBrush &b = brushes[ numBrushes++ ];
			b.firstSide = rec.firstSide;
			b.numSides = rec.numSides;
			b.shaderNum = rec.shaderNum;
			b.modelIndex = 0;
			b.planes = &brushPlanes[ numBrushPlanes - rec.numSides ];
			b.numPlanes = rec.numSides;
		}
		delete[] sideCounts;

		// associate each brush with the model that lists it
		for ( int m = 0; m < numModels; m++ ) {
			const hfBspModel &model = models[m];
			if ( model.firstBrush < 0 || model.numBrushes <= 0 ) { continue; }
			if ( model.firstBrush + model.numBrushes > numBrushes ) { continue; }
			for ( int b = 0; b < model.numBrushes; b++ ) {
				brushes[ model.firstBrush + b ].modelIndex = m;
			}
		}

		return true;
	}

	/*
	=========
	BuildEntities

	The entities lump is a run of `"key" "value"` pairs; a bare `"}"` closes
	an entity. Values may contain escaped quotes, so the scan tracks quoting
	rather than splitting on whitespace.
	=========
	*/
	bool				BuildEntities() {
		delete[] entities;
		entities = NULL;
		numEntities = 0;

		const char *text = (const char *)Lump( HF_BSP_LUMP_ENTITIES );
		if ( text == NULL ) { return true; }
		int len = header.lumps[HF_BSP_LUMP_ENTITIES].filelen;
		if ( len <= 0 ) { return true; }

		// one entity per '}' at worst, plus one for a trailing unterminated one
		// new[] default-constructs each entity, which is what sets up the
		// hfStrList sentinels; memset would leave them null
		entities = new hfBspEntity[ len / 2 + 2 ];

		int pos = 0;
		int cap = len / 2 + 2;

		while ( pos < len ) {
			SkipSpace( text, len, pos );
			if ( pos >= len ) { break; }

			// The braces idTech3 writes into this lump are *not* quoted:
			// WriteEntities emits `}\n{\n` between entities and a trailing
			// `}\n`, with no brace at all before the first one. Both are
			// accepted quoted or bare, since a hand written lump may differ.
			if ( text[pos] == '{' ) {
				pos++;
				continue;
			}
			if ( text[pos] == '}' ) {
				pos++;
				CloseEntity( entities, numEntities, cap );
				continue;
			}

			// read one quoted token
			if ( text[pos] != '\"' ) {
				pos++;			// unexpected byte, resync on the next token
				continue;
			}
			hfStr token;
			pos = ReadQuoted( text, len, pos + 1, token );

			if ( token == "}" ) {
				CloseEntity( entities, numEntities, cap );
				continue;
			}

			// a key is always followed by its value, with any amount of white
			// space (including a newline) in between
			SkipSpace( text, len, pos );
			if ( pos >= len || text[pos] != '\"' ) {
				continue;			// a lone token is not a pair, skip it
			}

			hfStr value;
			pos = ReadQuoted( text, len, pos + 1, value );

			if ( numEntities >= cap ) { return false; }
			hfBspEntity &e = entities[numEntities];
			e.keys.Append( token );
			e.values.Append( value );
		}

		// an entity that was never closed still counts if it has pairs
		if ( numEntities < cap && entities[numEntities].Num() > 0 ) {
			numEntities++;
		}

		// release the unused tail so the destructors do not run twice later
		if ( numEntities < cap ) {
			for ( int i = numEntities; i < cap; i++ ) {
				entities[i].keys.Clear();
				entities[i].values.Clear();
			}
		}

		if ( numEntities > HF_BSP_MAX_ENTITIES ) { return false; }
		return true;
	}
};

#endif
