#ifndef __SHADOWSHARED_H__
#define __SHADOWSHARED_H__

enum shadowVolumeState_t {
	SHADOWVOLUME_DONE		= 0,
	SHADOWVOLUME_UNFINISHED	= 1
};

// this extra stretch should also make the "inside shadow volume test" valid for both eyes of a stereo view
const float INSIDE_SHADOW_VOLUME_EXTRA_STRETCH = 4.0f;	// in theory, should vary with FOV

#define TEMP_ROUND4( x )				( ( x + 3 ) & ~3 )	// round up to a multiple of 4 for SIMD
#define TEMP_CULLBITS( numVerts )		TEMP_ROUND4( numVerts )

bool R_ViewPotentiallyInsideInfiniteShadowVolume( const hfBounds & occluderBounds, const hfVec3 & localLight, const hfVec3 & localView, const float znear );

bool R_LineIntersectsTriangleExpandedWithSphere( const hfVec3 & lineStart, const hfVec3 & lineEnd, const hfVec3 & lineDir, const float lineLength,
													const float sphereRadius, const hfVec3 & triVert0, const hfVec3 & triVert1, const hfVec3 & triVert2 );
bool R_ViewInsideShadowVolume( byte * cullBits, const hfShadowVert * verts, int numVerts, const triIndex_t * indexes, int numIndexes,
								const hfVec3 & localLightOrigin, const hfVec3 & localViewOrigin, const float zNear );

#endif // !__SHADOWSHARED_H__

