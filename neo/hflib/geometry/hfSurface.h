#ifndef __SURFACE_H__
#define __SURFACE_H__

/*
===============================================================================

	Surface base class.

	A surface is tesselated to a triangle mesh with each edge shared by
	at most two triangles.

===============================================================================
*/

typedef struct surfaceEdge_s {
	int						verts[2];	// edge vertices always with ( verts[0] < verts[1] )
	int						tris[2];	// edge triangles
} surfaceEdge_t;


class hfSurface {
public:
							hfSurface();
							explicit hfSurface( const hfSurface &surf );
							explicit hfSurface( const hfDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
							~hfSurface();

	const hfDrawVert &		operator[]( const int index ) const;
	hfDrawVert &			operator[]( const int index );
	hfSurface &				operator+=( const hfSurface &surf );

	int						GetNumIndexes() const { return indexes.Num(); }
	const int *				GetIndexes() const { return indexes.Ptr(); }
	int						GetNumVertices() const { return verts.Num(); }
	const hfDrawVert *		GetVertices() const { return verts.Ptr(); }
	const int *				GetEdgeIndexes() const { return edgeIndexes.Ptr(); }
	const surfaceEdge_t *	GetEdges() const { return edges.Ptr(); }

	void					Clear();
	void					TranslateSelf( const hfVec3 &translation );
	void					RotateSelf( const hfMat3 &rotation );

							// splits the surface into a front and back surface, the surface itself stays unchanged
							// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
							// returns a SIDE_?
	int						Split( const hfPlane &plane, const float epsilon, hfSurface **front, hfSurface **back, int *frontOnPlaneEdges = NULL, int *backOnPlaneEdges = NULL ) const;
							// cuts off the part at the back side of the plane, returns true if some part was at the front
							// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const hfPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

							// returns true if each triangle can be reached from any other triangle by a traversal
	bool					IsConnected() const;
							// returns true if the surface is closed
	bool					IsClosed() const;
							// returns true if the surface is a convex hull
	bool					IsPolytope( const float epsilon = 0.1f ) const;

	float					PlaneDistance( const hfPlane &plane ) const;
	int						PlaneSide( const hfPlane &plane, const float epsilon = ON_EPSILON ) const;

							// returns true if the line intersects one of the surface triangles
	bool					LineIntersection( const hfVec3 &start, const hfVec3 &end, bool backFaceCull = false ) const;
							// intersection point is start + dir * scale
	bool					RayIntersection( const hfVec3 &start, const hfVec3 &dir, float &scale, bool backFaceCull = false ) const;

protected:
	hfList<hfDrawVert, TAG_IDLIB_LIST_SURFACE>		verts;			// vertices
	hfList<int, TAG_IDLIB_LIST_SURFACE>				indexes;		// 3 references to vertices for each triangle
	hfList<surfaceEdge_t, TAG_IDLIB_LIST_SURFACE>	edges;			// edges
	hfList<int, TAG_IDLIB_LIST_SURFACE>				edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes();
	int						FindEdge( int v1, int v2 ) const;
};

/*
====================
hfSurface::hfSurface
====================
*/
HF_INLINE hfSurface::hfSurface() {
}

/*
=================
hfSurface::hfSurface
=================
*/
HF_INLINE hfSurface::hfSurface( const hfDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	assert( verts != NULL && indexes != NULL && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	memcpy( this->verts.Ptr(), verts, numVerts * sizeof( verts[0] ) );
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
hfSurface::hfSurface
====================
*/
HF_INLINE hfSurface::hfSurface( const hfSurface &surf ) {
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
hfSurface::~hfSurface
====================
*/
HF_INLINE hfSurface::~hfSurface() {
}

/*
=================
hfSurface::operator[]
=================
*/
HF_INLINE const hfDrawVert &hfSurface::operator[]( const int index ) const {
	return verts[ index ];
};

/*
=================
hfSurface::operator[]
=================
*/
HF_INLINE hfDrawVert &hfSurface::operator[]( const int index ) {
	return verts[ index ];
};

/*
=================
hfSurface::operator+=
=================
*/
HF_INLINE hfSurface &hfSurface::operator+=( const hfSurface &surf ) {
	int i, m, n;
	n = verts.Num();
	m = indexes.Num();
	verts.Append( surf.verts );			// merge verts where possible ?
	indexes.Append( surf.indexes );
	for ( i = m; i < indexes.Num(); i++ ) {
		indexes[i] += n;
	}
	GenerateEdgeIndexes();
	return *this;
}

/*
=================
hfSurface::Clear
=================
*/
HF_INLINE void hfSurface::Clear() {
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
hfSurface::TranslateSelf
=================
*/
HF_INLINE void hfSurface::TranslateSelf( const hfVec3 &translation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz += translation;
	}
}

/*
=================
hfSurface::RotateSelf
=================
*/
HF_INLINE void hfSurface::RotateSelf( const hfMat3 &rotation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz *= rotation;
		verts[i].SetNormal( verts[i].GetNormal() * rotation );
		verts[i].SetTangent( verts[i].GetTangent() * rotation );
	}
}

#endif /* !__SURFACE_H__ */

