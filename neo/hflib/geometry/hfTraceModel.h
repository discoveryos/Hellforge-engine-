#ifndef __TRACEMODEL_H__
#define __TRACEMODEL_H__

/*
===============================================================================

	A trace model is an arbitrary polygonal model which is used by the
	collision detection system to find collisions, contacts or the contents
	of a volume. For collision detection speed reasons the number of vertices
	and edges are limited. The trace model can have any shape. However convex
	models are usually preferred.

===============================================================================
*/

class hfVec3;
class hfMat3;
class hfBounds;

// trace model type
typedef enum {
	TRM_INVALID,		// invalid trm
	TRM_BOX,			// box
	TRM_OCTAHEDRON,		// octahedron
	TRM_DODECAHEDRON,	// dodecahedron
	TRM_CYLINDER,		// cylinder approximation
	TRM_CONE,			// cone approximation
	TRM_BONE,			// two tetrahedrons attached to each other
	TRM_POLYGON,		// arbitrary convex polygon
	TRM_POLYGONVOLUME,	// volume for arbitrary convex polygon
	TRM_CUSTOM			// loaded from map model or ASE/LWO
} traceModel_t;

// these are bit cache limits
#define MAX_TRACEMODEL_VERTS		32
#define MAX_TRACEMODEL_EDGES		32
#define MAX_TRACEMODEL_POLYS		16
#define MAX_TRACEMODEL_POLYEDGES	16

typedef hfVec3 traceModelVert_t;

typedef struct {
	int					v[2];
	hfVec3				normal;
} traceModelEdge_t;

typedef struct {
	hfVec3				normal;
	float				dist;
	hfBounds			bounds;
	int					numEdges;
	int					edges[MAX_TRACEMODEL_POLYEDGES];
} traceModelPoly_t;

class hfTraceModel {

public:
	traceModel_t		type;
	int					numVerts;
	traceModelVert_t	verts[MAX_TRACEMODEL_VERTS];
	int					numEdges;
	traceModelEdge_t	edges[MAX_TRACEMODEL_EDGES+1];
	int					numPolys;
	traceModelPoly_t	polys[MAX_TRACEMODEL_POLYS];
	hfVec3				offset;			// offset to center of model
	hfBounds			bounds;			// bounds of model
	bool				isConvex;		// true when model is convex

public:
						hfTraceModel();
						// axial bounding box
						hfTraceModel( const hfBounds &boxBounds );
						// cylinder approximation
						hfTraceModel( const hfBounds &cylBounds, const int numSides );
						// bone
						hfTraceModel( const float length, const float width );

						// axial box
	void				SetupBox( const hfBounds &boxBounds );
	void				SetupBox( const float size );
						// octahedron
	void				SetupOctahedron( const hfBounds &octBounds );
	void				SetupOctahedron( const float size );
						// dodecahedron
	void				SetupDodecahedron( const hfBounds &dodBounds );
	void				SetupDodecahedron( const float size );
						// cylinder approximation
	void				SetupCylinder( const hfBounds &cylBounds, const int numSides );
	void				SetupCylinder( const float height, const float width, const int numSides );
						// cone approximation
	void				SetupCone( const hfBounds &coneBounds, const int numSides );
	void				SetupCone( const float height, const float width, const int numSides );
						// two tetrahedrons attached to each other
	void				SetupBone( const float length, const float width );
						// arbitrary convex polygon
	void				SetupPolygon( const hfVec3 *v, const int count );
	void				SetupPolygon( const hfWinding &w );
						// generate edge normals
	int					GenerateEdgeNormals();
						// translate the trm
	void				Translate( const hfVec3 &translation );
						// rotate the trm
	void				Rotate( const hfMat3 &rotation );
						// shrink the model m units on all sides
	void				Shrink( const float m );
						// compare
	bool				Compare( const hfTraceModel &trm ) const;
	bool				operator==(	const hfTraceModel &trm ) const;
	bool				operator!=(	const hfTraceModel &trm ) const;
						// get the area of one of the polygons
	float				GetPolygonArea( int polyNum ) const;
						// get the silhouette edges
	int					GetProjectionSilhouetteEdges( const hfVec3 &projectionOrigin, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
	int					GetParallelProjectionSilhouetteEdges( const hfVec3 &projectionDir, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
						// calculate mass properties assuming an uniform density
	void				GetMassProperties( const float density, float &mass, hfVec3 &centerOfMass, hfMat3 &inertiaTensor ) const;

private:
	void				InitBox();
	void				InitOctahedron();
	void				InitDodecahedron();
	void				InitBone();

	void				ProjectionIntegrals( int polyNum, int a, int b, struct projectionIntegrals_s &integrals ) const;
	void				PolygonIntegrals( int polyNum, int a, int b, int c, struct polygonIntegrals_s &integrals ) const;
	void				VolumeIntegrals( struct volumeIntegrals_s &integrals ) const;
	void				VolumeFromPolygon( hfTraceModel &trm, float thickness ) const;
	int					GetOrderedSilhouetteEdges( const int edgeIsSilEdge[MAX_TRACEMODEL_EDGES+1], int silEdges[MAX_TRACEMODEL_EDGES] ) const;
};


HF_INLINE hfTraceModel::hfTraceModel() {
	type = TRM_INVALID;
	numVerts = numEdges = numPolys = 0;
	bounds.Zero();
}

HF_INLINE hfTraceModel::hfTraceModel( const hfBounds &boxBounds ) {
	InitBox();
	SetupBox( boxBounds );
}

HF_INLINE hfTraceModel::hfTraceModel( const hfBounds &cylBounds, const int numSides ) {
	SetupCylinder( cylBounds, numSides );
}

HF_INLINE hfTraceModel::hfTraceModel( const float length, const float width ) {
	InitBone();
	SetupBone( length, width );
}

HF_INLINE bool hfTraceModel::operator==( const hfTraceModel &trm ) const {
	return Compare( trm );
}

HF_INLINE bool hfTraceModel::operator!=( const hfTraceModel &trm ) const {
	return !Compare( trm );
}

#endif /* !__TRACEMODEL_H__ */


