#ifndef __MATH_SIMD_GENERIC_H__
#define __MATH_SIMD_GENERIC_H__

/*
===============================================================================

	Generic implementation of hfSIMDProcessor

===============================================================================
*/

class hfSIMD_Generic : public hfSIMDProcessor {
public:
	virtual const char * VPCALL GetName() const;

	virtual void VPCALL MinMax( float &min,			float &max,				const float *src,		const int count );
	virtual	void VPCALL MinMax( hfVec2 &min,		hfVec2 &max,			const hfVec2 *src,		const int count );
	virtual void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfVec3 *src,		const int count );
	virtual	void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfDrawVert *src,	const int count );
	virtual	void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfDrawVert *src,	const triIndex_t *indexes,		const int count );

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );
	virtual void VPCALL Memset( void *dst,			const int val,			const int count );

	virtual void VPCALL BlendJoints( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL BlendJointsFast( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( hfJointMat *jointMats, const hfJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( hfJointQuat *jointQuats, const hfJointMat *jointMats, const int numJoints );
	virtual void VPCALL TransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
};

#endif /* !__MATH_SIMD_GENERIC_H__ */

