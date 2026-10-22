#ifndef	HF_SKELETON_COMPONENT_H
#define	HF_SKELETON_COMPONENT_H
#include "../../hflib/hfEcs.h"

const int	HF_SKELETON_MAX_BONES	= 8;

struct hfAnimBone	{ const char *name; int parent; };
struct hfBonePose	{ hfQuat rotation; hfVec3 position; };

// A skeleton is a small array of bones (name + parent index) plus, per bone:
// a bind (rest) pose, a working (local) pose, the composed world pose, and the
// matrices a skinning pass consumes. Bones must be added parent-first, so
// index order is also evaluation order — the same assumption the forward
// kinematics pass below relies on.
struct hfSkeletonComponent {
	HF_INLINE hfSkeletonComponent() : boneCount( 0 ) {
		int i;
		for ( i = 0; i < HF_SKELETON_MAX_BONES; i++ ) {
			restPose[ i ].rotation = hfQuat( 0, 0, 0, 1 );
			restPose[ i ].position = hfVec3( 0, 0, 0 );
			localPose[ i ].rotation = hfQuat( 0, 0, 0, 1 );
			localPose[ i ].position = hfVec3( 0, 0, 0 );
			worldPose[ i ].rotation = hfQuat( 0, 0, 0, 1 );
			worldPose[ i ].position = hfVec3( 0, 0, 0 );
			worldMatrix[ i ].Identity();
			restWorldMatrix[ i ].Identity();
			skinMatrix[ i ].Identity();
		}
	}
	HF_INLINE int	AddBone( const char *n, int parent ) {
		if ( boneCount >= HF_SKELETON_MAX_BONES ) return -1;
		if ( parent >= boneCount ) return -1;		// parents must come first
		hfAnimBone &b = bones[ boneCount ];
		b.name = n; b.parent = parent;
		restPose[ boneCount ].rotation = hfQuat( 0, 0, 0, 1 );
		restPose[ boneCount ].position = hfVec3( 0, 0, 0 );
		localPose[ boneCount ].rotation = hfQuat( 0, 0, 0, 1 );
		localPose[ boneCount ].position = hfVec3( 0, 0, 0 );
		return boneCount++;
	}
	HF_INLINE int	FindBone( const char *n ) const {
		int i;
		for ( i = 0; i < boneCount; i++ )
			if ( hfStr::Cmp( bones[ i ].name, n ) == 0 ) return i;
		return -1;
	}
	HF_INLINE int	GetBoneCountHF() const { return boneCount; }
	HF_INLINE int	GetBoneCount() const { return boneCount; }
	HF_INLINE int	GetParent( int b ) const {
		return ( b >= 0 && b < boneCount ) ? bones[ b ].parent : -1;
	}

	// Bind pose: the pose the mesh was authored in. static-pose skinning
	// (skinMatrix == identity) is exactly localPose == restPose.
	HF_INLINE void	SetRestPose( int b, const hfQuat &r, const hfVec3 &p ) {
		if ( b < 0 || b >= boneCount ) return;
		restPose[ b ].rotation = r;
		restPose[ b ].position = p;
	}
	HF_INLINE void	SetRestPose( int b, const hfBonePose &pose ) {
		SetRestPose( b, pose.rotation, pose.position );
	}
	HF_INLINE void	SetLocalPose( int b, const hfQuat &r, const hfVec3 &p ) {
		if ( b < 0 || b >= boneCount ) return;
		localPose[ b ].rotation = r;
		localPose[ b ].position = p;
	}
	// Put the working pose back on the bind pose. With localPose == restPose
	// the inverse-bind cancels exactly and every skinMatrix is the identity —
	// the invariant the harness checks.
	HF_INLINE void	ResetToRest() {
		int i;
		for ( i = 0; i < boneCount; i++ )
			localPose[ i ] = restPose[ i ];
	}

	// Forward kinematics: compose localPose down the chain into worldPose and
	// worldMatrix. Because parents always have a smaller index, one pass is
	// enough and no recursion or visited-set is required.
	HF_INLINE void	ComposeWorld() {
		int i;
		for ( i = 0; i < boneCount; i++ ) {
			int p = bones[ i ].parent;
			if ( p < 0 ) {
				worldPose[ i ] = localPose[ i ];
			} else {
				worldPose[ i ].rotation = worldPose[ p ].rotation * localPose[ i ].rotation;
				worldPose[ i ].position = worldPose[ p ].position +
					worldPose[ p ].rotation * localPose[ i ].position;
			}
			worldMatrix[ i ] = hfMat4( worldPose[ i ].rotation.ToMat3(), worldPose[ i ].position );
		}
	}

	// Inverse-bind skinning: skinMatrix = worldMatrix * inverse(restWorldMatrix),
	// the transform a vertex shader multiplies a bind-space vertex by. At rest
	// it is the identity for every bone, which is the property the harness
	// checks first.
	HF_INLINE void	ComputeSkinMatrices() {
		hfBonePose	rw[ HF_SKELETON_MAX_BONES ];
		int i;
		for ( i = 0; i < boneCount; i++ ) {
			int p = bones[ i ].parent;
			if ( p < 0 ) {
				rw[ i ] = restPose[ i ];
			} else {
				rw[ i ].rotation = rw[ p ].rotation * restPose[ i ].rotation;
				rw[ i ].position = rw[ p ].position + rw[ p ].rotation * restPose[ i ].position;
			}
			restWorldMatrix[ i ] = hfMat4( rw[ i ].rotation.ToMat3(), rw[ i ].position );
			skinMatrix[ i ] = worldMatrix[ i ] * restWorldMatrix[ i ].Inverse();
		}
	}

	// Skinning CPU reference: maps a bind-space vertex to its animated world
	// position for one bone (no blending — the caller blends by weight).
	HF_INLINE bool	TransformVertex( int b, const hfVec3 &v, hfVec3 &out ) const {
		if ( b < 0 || b >= boneCount ) return false;
		out = skinMatrix[ b ] * v;
		return true;
	}

	int		boneCount;
	hfAnimBone	bones[ HF_SKELETON_MAX_BONES ];
	hfBonePose	restPose[ HF_SKELETON_MAX_BONES ];
	hfBonePose	localPose[ HF_SKELETON_MAX_BONES ];
	hfBonePose	worldPose[ HF_SKELETON_MAX_BONES ];
	hfMat4		worldMatrix[ HF_SKELETON_MAX_BONES ];
	hfMat4		restWorldMatrix[ HF_SKELETON_MAX_BONES ];
	hfMat4		skinMatrix[ HF_SKELETON_MAX_BONES ];
};
#endif
