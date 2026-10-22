#ifndef __SURFACE_PATCH_H__
#define __SURFACE_PATCH_H__

/*
===============================================================================

	Bezier patch surface.

===============================================================================
*/

class hfSurface_Patch : public hfSurface {

public:
						hfSurface_Patch();
						hfSurface_Patch( int maxPatchWidth, int maxPatchHeight );
						hfSurface_Patch( const hfSurface_Patch &patch );
						~hfSurface_Patch();

	void				SetSize( int patchWidth, int patchHeight );
	int					GetWidth() const;
	int					GetHeight() const;

						// subdivide the patch mesh based on error
	void				Subdivide( float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals = false );
						// subdivide the patch up to an explicit number of horizontal and vertical subdivisions
	void				SubdivideExplicit( int horzSubdivisions, int vertSubdivisions, bool genNormals, bool removeLinear = false );

protected:
	int					width;			// width of patch
	int					height;			// height of patch
	int					maxWidth;		// maximum width allocated for
	int					maxHeight;		// maximum height allocated for
	bool				expanded;		// true if vertices are spaced out

private:
						// put the approximation points on the curve
	void				PutOnCurve();
						// remove columns and rows with all points on one line
	void				RemoveLinearColumnsRows();
						// resize verts buffer
	void				ResizeExpanded( int height, int width );
						// space points out over maxWidth * maxHeight buffer
	void				Expand();
						// move all points to the start of the verts buffer
	void				Collapse();
						// project a point onto a vector to calculate maximum curve error
	void				ProjectPointOntoVector( const hfVec3 &point, const hfVec3 &vStart, const hfVec3 &vEnd, hfVec3 &vProj );
						// generate normals
	void				GenerateNormals();
						// generate triangle indexes
	void				GenerateIndexes();
						// lerp point from two patch point
	void				LerpVert( const hfDrawVert &a, const hfDrawVert &b, hfDrawVert &out ) const;
						// sample a single 3x3 patch
	void				SampleSinglePatchPoint( const hfDrawVert ctrl[3][3], float u, float v, hfDrawVert *out ) const;
	void				SampleSinglePatch( const hfDrawVert ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, hfDrawVert *outVerts ) const;
};

/*
=================
hfSurface_Patch::hfSurface_Patch
=================
*/
HF_INLINE hfSurface_Patch::hfSurface_Patch() {
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
hfSurface_Patch::hfSurface_Patch
=================
*/
HF_INLINE hfSurface_Patch::hfSurface_Patch( int maxPatchWidth, int maxPatchHeight ) {
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

/*
=================
hfSurface_Patch::hfSurface_Patch
=================
*/
HF_INLINE hfSurface_Patch::hfSurface_Patch( const hfSurface_Patch &patch ) {
	(*this) = patch;
}

/*
=================
hfSurface_Patch::~hfSurface_Patch
=================
*/
HF_INLINE hfSurface_Patch::~hfSurface_Patch() {
}

/*
=================
hfSurface_Patch::GetWidth
=================
*/
HF_INLINE int hfSurface_Patch::GetWidth() const {
	return width;
}

/*
=================
hfSurface_Patch::GetHeight
=================
*/
HF_INLINE int hfSurface_Patch::GetHeight() const {
	return height;
}

#endif /* !__SURFACE_PATCH_H__ */

